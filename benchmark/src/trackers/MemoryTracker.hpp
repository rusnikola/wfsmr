/*

Copyright 2017 University of Rochester

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. 

*/



#ifndef MEMORY_TRACKER_HPP
#define MEMORY_TRACKER_HPP

#include <queue>
#include <list>
#include <vector>
#include <atomic>
#include "ConcurrentPrimitives.hpp"
#include "RAllocator.hpp"

#include "BaseTracker.hpp"
#include "RCUTracker.hpp"
#include "HyalineTrackerEL.hpp"
#include "HyalineSTrackerEL.hpp"
#include "HyalineOTrackerEL.hpp"
#include "HyalineOSTrackerEL.hpp"
#include "HyalineTrackerTR.hpp"
#include "HyalineSTrackerTR.hpp"
#include "HyalineOTrackerTR.hpp"
#include "HyalineOSTrackerTR.hpp"
#include "IntervalTracker.hpp"
#include "RangeTrackerNew.hpp"
#include "HazardTracker.hpp"
#include "HETracker.hpp"
#include "WFETracker.hpp"
#include "HRTracker.hpp"
#include "WFRTracker.hpp"
#if !(__x86_64__ || __ppc64__)
#include "RangeTrackerTP.hpp"
#endif




enum TrackerType{
	//for epoch-based trackers.
	NIL = 0,
	RCU = 2,
	Interval = 4,
	Range = 6,
	Range_new = 8,
	QSBR = 10,
	Range_TP = 12,
	//for HP-like trackers.
	Hazard = 1,
	Hazard_dynamic = 3,
	HE = 5,
	WFE = 7,
	HR = 9,
	WFR = 11,
	HyalineEL = 18,
	HyalineSEL = 19,
	HyalineOEL = 20,
	HyalineOSEL = 21,
	HyalineELSMALL = 22,
	HyalineSELSMALL = 23,
	HyalineTR = 14,
	HyalineSTR = 15,
	HyalineOTR = 16,
	HyalineOSTR = 17
};

class BaseMT {
public:
	virtual void lastExit(int tid) = 0;
};

extern int count_retired;

