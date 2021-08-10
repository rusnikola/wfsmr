/*
 * Copyright (c) 2020 Ruslan Nikolaev.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef HR_TRACKER_HPP
#define HR_TRACKER_HPP

#include <queue>
#include <list>
#include <vector>
#include <atomic>
#include "ConcurrentPrimitives.hpp"
#include "RAllocator.hpp"

#include "BaseTracker.hpp"

#define HR_INVPTR	((HRInfo*)-1LL)
#define MAX_HR		16
#define MAX_HRC		12

template<class T> class HRTracker: public BaseTracker<T>{
private:
	int task_num;
	int hr_num;
	int epochFreq;
	int freq;
	bool collect;

	
public:
	struct HRSlot;

	struct HRInfo {
		union {
			struct {
				union {
					std::atomic<HRInfo*> next;
					std::atomic<HRInfo*>* slot;
				};
				HRInfo* batch_link;
				union {
					std::atomic<uintptr_t> refs;
					HRInfo* batch_next;
				};
			};
			uint64_t birth_epoch;
		};
	};

	struct HRBatch {
		uint64_t min_epoch;
		HRInfo* first;
		HRInfo* last;
		size_t counter;
		size_t list_count;
		HRInfo* list;
		alignas(128) char pad[0];
	};

	struct HRSlot {
		// do not reorder
		std::atomic<HRInfo*> first[MAX_HR];
		std::atomic<uint64_t> epoch[MAX_HR];
		alignas(128) char pad[0];
	};

private:
	HRSlot* slots;
	HRBatch* batches;
	padded<uint64_t>* alloc_counters;
	paddedAtomic<uint64_t> epoch;

public:
	~HRTracker() { };
	HRTracker(int task_num, int hr_num, int epochFreq, int emptyFreq, bool collect): 
	 BaseTracker<T>(task_num),task_num(task_num),hr_num(hr_num),epochFreq(epochFreq*task_num),freq(emptyFreq),collect(collect){
		batches = (HRBatch*) memalign(alignof(HRBatch), sizeof(HRBatch) * task_num);
		slots = (HRSlot*) memalign(alignof(HRSlot), sizeof(HRSlot) * task_num);
		alloc_counters = new padded<uint64_t>[task_num];
		for (int i = 0; i<task_num; i++) {
			alloc_counters[i].ui = 0;
			batches[i].first = nullptr;
			batches[i].counter = 0;
			batches[i].list_count = 0;
			batches[i].list = nullptr;
			for (int j = 0; j<hr_num; j++){
				slots[i].first[j].store(HR_INVPTR, std::memory_order_release);
				slots[i].epoch[j].store(0, std::memory_order_release);
			}
		}
		epoch.ui.store(1, std::memory_order_release);
	}
	HRTracker(int task_num, int emptyFreq) : HRTracker(task_num,emptyFreq,true){}

	void __attribute__ ((deprecated)) reserve(uint64_t e, int tid) {
		return reserve(tid);
	}
	uint64_t getEpoch() {
		return epoch.ui.load(std::memory_order_acquire);
	}

	void* alloc(int tid) {
		alloc_counters[tid] = alloc_counters[tid]+1;
		if (alloc_counters[tid] % epochFreq == 0){
			epoch.ui.fetch_add(1, std::memory_order_acq_rel);
		}
		char* block = (char*) malloc(sizeof(HRInfo) + sizeof(T));
		HRInfo* info = (HRInfo*) (block + sizeof(T));
		info->birth_epoch = getEpoch();
		return (void*) block;
	}

	void reclaim(T* obj) {
		obj->~T();
		free ((char*) obj);
	}

	inline void free_list(HRInfo* list) {
		while (list != nullptr) {
			HRInfo* start = list->batch_link;
			list = list->next;
			do {
				T* obj = (T*) start - 1;
				start = start->batch_next;
				reclaim(obj);
				this->dec_retired(0); // tid=0, not used
			} while (start != nullptr);
		}
	}

	void traverse(HRInfo** list, HRInfo* next) {
		while (true) {
			HRInfo* curr = next;
			if (!curr)
				break;
			next = curr->next.load(std::memory_order_acquire);
			HRInfo* refs = curr->batch_link;
			if (refs->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) {
				refs->next = *list;
				*list = refs;
			}
		}
	}

	inline void traverse_cache(HRBatch* batch, HRInfo* next) {
		if (next != nullptr) {
			if (batch->list_count == MAX_HRC) {
				free_list(batch->list);
				batch->list = nullptr;
				batch->list_count = 0;
			}
			batch->list_count++;
			traverse(&batch->list, next);
		}
	}

	__attribute__((noinline)) uint64_t do_update(uint64_t curr_epoch, int index, int tid) {
		// Dereference previous nodes
		if (slots[tid].first[index].load(std::memory_order_acquire) != nullptr) {
			HRInfo* first = slots[tid].first[index].exchange(HR_INVPTR, std::memory_order_acq_rel);
			if (first != HR_INVPTR) traverse_cache(&batches[tid], first);
			slots[tid].first[index].store(nullptr, std::memory_order_seq_cst);
			curr_epoch = getEpoch();
		}
		slots[tid].epoch[index].store(curr_epoch, std::memory_order_seq_cst);
		return curr_epoch;
	}

	T* read(std::atomic<T*>& obj, int index, int tid, T* node) {
		uint64_t prev_epoch = slots[tid].epoch[index].load(std::memory_order_acquire);
		while (true) {
			T* ptr = obj.load(std::memory_order_acquire);
			uint64_t curr_epoch = getEpoch();
			if (curr_epoch == prev_epoch){
				return ptr;
			} else {
				prev_epoch = do_update(curr_epoch, index, tid);
			}
		}
	}

	void reserve_slot(T* obj, int index, int tid, T* node) {
		uint64_t prev_epoch = slots[tid].epoch[index].load(std::memory_order_acquire);
		while (true) {
			uint64_t curr_epoch = getEpoch();
			if (curr_epoch == prev_epoch){
				return;
			} else {
				prev_epoch = do_update(curr_epoch, index, tid);
			}
		}
	}

	void clear_all(int tid) {
		HRInfo* first[MAX_HR];
		for (int i = 0; i < hr_num; i++) {
			first[i] = slots[tid].first[i].exchange(HR_INVPTR, std::memory_order_acq_rel);
		}
		for (int i = 0; i < hr_num; i++) {
			if (first[i] != HR_INVPTR)
				traverse(&batches[tid].list, first[i]);
		}
		free_list(batches[tid].list);
		batches[tid].list = nullptr;
		batches[tid].list_count = 0;
	}

	void try_retire(HRBatch* batch) {
		HRInfo* curr = batch->first;
		HRInfo* refs = batch->last;
		uint64_t min_epoch = batch->min_epoch;
		// Find available slots
		HRInfo* last = curr;
		for (int i = 0; i < task_num; i++) {
			for (int j = 0; j < hr_num; j++) {
				HRInfo* first = slots[i].first[j].load(std::memory_order_acquire);
				if (first == HR_INVPTR)
					continue;
				uint64_t epoch = slots[i].epoch[j].load(std::memory_order_acquire);
				if (epoch < min_epoch)
					continue;
				if (last == refs)
					return;
				last->slot = &slots[i].first[j];
				last = last->batch_next;
			}
		}
		// Retire if successful
		size_t adjs = 0;
		for (; curr != last; curr = curr->batch_next) {
			std::atomic<HRInfo*>* slot_first = curr->slot;
			std::atomic<uint64_t>* slot_epoch = (std::atomic<uint64_t>*) (slot_first + MAX_HR);
			HRInfo* prev = slot_first->load(std::memory_order_acquire);
			do {
				if (prev == HR_INVPTR)
					goto next;
				uint64_t epoch = slot_epoch->load(std::memory_order_acquire);
				if (epoch < min_epoch)
					goto next;
				curr->next.store(prev, std::memory_order_relaxed);
			} while (!slot_first->compare_exchange_weak(prev, curr, std::memory_order_acq_rel, std::memory_order_acquire));
			adjs++;
next: ;
		}
		// Adjust the reference count
		if (refs->refs.fetch_add(adjs, std::memory_order_acq_rel) == -adjs) {
			refs->next = nullptr;
			free_list(refs);
		}
		// Reset the batch
		batch->first = nullptr;
		batch->counter = 0;
	}

	void retire(T* obj, int tid) {
		if (obj == nullptr) { return; }
		HRInfo* node = (HRInfo *) (obj + 1);
		if (!batches[tid].first) {
			batches[tid].min_epoch = node->birth_epoch;
			batches[tid].last = node;
		} else {
			if (batches[tid].min_epoch > node->birth_epoch)
				batches[tid].min_epoch = node->birth_epoch;
			node->batch_link = batches[tid].last;
		}

		// Implicitly initialize refs to 0 for the last node
		node->batch_next = batches[tid].first;

		batches[tid].first = node;
		batches[tid].counter++;
		if (collect && batches[tid].counter % freq == 0) {
			batches[tid].last->batch_link = node;
			try_retire(&batches[tid]);
		}
	}

	bool collecting() { return collect; }
};


#endif
