#ifndef SKIP_LIST
#define SKIP_LIST

#include <atomic>
#include <string>
#include <list>
#include "Harness.hpp"
#include "ROrderedMap.hpp"
#include "MemoryTracker.hpp"
#include "RetiredMonitorable.hpp"

// 6 levels at most for now
#define SL_MAX_LEVEL 5 // (number + 1) * 2 + 1 must fit in the number of HP/HE/WFE/HE/WFR indices

#ifdef NGC
#define COLLECT false
#else
#define COLLECT true
#endif

template <class K, class V> class SkipList : public RUnorderedMap<K, V>, public RetiredMonitorable {
private:

	struct Node {
		Node(const K &_key, const V &_value, size_t _topLevel)
		  : key(_key), value(_value), topLevel(_topLevel), refcnt(_topLevel+1) {
			next = new std::atomic<Node*>[topLevel+1];
		}
		~Node() {
			delete [] next;
		}
		K key;
		V value;
		size_t topLevel;
		std::atomic<size_t> refcnt;
		std::atomic<Node*> *next;
	};

	Node* head;

	MemoryTracker<Node>* memory_tracker;
	std::mt19937_64 gen;

	inline Node *alloc_node(int tid) {
		void *ptr = memory_tracker->alloc(tid);
		return new (ptr) Node({}, {}, SL_MAX_LEVEL);
	}

	inline Node *alloc_node(int tid, const K &key, const V &value,
			size_t height) {
		void *ptr = memory_tracker->alloc(tid);
		return new (ptr) Node(key, value, height);
	}

public:
	SkipList(GlobalTestConfig* gtc) : RetiredMonitorable(gtc), gen(1) {
		int epochf = gtc->getEnv("epochf").empty()? 150:stoi(gtc->getEnv("epochf"));
		int emptyf = gtc->getEnv("emptyf").empty()? 30:stoi(gtc->getEnv("emptyf"));
		memory_tracker = new MemoryTracker<Node>(gtc, epochf, emptyf, (SL_MAX_LEVEL + 1) * 2 + 1, COLLECT);
		this->setBaseMT(memory_tracker);
		head = alloc_node(0);
		for (size_t i = 0; i <= SL_MAX_LEVEL; i++) {
			head->next[i].store(nullptr, std::memory_order_relaxed);
		}
	}

	~SkipList() { }

	bool find(const K &key, int tid, Node **preds, Node **succs)
	{
		int idx;
		Node *pred, *curr, *succ;

		retry:
		pred = head;
		idx = 0;
		for (ssize_t l = SL_MAX_LEVEL; l >= 0; l--) {
			curr = memory_tracker->read(pred->next[l], idx+1, tid, pred);
			if ((size_t) curr & 0x1UL) goto retry;
			while (curr) {
				succ = memory_tracker->read(curr->next[l], idx+2, tid, curr);
				while ((size_t) succ & 0x1UL) {
					succ = (Node *) ((size_t) succ & ~0x1UL);
					if (!pred->next[l].compare_exchange_strong(curr, succ))
						goto retry;
					if (curr->refcnt.fetch_sub(1) == 1)
						memory_tracker->retire(curr, tid);
					curr = succ;
					memory_tracker->transfer(idx+2, idx+1, tid);
					if (!curr)
						break;
					succ = memory_tracker->read(curr->next[l], idx+2, tid, curr);
				}
				if (!curr || curr->key >= key)
					break;
				pred = curr;
				memory_tracker->transfer(idx+1, idx, tid);
				curr = succ;
				memory_tracker->transfer(idx+2, idx+1, tid);
			}
			preds[l] = pred;
			succs[l] = curr;
			idx += 2;
		}
		return curr && curr->key == key;
	}

	optional<V> get(K key, int tid)
	{
		Node *preds[SL_MAX_LEVEL+1], *succs[SL_MAX_LEVEL+1];
		optional<V> res={};

		collect_retired_size(memory_tracker->get_retired_cnt(tid), tid);
		memory_tracker->start_op(tid);

		if (find(key, tid, preds, succs))
			res = succs[0]->value;

		memory_tracker->end_op(tid);
		memory_tracker->clear_all(tid);
		return res;
	}

	optional<V> put(K key, V val, int tid)
	{
		optional<V> res={};
		bool isPresent;
		Node *preds[SL_MAX_LEVEL+1], *succs[SL_MAX_LEVEL+1];
		size_t topLevel = gen() % (SL_MAX_LEVEL + 1);

		collect_retired_size(memory_tracker->get_retired_cnt(tid), tid);
		memory_tracker->start_op(tid);

		Node *pred, *succ, *newNode = alloc_node(tid, key, val, topLevel);

		do {
			isPresent = find(key, tid, preds, succs);
			for (size_t i = 0; i <= topLevel; i++)
				newNode->next[i].store(succs[i], std::memory_order_relaxed);
			pred = preds[0];
			succ = succs[0];
		} while (!pred->next[0].compare_exchange_strong(succ, newNode));

		if (isPresent) {
			Node *nodeToRemove = succ;
			for (size_t l = nodeToRemove->topLevel; l >= 1; l--) {
				succ = nodeToRemove->next[l].load();
				do {
					if ((size_t) succ & 0x1)
						break;
				} while (!nodeToRemove->next[l].compare_exchange_weak(succ, (Node *) ((size_t) succ | 0x1)));
			}
			succ = nodeToRemove->next[0].load();
			while (!((size_t) succ & 0x1)) {
				if (nodeToRemove->next[0].compare_exchange_strong(succ, (Node *) ((size_t) succ | 0x1))) {
					res = nodeToRemove->value;
					if (newNode->next[0].compare_exchange_strong(nodeToRemove, succ)) {
						if (nodeToRemove->refcnt.fetch_sub(1) == 1)
							memory_tracker->retire(nodeToRemove, tid);
					}
					break;
				}
			}
			if (topLevel != 0) {
				if (!find(key, tid, preds, succs) || succs[0] != newNode) {
					// The node is already in the process of deletion,
					// roll back
					if (newNode->refcnt.fetch_sub(topLevel) == topLevel)
						memory_tracker->retire(newNode, tid);
					goto done;
				}
			}
		}

		for (size_t l = 1; l <= topLevel; l++) {
			while (1) {
				pred = preds[l];
				succ = succs[l];
				Node *expected = newNode->next[l].load();
				while (expected != succ) {
					if ((size_t) expected & 0x1UL) {
						// The node is already in the process of deletion,
						// roll back
						if (newNode->refcnt.fetch_sub(topLevel + 1 - l) == topLevel + 1 - l)
							memory_tracker->retire(newNode, tid);
						goto done;
					}
					newNode->next[l].compare_exchange_weak(expected, succ);
				}
				if (pred->next[l].compare_exchange_strong(succ, newNode))
					break;
				if (!find(key, tid, preds, succs) || succs[0] != newNode) {
					// The node is already in the process of deletion,
					// roll back
					if (newNode->refcnt.fetch_sub(topLevel + 1 - l) == topLevel + 1 - l)
						memory_tracker->retire(newNode, tid);
					goto done;
				}
			}
		}

done:
		memory_tracker->end_op(tid);
		memory_tracker->clear_all(tid);
		return res;
	}

	bool insert(K key, V val, int tid)
	{
		bool res = false;
		Node *preds[SL_MAX_LEVEL+1], *succs[SL_MAX_LEVEL+1];
		size_t topLevel = gen() % (SL_MAX_LEVEL + 1);

		collect_retired_size(memory_tracker->get_retired_cnt(tid), tid);
		memory_tracker->start_op(tid);

		Node *pred, *succ, *newNode = alloc_node(tid, key, val, topLevel);

		do {
			if (find(key, tid, preds, succs)) {
				memory_tracker->reclaim(newNode, tid);
				goto error;
			}
			for (size_t i = 0; i <= topLevel; i++)
				newNode->next[i].store(succs[i], std::memory_order_relaxed);
			pred = preds[0];
			succ = succs[0];
		} while (!pred->next[0].compare_exchange_strong(succ, newNode));

		for (size_t l = 1; l <= topLevel; l++) {
			while (1) {
				pred = preds[l];
				succ = succs[l];
				Node *expected = newNode->next[l].load();
				while (expected != succ) {
					if ((size_t) expected & 0x1UL) {
						// The node is already in the process of deletion,
						// roll back
						if (newNode->refcnt.fetch_sub(topLevel + 1 - l) == topLevel + 1 - l)
							memory_tracker->retire(newNode, tid);
						goto done;
					}
					newNode->next[l].compare_exchange_weak(expected, succ);
				}
				if (pred->next[l].compare_exchange_strong(succ, newNode))
					break;
				if (!find(key, tid, preds, succs) || succs[0] != newNode) {
					// The node is already in the process of deletion,
					// roll back
					if (newNode->refcnt.fetch_sub(topLevel + 1 - l) == topLevel + 1 - l)
						memory_tracker->retire(newNode, tid);
					goto done;
				}
			}
		}

done:
		res = true;

error:
		memory_tracker->end_op(tid);
		memory_tracker->clear_all(tid);
		return res;
	}

	optional<V> remove(K key, int tid)
	{
		Node *preds[SL_MAX_LEVEL+1], *succs[SL_MAX_LEVEL+1];
		optional<V> res={};

		collect_retired_size(memory_tracker->get_retired_cnt(tid), tid);
		memory_tracker->start_op(tid);

		Node *nodeToRemove, *succ;

		if (!find(key, tid, preds, succs))
			goto done;
		nodeToRemove = succs[0];
		for (size_t l = nodeToRemove->topLevel; l >= 1; l--) {
			succ = nodeToRemove->next[l].load();
			do {
				if ((size_t) succ & 0x1)
					break;
			} while (!nodeToRemove->next[l].compare_exchange_weak(succ, (Node *) ((size_t) succ | 0x1)));
		}
		succ = nodeToRemove->next[0].load();
		while (!((size_t) succ & 0x1)) {
			if (nodeToRemove->next[0].compare_exchange_strong(succ, (Node *) ((size_t) succ | 0x1))) {
				res = nodeToRemove->value;
				find(key, tid, preds, succs);
				goto done;
			}
		}

done:
		memory_tracker->end_op(tid);
		memory_tracker->clear_all(tid);
		return res;
	}

	optional<V> replace(K key, V val, int tid)
	{
		optional<V> res={};
		return res;
	}
};

template <class K, class V> class SkipListFactory : public RideableFactory{
	SkipList<K, V>* build(GlobalTestConfig* gtc){
		return new SkipList<K, V>(gtc);
	}
};

#endif