template<class T>
class MemoryTracker : public BaseMT{
private:
	BaseTracker<T>* tracker = NULL;
	TrackerType type = NIL;
	padded<int*>* slot_renamers = NULL;
public:
	MemoryTracker(GlobalTestConfig* gtc, int epoch_freq, int empty_freq, int slot_num, bool collect){
		count_retired = gtc->count_retired;
		int task_num = gtc->task_num + gtc->task_stall;
		std::string tracker_type = gtc->getEnv("tracker");
		if (tracker_type.empty()){
			tracker_type = "RCU";
			gtc->setEnv("tracker", "RCU");
		}

		slot_renamers = new padded<int*>[task_num];
		for (int i = 0; i < task_num; i++){
			slot_renamers[i].ui = new int[slot_num];
			for (int j = 0; j < slot_num; j++){
				slot_renamers[i].ui[j] = j;
			}
		}
		if (tracker_type == "NIL"){
			tracker = new BaseTracker<T>(task_num);
			type = NIL;
		} else if (tracker_type == "RCU"){
			tracker = new RCUTracker<T>(task_num, epoch_freq, empty_freq, collect);
			type = RCU;
		} else if (tracker_type == "HyalineEL"){
			tracker = new HyalineELTracker<T>(task_num, epoch_freq, empty_freq, 128, collect);
			type = HyalineEL;
		} else if (tracker_type == "HyalineSEL"){
			tracker = new HyalineSELTracker<T>(task_num, epoch_freq, empty_freq, 128, collect);
			type = HyalineSEL;
		}  else if (tracker_type == "HyalineOEL"){
			tracker = new HyalineOELTracker<T>(task_num, epoch_freq, empty_freq, collect);
			type = HyalineOEL;
		} else if (tracker_type == "HyalineOSEL"){
			tracker = new HyalineOSELTracker<T>(task_num, epoch_freq, empty_freq, collect);
			type = HyalineOSEL;
		} else if (tracker_type == "HyalineELSMALL"){
			tracker = new HyalineELTracker<T>(task_num, epoch_freq, empty_freq, 32, collect);
			type = HyalineELSMALL;
		} else if (tracker_type == "HyalineSELSMALL"){
			tracker = new HyalineSELTracker<T>(task_num, epoch_freq, empty_freq, 32, collect);
			type = HyalineSELSMALL;
		} else if (tracker_type == "HyalineTR"){
			tracker = new HyalineTRTracker<T>(task_num, epoch_freq, empty_freq, 32, collect);
			type = HyalineTR;
		} else if (tracker_type == "HyalineSTR"){
			tracker = new HyalineSTRTracker<T>(task_num, epoch_freq, empty_freq, 32, collect);
			type = HyalineSTR;
		}  else if (tracker_type == "HyalineOTR"){
			tracker = new HyalineOTRTracker<T>(task_num, epoch_freq, empty_freq, collect);
			type = HyalineOTR;
		} else if (tracker_type == "HyalineOSTR"){
			tracker = new HyalineOSTRTracker<T>(task_num, epoch_freq, empty_freq, collect);
			type = HyalineOSTR;
		} else if (tracker_type == "Range_new"){
			tracker = new RangeTrackerNew<T>(task_num, epoch_freq, empty_freq, collect);
			type = Range_new;
		} else if (tracker_type == "Hazard"){
			tracker = new HazardTracker<T>(task_num, slot_num, empty_freq, collect);
			type = Hazard;
		} else if (tracker_type == "HE"){
			// tracker = new HETracker<T>(task_num, slot_num, 1, collect);
			tracker = new HETracker<T>(task_num, slot_num, epoch_freq, empty_freq, collect);
			type = HE;
		} else if (tracker_type == "WFE"){
			tracker = new WFETracker<T>(task_num, slot_num, epoch_freq, empty_freq, collect);
			type = WFE;
		} else if (tracker_type == "HR"){
			tracker = new HRTracker<T>(task_num, slot_num, epoch_freq, empty_freq, collect);
			type = HR;
		} else if (tracker_type == "WFR"){
			tracker = new WFRTracker<T>(task_num, slot_num, epoch_freq, empty_freq, collect);
			type = WFR;
		} else if (tracker_type == "QSBR"){
			tracker = new RCUTracker<T>(task_num, epoch_freq, empty_freq, type_QSBR, collect);
			type = QSBR;
		} else if (tracker_type == "Interval"){
			tracker = new IntervalTracker<T>(task_num, epoch_freq, empty_freq, collect);
			type = Interval;
		}
		
		// only compile in 32 bit mode
#if !(__x86_64__ || __ppc64__)
		else if (tracker_type == "TP"){
			tracker = new RangeTrackerTP<T>(task_num, epoch_freq, empty_freq, collect);
			type = Range_TP;
		}
#endif

		else {
			errexit("constructor - tracker type error.");
		}
		
		
	}

	void lastExit(int tid) {
		tracker->last_end_op(tid);
	}

	void* alloc(){
		return tracker->alloc();
	}

	void* alloc(int tid){
		return tracker->alloc(tid);
	}
	//NOTE: reclaim shall be only used to thread-local objects.
	void reclaim(T* obj){
		if(obj!=nullptr)
			tracker->reclaim(obj);
	}

	void reclaim(T* obj, int tid){
		if (obj!=nullptr)
			tracker->reclaim(obj, tid);
	}

	void start_op(int tid){
		//tracker->inc_opr(tid);
		tracker->start_op(tid);
	}

	void end_op(int tid){
		tracker->end_op(tid);
	}

	T* read(std::atomic<T*>& obj, int idx, int tid, T* node){
		return tracker->read(obj, slot_renamers[tid].ui[idx], tid, node);
	}

	void reserve_slot(T* ptr, int slot, int tid, T* node){
		return tracker->reserve_slot(ptr, slot, tid, node);
	}

	void transfer(int src_idx, int dst_idx, int tid){
		int tmp = slot_renamers[tid].ui[src_idx];
		slot_renamers[tid].ui[src_idx] = slot_renamers[tid].ui[dst_idx];
		slot_renamers[tid].ui[dst_idx] = tmp;
	}

	void release(int idx, int tid){
		tracker->release(slot_renamers[tid].ui[idx], tid);
	}
	
	void clear_all(int tid){
		tracker->clear_all(tid);
	}

	void retire(T* obj, int tid){
		tracker->inc_retired(tid);
		tracker->retire(obj, tid);
	}

	uint64_t get_retired_cnt(int tid){
		if (type){
			return tracker->get_retired_cnt(tid);
		} else {
			return 0;
		}
	}
};


#endif
