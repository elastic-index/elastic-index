
#include "./pcm/pcm-memory.cpp"
#include "./pcm/pcm-numa.cpp"
#include "./papi_util.cpp"
#include <sys/mman.h>
#include "indexkey.h"

// #include "microbench.h"

#include <cstring>
#include <cctype>
#include <atomic>
#include <iomanip>

thread_local long skiplist_steps = 0;
std::atomic<long> skiplist_total_steps;

size_t _LEAF_SLOTS = 16;//128;
size_t _INNER_SLOTS = 16;
static int INIT_LIMIT=50*1000000;
static int LIMIT=100*1000000;

template <typename KeyType, typename ValueType>
struct EntryType {
    KeyType key;
    ValueType value;
};

template <typename KeyType, typename ValueType>
struct TxnType {
    KeyType key;
    uint64_t value; // better, a union of ValueType and uint64_t?
    int op;
};

//#define USE_TBB

#ifdef USE_TBB
#include "tbb/tbb.h"
#endif

// Enable this if you need pre-allocation utilization
//#define BWTREE_CONSOLIDATE_AFTER_INSERT

#ifdef BWTREE_CONSOLIDATE_AFTER_INSERT
  #ifdef USE_TBB
  #warning "Could not use TBB and BwTree consolidate together"
  #endif
#endif

#ifdef BWTREE_COLLECT_STATISTICS
  #ifdef USE_TBB
  #warning "Could not use TBB and BwTree statistics together"
  #endif
#endif


// START OF STRING&UUID INCANTATIONS
static const size_t StrSize = 31;
static const size_t uuidSize = 16;
// #ifdef USE_STRING_KEY
// static const size_t StrSize = 31;
// static const size_t uuidSize = 31;
// #else
// #ifdef USE_UUID_KEY
// static const size_t StrSize = 16;
// static const size_t uuidSize = 16;
// #else
// #endif
// #endif

// static const size_t StrSize = 31;
typedef GenericKey<StrSize> StrType;
typedef GenericComparator<StrSize> StrCmp;
typedef GenericEqualityChecker<StrSize> StrEq;
typedef GenericHasher<StrSize> StrHash;

// static const size_t uuidSize = 16;
typedef GenericKey<uuidSize> uuidType;

template <std::size_t keySize>
class uuidComparator {
public:
  uuidComparator() {}

  inline bool operator()(const GenericKey<keySize> &lhs, const GenericKey<keySize> &rhs) const {
    const uint64_t * const val_lhs = (uint64_t *) lhs.data;
    const uint64_t * const val_rhs = (uint64_t *) rhs.data;
    for (int i = keySize/sizeof(uint64_t) - 1; i >= 0; i--) {
      if (val_lhs[i] != val_rhs[i])
        return val_lhs[i] < val_rhs[i];
    }
    return false;
  }
};

typedef uuidComparator<uuidSize> uuidCmp;
typedef GenericEqualityChecker<uuidSize> uuidEq;
typedef GenericHasher<uuidSize> uuidHash;
// END OF STRING&UUID INCANTATIONS

// #define DEBUG_LOADING

// Whether insert interleaves
//#define INTERLEAVED_INSERT

// Whether read operatoin miss will be counted
//#define COUNT_READ_MISS

// typedef uint64_t keytype;
// typedef std::less<uint64_t> keycomp;

static const uint64_t key_type=0;
static const uint64_t value_type=0; // 0 = random pointers, 1 = pointers to keys

extern bool hyperthreading;

// This is the flag for whather to measure memory bandwidth
static bool memory_bandwidth = false;
// Whether to measure NUMA Throughput
static bool numa = false;
// Whether we only perform insert
static bool insert_only = false;

// We could set an upper bound of the number of loaded keys
static int64_t max_init_key = -1;

double LOAD_TIME, INS_TIME, TXNS_TIME;
size_t LOAD_MEM, INS_MEM, TXNS_MEM, BENCH_MEM;

#include "util.h"

template<typename KeyType>
void load_key(std::istream &in, KeyType &key) {
  static std::string key_str;
  #ifdef USE_STRING_KEY
  in >> key_str;
  key.setFromString(key_str);    
  #else
    #ifdef USE_UUID_KEY
    int c;
    key_str.clear();
    for (int i = 0; i < 16; ++i) {
      in >> c;
      key_str.push_back((char)c);
    }
    key.setFromString(key_str);    
    #else
    in >> key;
    #endif
  #endif
}

size_t MemUsage(const string& str = "")
{
    size_t size = 0;
    char* line;
    size_t len;
    FILE* f = fopen("/proc/self/status", "r");

    if(f == nullptr) {
      fprintf(stderr, "Could not open /proc/self/statm to read memory usage\n");
      fflush(stderr);
      exit(1);
    }

    static int xxx = 0;
    FILE* fc;
    if (xxx)
      fc = fopen("mem.txt", "a");
    else
      fc = fopen("mem.txt", "w");
    if(fc == nullptr) {
      fprintf(stderr, "Could not open mem.txt to log memory usage measurement requests\n");
      fflush(stderr);
      exit(1);
    }
    
    fprintf(fc, "\n== call #%d (%s) ==\n\n", xxx++, (str.c_str()));

    len = 128;
    line = (char*) malloc(len);
 	
    /* Read memory size data from /proc/pid/status */
    while (1) {
        if (getline(&line, &len, f) == -1)
            break;
        fputs(line, fc);
	/* Find VmSize */
        if (!strncmp(line, "VmSize:", 7)) {
           char* vmsize = &line[7];
           len = strlen(vmsize);
           /* Get rid of " kB\n" */
           vmsize[len - 4] = 0;
           size = strtol(vmsize, 0, 0) * 1024;
          //  break;
        }
    }
    free(line);
    fclose(f);
    fclose(fc);

    return size;
}

