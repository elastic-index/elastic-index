
#include "indexkey.h"
#include "microbench.h"
#include "index.h"

#ifndef _UTIL_H
#define _UTIL_H

bool hyperthreading = false;

//This enum enumerates index types we support
enum {
  TYPE_BWTREE = 0,
  TYPE_MASSTREE,
  TYPE_ARTOLC,
  TYPE_BTREEOLC,
  TYPE_SKIPLIST,
  TYPE_BTREERTM,
  TYPE_NONE,
  TYPE_HOT,
  TYPE_CONCUR_HOT,
  TYPE_STX,
  TYPE_STX_HYBRID,
  TYPE_STX_TRIE,
  TYPE_STX_SEQTREE,
  TYPE_STX_SUBTRIE,
  TYPE_BTREEOLC_BLINDI_SEQ,
  TYPE_BTREEOLC_BLINDI_TRIE,
  TYPE_BTREEOLC_BLINDI_SUBTRIE
};

// These are workload operations
enum {
  OP_INSERT,
  OP_READ,
  OP_UPSERT,
  OP_SCAN,
};

// These are YCSB workloads
enum {
  WORKLOAD_A,
  WORKLOAD_C,
  WORKLOAD_E,
  WORKLOAD_B,
  WORKLOAD_D,
  WORKLOAD_F,
};

// These are key types we use for running the benchmark
enum {
  UNIFORM_KEY,
  ZIPFIAN_KEY,
  MONO_KEY,
  RDTSC_KEY,
  EMAIL_KEY,
  UUID_KEY,
  STRING_KEY,
};

//==============================================================
// GET INSTANCE
//==============================================================
template<typename KeyType,
         typename KeyComparator=std::less<KeyType>, 
         typename KeyEqual=std::equal_to<KeyType>, 
         typename KeyHash=std::hash<KeyType>,
         int innerSlots = 16, 
         int leafSlots = 128>
Index<KeyType, uint64_t, KeyComparator> *getInstance(const int type, const uint64_t kt) {
  // if (type == TYPE_BWTREE)
    // return new BwTreeIndex<KeyType, KeyComparator, KeyEqual, KeyHash>(kt);
  // else if (type == TYPE_MASSTREE)
    // return new MassTreeIndex<KeyType, KeyComparator>(kt);
  // else if (type == TYPE_ARTOLC)
    // return nullptr;
      // return new ArtOLCIndex<KeyType, KeyComparator>(kt);
  if (type == TYPE_BTREEOLC)
    return new BTreeOLCIndex<KeyType, KeyComparator, innerSlots, leafSlots>(kt);
  // else if (type == TYPE_SKIPLIST)
    // return new SkipListIndex<KeyType, KeyComparator>(kt);
    // return nullptr;
  // else if (type == TYPE_BTREERTM)
    // return nullptr;
    // return new BTreeRTMIndex<KeyType, KeyComparator>(kt); 
  else if (type == TYPE_HOT) 
    return new HotIndex<KeyType, uint64_t, KeyComparator, innerSlots, leafSlots>(kt);
  else if (type == TYPE_CONCUR_HOT) 
    return new ConcurrentHotIndex<KeyType, uint64_t, KeyComparator, innerSlots, leafSlots>(kt);
  else if (type == TYPE_STX) 
    return new STXIndex<KeyType, uint64_t, KeyComparator, innerSlots, leafSlots>(kt);
  else if (type == TYPE_STX_TRIE) {
    return new STXTrieIndex<KeyType, uint64_t, KeyComparator, innerSlots, leafSlots>(kt);
  } else if (type == TYPE_STX_HYBRID) {
    return new STXHybridIndex<KeyType, uint64_t, KeyComparator, innerSlots, leafSlots>(kt);
  } else if (type == TYPE_STX_SEQTREE) {
    return new STXSeqtreeIndex<KeyType, uint64_t, KeyComparator, innerSlots, leafSlots>(kt);
  } else if (type == TYPE_STX_SUBTRIE) {
    return new STXSubtrieIndex<KeyType, uint64_t, KeyComparator, innerSlots, leafSlots>(kt);
  } else if (type == TYPE_BTREEOLC_BLINDI_SEQ) {
    return new BTreeOLCSeqtreeIndex<KeyType, uint64_t, KeyComparator, innerSlots, leafSlots>(kt);
  } else if (type == TYPE_BTREEOLC_BLINDI_TRIE) {
    return new BTreeOLCTrieIndex<KeyType, uint64_t, KeyComparator, innerSlots, leafSlots>(kt);
  } else if (type == TYPE_BTREEOLC_BLINDI_SUBTRIE) {
    return new BTreeOLCSubtrieIndex<KeyType, uint64_t, KeyComparator, innerSlots, leafSlots>(kt);
  } else {
    fprintf(stderr, "Unknown index type: %d\n", type);
    exit(1);
  }
  
  return nullptr;
}