void *allocFromOS(size_t size) {

    if ((size % 4096))
        size += 4096 - (size % 4096);

    void *p = mmap(0, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (p == MAP_FAILED) {
        std::cout << "Allocation error" << std::endl;
        exit(1);
    }
    return p;
}


// /*
//  * MemUsage() - Reads memory usage from /proc file system
//  */
// size_t MemUsage() {
//   FILE *fp = fopen("/proc/self/statm", "r");
//   if(fp == nullptr) {
//     fprintf(stderr, "Could not open /proc/self/statm to read memory usage\n");
//     exit(1);
//   }

//   unsigned long unused;
//   unsigned long rss;
//   if (fscanf(fp, "%ld %ld %ld %ld %ld %ld %ld", &unused, &rss, &unused, &unused, &unused, &unused, &unused) != 7) {
//     perror("");
//     exit(1);
//   }

//   (void)unused;
//   fclose(fp);

//   return rss * (4096 / 1024); // in KiB (not kB)
// }

//==============================================================
// LOAD
//==============================================================
template<typename KeyType>
inline void load(int wl,
                 int kt,
                 int index_type,
                 EntryType<KeyType, uint64_t>* storage,
                 TxnType<KeyType, uint64_t>* txns
) {
  std::string init_file;
  std::string txn_file;

if (kt == ZIPFIAN_KEY && wl == WORKLOAD_A) {
    init_file = "workloads/zipfian_workloada_input_50M.dat";
    txn_file =  "workloads/zipfian_workloada_txns_100M.dat";
  } else if (kt == ZIPFIAN_KEY && wl == WORKLOAD_C) {
    init_file = "workloads/zipfian_workloadc_input_50M.dat";
    txn_file =  "workloads/zipfian_workloadc_txns_100M.dat";
  } else if (kt == ZIPFIAN_KEY && wl == WORKLOAD_E) {
    init_file = "workloads/zipfian_workloade_input_50M.dat";
    txn_file =  "workloads/zipfian_workloade_txns_100M.dat";
  } else if (kt == ZIPFIAN_KEY && wl == WORKLOAD_B) {
    init_file = "workloads/zipfian_workloadb_input_50M.dat";
    txn_file =  "workloads/zipfian_workloadb_txns_100M.dat";
  } else if (kt == ZIPFIAN_KEY && wl == WORKLOAD_D) {
    init_file = "workloads/zipfian_workloadd_input_50M.dat";
    txn_file =  "workloads/zipfian_workloadd_txns_100M.dat";
  } else if (kt == ZIPFIAN_KEY && wl == WORKLOAD_F) {
    init_file = "workloads/zipfian_workloadf_input_50M.dat";
    txn_file =  "workloads/zipfian_workloadf_txns_100M.dat";
  } else if (kt == UNIFORM_KEY && wl == WORKLOAD_A) {
    init_file = "workloads/uniform_workloada_input_50M.dat";
    txn_file =  "workloads/uniform_workloada_txns_100M.dat";
  } else if (kt == UNIFORM_KEY && wl == WORKLOAD_C) {
    init_file = "workloads/uniform_workloadc_input_50M.dat";
    txn_file =  "workloads/uniform_workloadc_txns_100M.dat";
  } else if (kt == UNIFORM_KEY && wl == WORKLOAD_E) {
    init_file = "workloads/uniform_workloade_input_50M.dat";
    txn_file =  "workloads/uniform_workloade_txns_100M.dat";
  } else if (kt == UNIFORM_KEY && wl == WORKLOAD_B) {
    init_file = "workloads/uniform_workloadb_input_50M.dat";
    txn_file =  "workloads/uniform_workloadb_txns_100M.dat";
  } else if (kt == UNIFORM_KEY && wl == WORKLOAD_D) {
    init_file = "workloads/uniform_workloadd_input_50M.dat";
    txn_file =  "workloads/uniform_workloadd_txns_100M.dat";
  } else if (kt == UNIFORM_KEY && wl == WORKLOAD_F) {
    init_file = "workloads/uniform_workloadf_input_50M.dat";
    txn_file =  "workloads/uniform_workloadf_txns_100M.dat";
  } else if (kt == MONO_KEY && wl == WORKLOAD_A) {
    init_file = "workloads/monoint_workloada_input_50M.dat";
    txn_file =  "workloads/monoint_workloada_txns_100M.dat";
  } else if (kt == MONO_KEY && wl == WORKLOAD_C) {
    init_file = "workloads/monoint_workloadc_input_50M.dat";
    txn_file =  "workloads/monoint_workloadc_txns_100M.dat";
  } else if (kt == MONO_KEY && wl == WORKLOAD_E) {
    init_file = "workloads/monoint_workloade_input_50M.dat";
    txn_file =  "workloads/monoint_workloade_txns_100M.dat";
  } else if (kt == MONO_KEY && wl == WORKLOAD_B) {
    init_file = "workloads/monoint_workloadb_input_50M.dat";
    txn_file =  "workloads/monoint_workloadb_txns_100M.dat";
  } else if (kt == MONO_KEY && wl == WORKLOAD_D) {
    init_file = "workloads/monoint_workloadd_input_50M.dat";
    txn_file =  "workloads/monoint_workloadd_txns_100M.dat";
  } else if (kt == MONO_KEY && wl == WORKLOAD_F) {
    init_file = "workloads/monoint_workloadf_input_50M.dat";
    txn_file =  "workloads/monoint_workloadf_txns_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_A) {
    init_file = "workloads/string_workloada_input_50M.dat";
    txn_file =  "workloads/string_workloada_txns_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_C) {
    init_file = "workloads/string_workloadc_input_50M.dat";
    txn_file =  "workloads/string_workloadc_txns_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_E) {
    init_file = "workloads/string_workloade_input_50M.dat";
    txn_file =  "workloads/string_workloade_txns_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_B) {
    init_file = "workloads/string_workloadb_input_50M.dat";
    txn_file =  "workloads/string_workloadb_txns_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_D) {
    init_file = "workloads/string_workloadd_input_50M.dat";
    txn_file =  "workloads/string_workloadd_txns_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_F) {
    init_file = "workloads/string_workloadf_input_50M.dat";
    txn_file =  "workloads/string_workloadf_txns_100M.dat";
  } else if (kt == UUID_KEY && wl == WORKLOAD_A) {
    init_file = "workloads/uuid_workloada_input_50M.dat";
    txn_file =  "workloads/uuid_workloada_txns_100M.dat";
  } else if (kt == UUID_KEY && wl == WORKLOAD_C) {
    init_file = "workloads/uuid_workloadc_input_50M.dat";
    txn_file =  "workloads/uuid_workloadc_txns_100M.dat";
  } else if (kt == UUID_KEY && wl == WORKLOAD_E) {
    init_file = "workloads/uuid_workloade_input_50M.dat";
    txn_file =  "workloads/uuid_workloade_txns_100M.dat";
  } else if (kt == UUID_KEY && wl == WORKLOAD_B) {
    init_file = "workloads/uuid_workloadb_input_50M.dat";
    txn_file =  "workloads/uuid_workloadb_txns_100M.dat";
  } else if (kt == UUID_KEY && wl == WORKLOAD_D) {
    init_file = "workloads/uuid_workloadd_input_50M.dat";
    txn_file =  "workloads/uuid_workloadd_txns_100M.dat";
  } else if (kt == UUID_KEY && wl == WORKLOAD_F) {
    init_file = "workloads/uuid_workloadf_input_50M.dat";
    txn_file =  "workloads/uuid_workloadf_txns_100M.dat";
  } else {
    fprintf(stderr, "Unknown workload type or key type: %d, %d\n", wl, kt);
    exit(1);
  }

  std::ifstream infile_load(init_file);

  std::string op;
  KeyType key;
  int range;

  std::string insert("INSERT");
  std::string read("READ");
  std::string update("UPDATE");
  std::string scan("SCAN");

  int count = 0;
  for (count = 0; count < INIT_LIMIT && infile_load.good(); ++count) {
    infile_load >> op;
    if (op.compare(insert) != 0) {
      std::cout << "READING LOAD FILE FAIL!\n";
      return;
    }
    load_key<KeyType>(infile_load, key);
    storage[count].key = key;

    // If we have reached the max init key limit then just break
    if(max_init_key > 0 && count == max_init_key) {
      break;
    }
  }
  std::cerr << "Finished loading initial entries (" << count << " out of the expected " << INIT_LIMIT << ")\n";

  #ifdef DEBUG_LOADING
  std::cerr << "Here's the list of the loads:\n";
  for (count = 0; count < INIT_LIMIT; ++count)
    std::cerr << init_keys[count] << "\n";
  std::cerr << std::endl;
  #endif

  void *base_ptr = malloc(8);
  uint64_t base = (uint64_t)(base_ptr);
  free(base_ptr);

  // KeyType *init_keys_data = init_keys;

  if (value_type == 0) {
    for (count = 0; count < INIT_LIMIT; ++count)
      // storage[count].value = base + rand();
      storage[count].value = 42;
  }
  else {
    for (count = 0; count < INIT_LIMIT; ++count)
    // storage[count].value = 17;
      storage[count].value = reinterpret_cast<uint64_t>(&(storage[count].key));
  }

  // If we do not perform other transactions, we can skip txn file
  if(insert_only == true) {
    return;
  }

  // If we also execute transaction then open the
  // transacton file here
  std::ifstream infile_txn(txn_file);

  count = 0;
  for (count = 0; (count < LIMIT) && infile_txn.good(); ++count) {
    infile_txn >> op;
    load_key<KeyType>(infile_txn, key);
    if (op.compare(insert) == 0) {
      txns[count].key = key;
      txns[count].value = 1;
      txns[count].op = OP_INSERT;
    }
    else if (op.compare(read) == 0) {
      txns[count].key = key;
      txns[count].value = 0; // new
      txns[count].op = OP_READ;
    }
    else if (op.compare(update) == 0) {
      txns[count].key = key;
      txns[count].value = base + rand();
      txns[count].op = OP_UPSERT;
    }
    else if (op.compare(scan) == 0) {
      infile_txn >> range;
      // ops[count] = OP_READ;
      txns[count].key = key;
      txns[count].value = range;
      txns[count].op = OP_SCAN;
    }
    else {
      std::cout << "UNRECOGNIZED CMD!\n";
      return;
    }
  }
  std::cout << "Finished loading workload file with transactions (" << count << " out of the expected " << LIMIT << ")\n";

  #ifdef DEBUG_LOADING
  std::cerr << "Here's the list of the transactions:\n";
  for (count = 0; count < LIMIT; ++count)
    std::cerr << txns[count].op << " " << txns[count].key << " " << txns[count].value << "\n";
  std::cerr << std::endl;
  #endif

  // If it is YSCB-E workload then we compute average and stdvar
  if(LIMIT != 0 && wl == WORKLOAD_E) {
    // Average and variation
    long avg = 0, var = 0;

    for(auto i = 0; i < LIMIT; ++i) { // for(int r : ranges) {
      avg += txns[i].value;
    }

    avg /= (long)LIMIT;

    for(auto i = 0; i < LIMIT; ++i) { // for(int r : ranges) {
      var += ((txns[i].value - avg) * (txns[i].value - avg));
    }

    var /= (long)LIMIT;

    fprintf(stderr, "YCSB-E scan Avg length: %ld; Variance: %ld\n",
            avg, var);
  }

}

//==============================================================
// EXEC
//==============================================================
template<typename KeyType,
         typename KeyComparator=std::less<KeyType>,
         typename KeyEqual=std::equal_to<KeyType>,
         typename KeyHash=std::hash<KeyType>>
inline void exec(int wl,
                 int index_type,
                 int num_thread,
                 EntryType<KeyType, uint64_t>* storage,
                 TxnType<KeyType, uint64_t>* txns
) {
  auto mem0 = MemUsage("before inserts");
  Index<KeyType, uint64_t, KeyComparator> *idx = nullptr;
  if (_INNER_SLOTS == 16) {
    if (_LEAF_SLOTS == 16)
      idx = getInstance<KeyType, KeyComparator, KeyEqual, KeyHash, 16, 16>(index_type, key_type);
    else if (_LEAF_SLOTS == 32)
      idx = getInstance<KeyType, KeyComparator, KeyEqual, KeyHash, 16, 32>(index_type, key_type);
    else if (_LEAF_SLOTS == 64)
      idx = getInstance<KeyType, KeyComparator, KeyEqual, KeyHash, 16, 64>(index_type, key_type);
    else if (_LEAF_SLOTS == 128)
      idx = getInstance<KeyType, KeyComparator, KeyEqual, KeyHash, 16, 128>(index_type, key_type);
    else {
      std::cout << "\n\nUNSUPPORTED LEAF NODE SLOT NUMBER " << _LEAF_SLOTS << "\n\n";
      exit(0);
    }
  } else if (_INNER_SLOTS == 128)  {
    if (_LEAF_SLOTS == 16)
      idx = getInstance<KeyType, KeyComparator, KeyEqual, KeyHash, 128, 16>(index_type, key_type);
    else if (_LEAF_SLOTS == 32)
      idx = getInstance<KeyType, KeyComparator, KeyEqual, KeyHash, 128, 32>(index_type, key_type);
    else if (_LEAF_SLOTS == 64)
      idx = getInstance<KeyType, KeyComparator, KeyEqual, KeyHash, 128, 64>(index_type, key_type);      
    else if (_LEAF_SLOTS == 128)
      idx = getInstance<KeyType, KeyComparator, KeyEqual, KeyHash, 128, 128>(index_type, key_type);
    else {
      std::cout << "\n\nUNSUPPORTED LEAF NODE SLOT NUMBER: " << _LEAF_SLOTS << "\n\n";
      exit(0);
    }
  } else {
    std::cout << "\n\nUNSUPPORTED INNER NODE SLOT NUMBER " << _INNER_SLOTS << "\n\n";
    exit(0);
  }

  //WRITE ONLY TEST-----------------
  int count = INIT_LIMIT;//(int)init_keys.size();
  fprintf(stderr, "Populating the index with %d keys using %d threads\n", count, num_thread);

#ifdef USE_TBB
  std::cout << "USING TBB" << std::endl;
  tbb::task_scheduler_init init{num_thread};

  std::atomic<int> next_thread_id;
  next_thread_id.store(0);

  auto func = [idx, &storage, &next_thread_id](const tbb::blocked_range<size_t>& r) {
    size_t start_index = r.begin();
    size_t end_index = r.end();

    threadinfo *ti = threadinfo::make(threadinfo::TI_MAIN, -1);

    int thread_id = next_thread_id.fetch_add(1);
    idx->AssignGCID(thread_id);

    int gc_counter = 0;
    for(size_t i = start_index;i < end_index;i++) {
      #ifdef DEBUG_PRINT
      if (gc_counter % 1000 == 0) {
        fprintf(stderr, "#%d: Calling an insert of (%lu, %lu) with an address %p\n", gc_counter, init_keys[i], i, &init_keys[i]);
      }
      #endif
      idx->insert(storage[i].key,
        reinterpret_cast<uint64_t>(&(storage[i].key)),
        // values[i],
        ti
      );
      gc_counter++;
      if(gc_counter % 4096 == 0) {
        ti->rcu_quiesce();
      }
    }

    ti->rcu_quiesce();
    idx->UnregisterThread(thread_id);

    return;
  };

  idx->UpdateThreadLocal(num_thread);
  tbb::parallel_for(tbb::blocked_range<size_t>(0, count), func);
  idx->UpdateThreadLocal(1);
#else
  auto func = [idx, &storage, num_thread, index_type] \
              (uint64_t thread_id, bool) {
    size_t total_num_key = INIT_LIMIT;//init_keys.size();
    size_t key_per_thread = total_num_key / num_thread;
    size_t start_index = key_per_thread * thread_id;
    size_t end_index = start_index + key_per_thread;
    if (thread_id == num_thread-1)
      end_index = INIT_LIMIT;
    // std::cerr << "Thread #" << thread_id << " out of " << num_thread << " is responsible for the range [" << start_index << ", " << end_index << "]" <<std::endl;

    threadinfo *ti = threadinfo::make(threadinfo::TI_MAIN, -1);
    int gc_counter = 0;
#ifdef INTERLEAVED_INSERT
    for(size_t i = thread_id;i < total_num_key;i += num_thread) {
#else
    for(size_t i = start_index;i < end_index;i++) {
#endif
      if(index_type == TYPE_SKIPLIST) {
        idx->insert(storage[start_index + end_index - 1 - i].key,
                    reinterpret_cast<uint64_t>(&(storage[start_index + end_index - 1 - i].key)),
                    // values[start_index + end_index - 1 - i],
                    ti
        );
      } else {
#ifdef BWTREE_USE_DELTA_UPDATE
        idx->insert(storage[i].key,
                    reinterpret_cast<uint64_t>(&(storage[i].key)),
                    // values[i],
                    ti
        );
#else
        idx->insert_bwtree_fast(storage[start_index + end_index - 1 - i].key,
            reinterpret_cast<uint64_t>(&(storage[start_index + end_index - 1 - i].key))
          // values[i]
        );
#endif
      }
      gc_counter++;
      if(gc_counter % 4096 == 0) {
        ti->rcu_quiesce();
      }
    }

    ti->rcu_quiesce();

    return;
  };

  if(memory_bandwidth == true) {
    PCM_memory::StartMemoryMonitor();
  }

  if(numa == true) {
    PCM_NUMA::StartNUMAMonitor();
  }

  double start_time = get_now();
  StartThreads(idx, num_thread, func, false);
  double end_time = get_now();

  if(index_type == TYPE_SKIPLIST) {
    fprintf(stderr, "SkipList size = %lu\n", idx->GetIndexSize());
    fprintf(stderr, "Skiplist avg. steps = %f\n", (double)skiplist_total_steps / (double)INIT_LIMIT);//(double)init_keys.size());
  }

  if(memory_bandwidth == true) {
    PCM_memory::EndMemoryMonitor();
  }

  if(numa == true) {
    PCM_NUMA::EndNUMAMonitor();
  }
#endif

  // if (_INNER_SLOTS == 16) {
  //   if (_LEAF_SLOTS == 16) {
  //     auto btreeolc_seqtree = static_cast<BTreeOLCIndex<KeyType, KeyComparator, 16, 16>* >(idx);
  //     btreeolc_seqtree->print_tree();
  //   } else if (_LEAF_SLOTS == 32) {
  //     auto btreeolc_seqtree = static_cast<BTreeOLCIndex<KeyType, KeyComparator, 16, 32>* >(idx);
  //     btreeolc_seqtree->print_tree();
  //   } else if (_LEAF_SLOTS == 64) {
  //     auto btreeolc_seqtree = static_cast<BTreeOLCIndex<KeyType, KeyComparator, 16, 64>* >(idx);
  //     btreeolc_seqtree->print_tree();
  //   } else if (_LEAF_SLOTS == 128) {
  //     auto btreeolc_seqtree = static_cast<BTreeOLCIndex<KeyType, KeyComparator, 16, 128>* >(idx);
  //     btreeolc_seqtree->print_tree();
  //   } else {
  //     std::cout << "\n\nUNSUPPORTED LEAF NODE SLOT NUMBER " << _LEAF_SLOTS << "\n\n";
  //     exit(0);
  //   }
  // } else if (_INNER_SLOTS == 128)  {
  //   if (_LEAF_SLOTS == 16) {
  //     auto btreeolc_seqtree = static_cast<BTreeOLCIndex<KeyType, KeyComparator, 128, 16>* >(idx);
  //     btreeolc_seqtree->print_tree();
  //   } else if (_LEAF_SLOTS == 32) {
  //     auto btreeolc_seqtree = static_cast<BTreeOLCIndex<KeyType, KeyComparator, 128, 32>* >(idx);
  //     btreeolc_seqtree->print_tree();
  //   } else if (_LEAF_SLOTS == 64) {
  //     auto btreeolc_seqtree = static_cast<BTreeOLCIndex<KeyType, KeyComparator, 128, 64>* >(idx);
  //     btreeolc_seqtree->print_tree();
  //   } else if (_LEAF_SLOTS == 128) {
  //     auto btreeolc_seqtree = static_cast<BTreeOLCIndex<KeyType, KeyComparator, 128, 128>* >(idx);
  //     btreeolc_seqtree->print_tree();
  //   } else {
  //     std::cout << "\n\nUNSUPPORTED LEAF NODE SLOT NUMBER: " << _LEAF_SLOTS << "\n\n";
  //     exit(0);
  //   }
  // } else {
  //   std::cout << "\n\nUNSUPPORTED INNER NODE SLOT NUMBER " << _INNER_SLOTS << "\n\n";
  //   exit(0);
  // }

  double tput = count / (end_time - start_time) / 1000000; //Mops/sec
  INS_TIME = tput;
  size_t mem1 = MemUsage("after inserts");
  INS_MEM = mem1 - mem0;
  std::cout << "\033[1;32m";
  std::cout << "insert " << tput << "\n";
  std::cout << "\033[0m";

  #ifdef DEBUG_PRINT
  idx->print_stats();
  getchar();
  #endif

  // Only execute consolidation if BwTree delta chain is used
#ifdef BWTREE_CONSOLIDATE_AFTER_INSERT
  fprintf(stderr, "Starting consolidating delta chain on each level\n");
  idx->AfterLoadCallback();
#endif

  // If the workload only executes load phase then we return here
  if(insert_only == true) {
    delete idx;
    return;
  }

  // //READ/UPDATE/SCAN TEST----------------
  // int txn_num = GetTxnCount(ops, index_type);
  uint64_t sum = 0;
  uint64_t s = 0;

  // // if(INIT_LIMIT < LIMIT) { //values.size() < keys.size()) {
  // //   fprintf(stderr, "Values array too small\n");
  // //   exit(1);
  // // }

  // fprintf(stderr, "# of Txn: %d\n", txn_num);

  // This is used to count how many read misses we have found
  std::atomic<size_t> read_miss_counter{}, read_hit_counter{};
  read_miss_counter.store(0UL);
  read_hit_counter.store(0UL);

  auto func2 = [num_thread,
                idx,
                &read_miss_counter,
                &read_hit_counter,
                &txns
              ](uint64_t thread_id, bool) {
    size_t total_num_op = LIMIT;//ops.size();
    size_t op_per_thread = total_num_op / num_thread;
    size_t start_index = op_per_thread * thread_id;
    size_t end_index = start_index + op_per_thread;

    std::vector<uint64_t> v;
    v.reserve(10);

    threadinfo *ti = threadinfo::make(threadinfo::TI_MAIN, -1);
    int counter = 0;
    for(size_t i = start_index;i < end_index;i++) {
      #ifdef DEBUG_PRINT
      if (!((i - start_index) % (op_per_thread/10)))
          std::cerr << "Thread #" << thread_id << " has progressed until " << 10*((i - start_index) / (op_per_thread/10)) << "% of its task" << std::endl;
      #endif
      int op = txns[i].op;
      if (op == OP_INSERT) { //
      
        idx->insert(txns[i].key,
          reinterpret_cast<uint64_t>(&(txns[i].key)),
          // values[i],
          ti);
      }
      else if (op == OP_READ) { //READ
        v.clear();  
#ifdef BWTREE_USE_MAPPING_TABLE
        idx->find(txns[i].key, &v, ti);
#else
        idx->find_bwtree_fast(txns[i].key, &v);
#endif

        // If we count read misses then increment the
        // counter here if the vetor is empty
#ifdef COUNT_READ_MISS
        if(v.size() == 0UL) {
          read_miss_counter.fetch_add(1);
        } else {
          read_hit_counter.fetch_add(1);
        }
#endif
      }
      else if (op == OP_UPSERT) { //UPDATE
        // printf("... the next step is an upsert operation\n"); getchar();
        // fprintf(stderr, "UPSERT of (%lu, %lu) with an address %p\n", keys[i], reinterpret_cast<uint64_t>(&keys[i]), &keys[i]);
        idx->upsert(txns[i].key,
         reinterpret_cast<uint64_t>(&(txns[i].key)), //txns[i].value, // reinterpret_cast<uint64_t>(&keys[i]),
        ti);
      }
      else if (op == OP_SCAN) { //SCAN
        // fprintf(stderr, "  |__ scan operation\n");
        idx->scan(txns[i].key, txns[i].value, ti);
      }

      counter++;
      if(counter % 4096 == 0) {
        ti->rcu_quiesce();
      }
    }

    // Perform GC after all operations
    ti->rcu_quiesce();

    return;
  };

  if(memory_bandwidth == true) {
    PCM_memory::StartMemoryMonitor();
  }

  if(numa == true) {
    PCM_NUMA::StartNUMAMonitor();
  }

  start_time = get_now();

  StartThreads(idx, num_thread, func2, false);
  end_time = get_now();
  size_t mem2 = MemUsage("after txns");
  TXNS_MEM = mem2 - mem1;

  if(memory_bandwidth == true) {
    PCM_memory::EndMemoryMonitor();
  }

  if(numa == true) {
    PCM_NUMA::EndNUMAMonitor();
  }

  // Print out how many reads have missed in the index (do not have a value)
#ifdef COUNT_READ_MISS
  fprintf(stderr,
          "  Read misses: %lu; Read hits: %lu\n",
          read_miss_counter.load(),
          read_hit_counter.load());
#endif

  uint64_t txn_num = LIMIT;
  tput = txn_num / (end_time - start_time) / 1000000; //Mops/sec

  std::cout << "sum = " << sum << "\n";
  std::cout << "\033[1;31m";
  TXNS_TIME = (tput + (sum - sum));
  std::cout << "\ntxns   " << (tput + (sum - sum));
  // if (wl == WORKLOAD_A) {
  //   std::cout << "\nread/update " << (tput + (sum - sum));
  // } else if (wl == WORKLOAD_C) {
  //   std::cout << "\nread " << (tput + (sum - sum));
  // } else if (wl == WORKLOAD_E) {
  //   std::cout << "\ninsert/scan " << (tput + (sum - sum));
  // } else if (wl == WORKLOAD_) {
  //   std::cout << "\nread/update " << (tput + (sum - sum));
  // } else if (wl == WORKLOAD_C) {
  //   std::cout << "\nread " << (tput + (sum - sum));
  // } else if (wl == WORKLOAD_E) {
  //   std::cout << "\ninsert/scan " << (tput + (sum - sum));
  // } else {
  //   fprintf(stderr, "Unknown workload type: %d\n", wl);
  //   exit(1);
  // }

  std::cout << std::endl << std::endl;
  std::cout << "\033[0m" << "\n";

  if(index_type == TYPE_SKIPLIST) {
    fprintf(stderr, "SkipList size = %lu\n", idx->GetIndexSize());
    fprintf(stderr, "Skiplist avg. steps = %f\n", (double)skiplist_total_steps / (double)INIT_LIMIT);//(double)init_keys.size());
  }
  delete idx;

  return;
}

// /*
//  * run_rdtsc_benchmark() - This function runs the RDTSC benchmark which is a high
//  *                         contention insert-only benchmark
//  *
//  * Note that key num is the total key num
//  */
// void run_rdtsc_benchmark(int index_type, int thread_num, int key_num) {
//   Index<keytype, uint64_t, keycomp> *idx = getInstance<keytype, keycomp>(index_type, key_type);

//   auto func = [idx, thread_num, key_num](uint64_t thread_id, bool) {
//     size_t key_per_thread = key_num / thread_num;

//     threadinfo *ti = threadinfo::make(threadinfo::TI_MAIN, -1);

//     uint64_t *values = new uint64_t[key_per_thread];

//     int gc_counter = 0;
//     for(size_t i = 0;i < key_per_thread;i++) {
//       // Note that RDTSC may return duplicated keys from different cores
//       // to counter this we combine RDTSC with thread IDs to make it unique
//       // The counter value on a single core is always unique, though
//       uint64_t key = (Rdtsc() << 6) | thread_id;
//       values[i] = key;
//       //fprintf(stderr, "%lx\n", key);
//       if (gc_counter % 1000 == 0) {
//         fprintf(stderr, "#%d: Calling an insert of (%lu, %lu) with an address %p\n", gc_counter, key, reinterpret_cast<uint64_t>(values + i), &key);
//       }
//       idx->insert(key, reinterpret_cast<uint64_t>(values + i), ti);
//       gc_counter++;
//       if(gc_counter % 4096 == 0) {
//         ti->rcu_quiesce();
//       }
//     }

//     ti->rcu_quiesce();

//     delete [] values;

//     return;
//   };

//   if(numa == true) {
//     PCM_NUMA::StartNUMAMonitor();
//   }

//   double start_time = get_now();
//   StartThreads(idx, thread_num, func, false);
//   double end_time = get_now();

//   if(numa == true) {
//     PCM_NUMA::EndNUMAMonitor();
//   }

//   // Only execute consolidation if BwTree delta chain is used
// #ifdef BWTREE_CONSOLIDATE_AFTER_INSERT
//   idx->AfterLoadCallback();
// #endif

//   double tput = key_num * 1.0 / (end_time - start_time) / 1000000; //Mops/sec
//   std::cout << "insert " << tput << "\n";

//   return;
// }


template<typename KeyType,
         typename KeyComparator=std::less<KeyType>,
         typename KeyEqual=std::equal_to<KeyType>,
         typename KeyHash=std::hash<KeyType>>
void runBenchmarks(int wl, int kt, int index_type,
                 int num_thread, int repeat_counter) {
  auto mem0 = MemUsage("before loading");
  printf("Beginning the benchmark run (mem = %lu)\n", mem0);

  // If the key type is RDTSC we just run the special function
  if(kt != RDTSC_KEY) {
    // std::vector<KeyType> init_keys;
    // std::vector<KeyType> keys;
    // std::vector<uint64_t> values;
    // std::vector<int> ranges;
    // std::vector<int> ops; //INSERT = 0, READ = 1, UPDATE = 2

    // init_keys.reserve(50000000);
    // keys.reserve(10000000);
    // values.reserve(10000000);
    // ranges.reserve(10000000);
    // ops.reserve(10000000);
  EntryType<KeyType, uint64_t>* storage;
  TxnType<KeyType, uint64_t>* txns;

  void* ptr;

  ptr = allocFromOS(INIT_LIMIT * sizeof(EntryType<KeyType, uint64_t>));
  storage = new (ptr) EntryType<KeyType, uint64_t>[INIT_LIMIT];

  ptr = allocFromOS(LIMIT * sizeof(TxnType<KeyType, uint64_t>));
  txns = new (ptr) TxnType<KeyType, uint64_t>[LIMIT];

    memset(&storage[0], 0x00, INIT_LIMIT * sizeof(EntryType<KeyType, uint64_t>));
    memset(&txns[0], 0x00, LIMIT * sizeof(TxnType<KeyType, uint64_t>));

    load<KeyType>(wl, kt, index_type, storage, txns);
    auto mem1 = MemUsage("after loading");
    LOAD_MEM = mem1-mem0;
    printf("Finished loading workload file (mem = %lu)\n", mem1);

    if(index_type != TYPE_NONE) {
      // Then repeat executing the same workload
      while(repeat_counter > 0) {
        // auto mem2 = MemUsage();
        // auto mem2 = MemUsage("before");
        exec<KeyType, KeyComparator, KeyEqual, KeyHash>(wl, index_type, num_thread, storage, txns);
        // auto alll = malloc(100000000*4);
        // auto mem3 = MemUsage();
        BENCH_MEM = INS_MEM + TXNS_MEM;
        fprintf(stderr, "Finished execution (mem = %lu)\n", MemUsage("final result"));
        std::cout.setf(ios_base::fixed);
        std::cout << "\n\nSummary:\n" << std::setw(12)
                  // << "* load time = " << LOAD_TIME << "\n"
                  << "* load mem    = " << LOAD_MEM << "\n"
                  << "* insert tput = " << INS_TIME << "\n"
                  << "* insert mem  = " << INS_MEM << "\n"
                  << "* txns tput   = " << TXNS_TIME << "\n"
                  << "* txns mem    = " << TXNS_MEM << "\n"
                  << "* bench mem   = " << BENCH_MEM << std::endl;
        repeat_counter--;

    // long long unsigned all_loaded = init_keys.size()*sizeof(init_keys[0])
    //   + keys.size() * sizeof(keys[0])
    //   + values.size() * sizeof(values[0])
    //   + ranges.size() * sizeof(ranges[0])
    //   + ops.size() * sizeof(ops[0]);
    // std::cout << "LOADED so many bytes: " << all_loaded << std::endl;

        // std::cout << "Finished running benchmark (mem = " << MemUsage() << ")\n";
      }
    } else {
      fprintf(stderr, "Type None is selected - no execution phase\n");
    }
  } else {
    fprintf(stderr, "Running RDTSC benchmark...\n");
    // run_rdtsc_benchmark(index_type, num_thread, 50 * 1000 * 1000);
  }
}


int main(int argc, char *argv[]) {

  if (argc < 5) {
    std::cout << "Usage:\n";
    std::cout << "1. workload type: a, b, c, d, e, f, none\n";
    std::cout << "   \"none\" type means we just load the file and exit. \n"
                 "This serves as the base line for microbenchamrks\n";
    std::cout << "2. key distribution: uniform, zipfian, mono, string\n";
    std::cout << "3. index type: bwtree skiplist masstree artolc btreeolc btreertm\n";
    std::cout << "               hot stx concur_hot\n";
    std::cout << "               stx_trie stx_seqtree stx_subtrie\n";
    std::cout << "               btreeolc_seqtree btreeolc_trie btreeolc_subtrie\n";
    std::cout << "4. Number of threads: (1 - 40)\n";
    std::cout << "Optional arguments:\n";
    std::cout << "   --load-num=X: Only load X items of init data (X in 0..5000000)\n";
    std::cout << "   --txns-num=X: Only load X items of txns data (X in 0..10000000)\n";
    std::cout << "   --leaf-slots=X: Make leaf nodes have X slots (16, 32, 64 and 128 supported)\n";
    std::cout << "   --inner-slots=X: Make inner nodes have X slots (16 and 128 supported)\n";
    std::cout << "   --hyper: Whether to pin all threads on NUMA node 0\n";
    std::cout << "   --mem: Whether to monitor memory access\n";
    std::cout << "   --numa: Whether to monitor NUMA throughput\n";
    std::cout << "   --insert-only: Whether to only execute insert operations\n";

    return 1;
  }


  // Then read the workload type
  int wl;
  if (strcmp(argv[1], "a") == 0) {
    wl = WORKLOAD_A;
  } else if (strcmp(argv[1], "c") == 0) {
    wl = WORKLOAD_C;
  } else if (strcmp(argv[1], "e") == 0) {
    wl = WORKLOAD_E;
  } else if (strcmp(argv[1], "b") == 0) {
    wl = WORKLOAD_B;
  } else if (strcmp(argv[1], "d") == 0) {
    wl = WORKLOAD_D;
  } else if (strcmp(argv[1], "f") == 0) {
    wl = WORKLOAD_F;
  } else {
    fprintf(stderr, "Unknown workload: %s\n", argv[1]);
    exit(1);
  }

  // Then read key type
  int kt;
    if (strcmp(argv[2], "uniform") == 0) {
    kt = UNIFORM_KEY;
  } else if (strcmp(argv[2], "zipfian") == 0) {
    kt = ZIPFIAN_KEY;
  } else if (strcmp(argv[2], "mono") == 0) {
    kt = MONO_KEY;
  } else if (strcmp(argv[2], "string") == 0) {
    kt = STRING_KEY;
  } else if (strcmp(argv[2], "uuid") == 0) {
    kt = UUID_KEY;
  } else {
    fprintf(stderr, "Unknown key type: %s\n", argv[2]);
    exit(1);
  }

  int index_type;
  if (strcmp(argv[3], "bwtree") == 0)
    index_type = TYPE_BWTREE;
  else if (strcmp(argv[3], "masstree") == 0)
    index_type = TYPE_MASSTREE;
  else if (strcmp(argv[3], "artolc") == 0)
    index_type = TYPE_ARTOLC;
  else if (strcmp(argv[3], "btreeolc") == 0)
    index_type = TYPE_BTREEOLC;
  else if (strcmp(argv[3], "hot") == 0)
    index_type = TYPE_HOT;
  else if (strcmp(argv[3], "concur_hot") == 0)
    index_type = TYPE_CONCUR_HOT;
  else if (strcmp(argv[3], "stx") == 0)
    index_type = TYPE_STX;
  else if (strcmp(argv[3], "stx_hybrid") == 0)
    index_type = TYPE_STX_HYBRID;
  else if (strcmp(argv[3], "stx_trie") == 0)
    index_type = TYPE_STX_TRIE;
  else if (strcmp(argv[3], "stx_seqtree") == 0)
    index_type = TYPE_STX_SEQTREE;
  else if (strcmp(argv[3], "stx_subtrie") == 0)
    index_type = TYPE_STX_SUBTRIE;
  else if (strcmp(argv[3], "btreeolc_seqtree") == 0)
    index_type = TYPE_BTREEOLC_BLINDI_SEQ;
  else if (strcmp(argv[3], "btreeolc_trie") == 0)
    index_type = TYPE_BTREEOLC_BLINDI_TRIE;
  else if (strcmp(argv[3], "btreeolc_subtrie") == 0)
    index_type = TYPE_BTREEOLC_BLINDI_SUBTRIE;
  else if (strcmp(argv[3], "skiplist") == 0)
    index_type = TYPE_SKIPLIST;
  else if (strcmp(argv[3], "btreertm") == 0)
    index_type = TYPE_BTREERTM;
  else if (strcmp(argv[3], "none") == 0)
    // This is a special type used for measuring base cost (i.e.
    // only loading the workload files but do not invoke the index)
    index_type = TYPE_NONE;
  else {
    fprintf(stderr, "Unknown index type: %d\n", index_type);
    exit(1);
  }

  // Then read number of threads using command line
  int num_thread = atoi(argv[4]);
  if(num_thread < 1 || num_thread > 88) {
    fprintf(stderr, "Do not support %d threads\n", num_thread);
    exit(1);
  } else {
    fprintf(stderr, "Number of threads: %d\n", num_thread);
  }

  // Then read all remianing arguments
  int repeat_counter = 1;
  char **argv_end = argv + argc;
  for(char **v = argv + 5;v != argv_end;v++) {
    if(strcmp(*v, "--hyper") == 0) {
      // Enable hyoerthreading for scheduling threads
      hyperthreading = true;
    } else if(strcmp(*v, "--mem") == 0) {
      // Enable memory bandwidth measurement
      memory_bandwidth = true;
    } else if(strcmp(*v, "--numa") == 0) {
      numa = true;
    } else if(strcmp(*v, "--insert-only") == 0) {
      insert_only = true;
    } else if(strcmp(*v, "--repeat") == 0) {
      // If we repeat, then exec() will be called for 5 times
      repeat_counter = 5;
    } else if(strcmp(*v, "--max-init-key") == 0) {
      max_init_key = atoll(*(v + 1));
      if(max_init_key <= 0) {
        fprintf(stderr, "Illegal maximum init keys: %ld\n", max_init_key);
        exit(1);
      }

      // Ignore the next argument
      v++;
    } else if (strncmp(*v, "--load-num=", 11) == 0) {
      size_t num;
      sscanf(*v, "--load-num=%zd", &num);
      std::cout << "Requested to load this number of items: " << num << std::endl;   	
      if (num > INIT_LIMIT) {
        std::cout << " the --load-num argument must no greater than 50000000" << std::endl;
        exit(0);	
      }
      INIT_LIMIT = num;
    } else if (strncmp(*v, "--txns-num=", 11) == 0) {
      size_t num;
      sscanf(*v, "--txns-num=%zd", &num);
      std::cout << "Requested to execute this number of txns: " << num << std::endl;   	
      if (num > LIMIT) {
        std::cout << " the --txns-num argument must no greater than 10000000" << std::endl;
        exit(0);	
      }
      LIMIT = num;
    } else if (strncmp(*v, "--leaf-slots=", 13) == 0) {
      size_t leaf_num;
      sscanf(*v, "--leaf-slots=%zd", &leaf_num);
      std::cout << "Specified leaf-slot number: " << leaf_num << std::endl;   	
//      if ((leaf_num != 16) && (leaf_num != 32) && (leaf_num != 64) && (leaf_num != 128) && (leaf_num != 254) && (leaf_num != 256) && (leaf_num != 512) && (leaf_num != 1024)) {
//	     std::cout << " The leaf should be one of following: 16, 32, 64, 128, 254, 256, 512, 1024" << std::endl;
      if ((leaf_num != 16) && (leaf_num != 32) && (leaf_num != 64) && (leaf_num != 128)) {
        std::cout << " the --leaf-slots argument must be one of following: 16, 32, 64, 128" << std::endl;
        exit(0);	
      }
      _LEAF_SLOTS = leaf_num;
    } else if (strncmp(*v, "--inner-slots=", 14) == 0) {
      size_t inner_num;
      sscanf(*v, "--inner-slots=%zd", &inner_num); 
      std::cout << "Specified inner-slot number: " << inner_num << std::endl;  	
//      if ((leaf_num != 16) && (leaf_num != 32) && (leaf_num != 64) && (leaf_num != 128) && (leaf_num != 254) && (leaf_num != 256) && (leaf_num != 512) && (leaf_num != 1024)) {
//	     std::cout << " The leaf should be one of following: 16, 32, 64, 128, 254, 256, 512, 1024" << std::endl;
      if  ((inner_num != 16) && (inner_num != 128)) {
        std::cout << " the --inner-slots argument must be one of following: 16, 128" << std::endl;
        exit(0);	
      }
      _INNER_SLOTS = inner_num;
    } else {
      fprintf(stderr, "Unknown switch: %s\n", *v);
      exit(1);
    }
  }

  if(max_init_key != -1) {
    fprintf(stderr, "Maximum init keys: %ld\n", max_init_key);
    fprintf(stderr, "  NOTE: Memory is not affected in this case\n");
  }

#ifdef COUNT_READ_MISS
  fprintf(stderr, "  Counting read misses\n");
#endif

#ifdef BWTREE_CONSOLIDATE_AFTER_INSERT
  fprintf(stderr, "  BwTree will considate after insert phase\n");
#endif

#ifdef USE_TBB
  fprintf(stderr, "  Using Intel TBB to run concurrent tasks\n");
#endif

#ifdef INTERLEAVED_INSERT
  fprintf(stderr, "  Interleaved insert\n");
#endif

#ifdef BWTREE_COLLECT_STATISTICS
  fprintf(stderr, "  BwTree will collect statistics\n");
#endif

  // fprintf(stderr, "Leaf delta chain threshold: %d; Inner delta chain threshold: %d\n",
  //         LEAF_DELTA_CHAIN_LENGTH_THRESHOLD,
  //         INNER_DELTA_CHAIN_LENGTH_THRESHOLD);

#ifndef BWTREE_USE_MAPPING_TABLE
  fprintf(stderr, "  BwTree does not use mapping table\n");
  if(wl != WORKLOAD_C) {
    fprintf(stderr, "Could only use workload C\n");
    exit(1);
  }

  if(index_type != TYPE_BWTREE) {
    fprintf(stderr, "Could only use BwTree\n");
    exit(1);
  }
#endif

#ifndef BWTREE_USE_CAS
  fprintf(stderr, "  BwTree does not use CAS\n");
#endif

#ifndef BWTREE_USE_DELTA_UPDATE
  fprintf(stderr, "  BwTree does not use delta update\n");
  if(index_type != TYPE_BWTREE) {
    fprintf(stderr, "Could only use BwTree\n");
  }
#endif

#ifdef USE_OLD_EPOCH
  fprintf(stderr, "  BwTree uses old epoch\n");
#endif

  // If we do not interleave threads on two sockets then this will be printed
  if(hyperthreading == true) {
    fprintf(stderr, "  Hyperthreading for thread 10 - 19, 30 - 39\n");
  }

  if(repeat_counter != 1) {
    fprintf(stderr, "  Repeat for %d times (NOTE: Memory number may not be correct)\n",
            repeat_counter);
  }

  if(memory_bandwidth == true) {
    if(geteuid() != 0) {
      fprintf(stderr, "Please run the program as root in order to measure memory bandwidth\n");
      exit(1);
    }

    fprintf(stderr, "  Measuring memory bandwidth\n");

    PCM_memory::InitMemoryMonitor();
  }

  if(numa == true) {
    if(geteuid() != 0) {
      fprintf(stderr, "Please run the program as root in order to measure NUMA operations\n");
      exit(1);
    }

    fprintf(stderr, "  Measuring NUMA operations\n");

    // Call init here to avoid calling it mutiple times
    PCM_NUMA::InitNUMAMonitor();
  }

  if(insert_only == true) {
    fprintf(stderr, "Program will exit after insert operation\n");
  }

  // switch (_LEAF_SLOTS) {
  //   case 16:
  //     fprintf(stderr, "  BTree element pair count: %lu\n",
  //         (uint64_t)btreeolc::BTreeLeaf<uint64_t, uint64_t, 16>::maxEntries);
  //     break;
  //   case 128:
  //     fprintf(stderr, "  BTree element pair count: %lu\n",
  //         (uint64_t)btreeolc::BTreeLeaf<uint64_t, uint64_t, 128>::maxEntries);
  //     break;
  //   default:
  //     fprintf(stderr, "  BTree element pair count: (cannot print because the leaf-slots number is invalid)\n");
  //     break;
  // }

#ifdef USE_STRING_KEY
  if (kt == STRING_KEY)
    runBenchmarks<StrType, StrCmp, StrEq, StrHash>(wl, kt, index_type, num_thread, repeat_counter);
#else
  #ifdef USE_UUID_KEY
  if (kt == UUID_KEY)
    runBenchmarks<uuidType, uuidCmp, uuidEq, uuidHash>(wl, kt, index_type, num_thread, repeat_counter);
  #else
  if (kt == UNIFORM_KEY || kt == ZIPFIAN_KEY || kt == MONO_KEY)
    runBenchmarks<uint64_t>(wl, kt, index_type, num_thread, repeat_counter);
  #endif
#endif
exit_cleanup();

  return 0;
}