inline double get_now() { 
struct timeval tv; 
  gettimeofday(&tv, 0); 
  return tv.tv_sec + tv.tv_usec / 1000000.0; 
} 

/*
 * Rdtsc() - This function returns the value of the time stamp counter
 *           on the current core
 */
inline uint64_t Rdtsc()
{
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return (((uint64_t) hi << 32) | lo);
}

// This is the order of allocation

static int core_alloc_map_hyper[] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, // cpu #0
  88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,  // cpu #0 HTs
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, // cpu #1
  110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, // cpu #1 HTs
  44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, // cpu #2
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, // cpu #2 HTs
  66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, // cpu #3
  154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, // cpu #3 HTs
};


static int core_alloc_map_numa[] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, // cpu #0
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, // cpu #1
  44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, // cpu #2
  66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, // cpu #3
  88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,  // cpu #0 HTs
  110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, // cpu #1 HTs
  132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, // cpu #2 HTs
  154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, // cpu #3 HTs
};


constexpr static size_t MAX_CORE_NUM = 88;

inline void PinToCore(size_t thread_id) {
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);

  size_t core_id = thread_id % MAX_CORE_NUM;

  if(hyperthreading == true) {
    CPU_SET(core_alloc_map_hyper[core_id], &cpu_set);
  } else {
    CPU_SET(core_alloc_map_numa[core_id], &cpu_set);
  }

  int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set);
  if(ret != 0) {
    fprintf(stderr, "PinToCore() returns non-0\n");
    exit(1);
  }

  return;
}

template <typename KeyType, typename KeyComp, typename Fn, typename... Args>
void StartThreads(Index<KeyType, uint64_t, KeyComp> *tree_p,
                  uint64_t num_threads,
                  Fn &&fn,
                  Args &&...args) {
  std::vector<std::thread> thread_group;

  if(tree_p != nullptr) {
    tree_p->UpdateThreadLocal(num_threads);
  }

  auto fn2 = [tree_p, &fn](uint64_t thread_id, Args ...args) {
    if(tree_p != nullptr) {
      tree_p->AssignGCID(thread_id);
    }

    PinToCore(thread_id);
    fn(thread_id, args...);

    if(tree_p != nullptr) {
      tree_p->UnregisterThread(thread_id);
    }

    return;
  };

  for (uint64_t thread_itr = 0; thread_itr < num_threads; ++thread_itr) {
    thread_group.push_back(std::thread{fn2, thread_itr, std::ref(args...)});
  }

  for (uint64_t thread_itr = 0; thread_itr < num_threads; ++thread_itr) {
    thread_group[thread_itr].join();
  }

  // Print statistical data before we destruct thread local data
#ifdef BWTREE_COLLECT_STATISTICS
  tree_p->CollectStatisticalCounter(num_threads);
#endif

  if(tree_p != nullptr) {
    tree_p->UpdateThreadLocal(1);
  }

  return;
}

// /*
//  * GetTxnCount() - Counts transactions and return 
//  */
// template <bool upsert_hack=true>
// int GetTxnCount(int* ops,
//                 int index_type) {
//   int count = 0;
 
//   for(auto i = 0; i < LIMIT; ++i) {
//     auto op = ops[i];
//     switch(op) {
//       case OP_INSERT:
//       case OP_READ:
//       case OP_SCAN:
//         count++;
//         break;
//       case OP_UPSERT:
//         count++;

//         break;
//       default:
//         fprintf(stderr, "Unknown operation\n");
//         exit(1);
//         break;
//     }
//   }

//   return count;
// }


#endif
