// #include "microbench.h"
#include "index.h"
#include <sys/mman.h>
#include <iomanip>

// Used for skiplist
thread_local long skiplist_steps = 0;
std::atomic<long> skiplist_total_steps;

extern bool hyperthreading;

// using KeyEqualityChecker = GenericEqualityChecker<31>;
// using KeyHashFunc = GenericHasher<31>;

size_t _LEAF_SLOTS = 128;
size_t _INNER_SLOTS = 16;
static int INIT_LIMIT=50*1000000;
static int LIMIT=10*1000000;

static const uint64_t key_type=0;
static const uint64_t value_type=1; // 0 = random pointers, 1 = pointers to keys

#ifdef USE_STRING_KEY
static const size_t StrSize = 31;
static const size_t uuidSize = 31;
#else
#ifdef USE_UUID_KEY
static const size_t StrSize = 16;
static const size_t uuidSize = 16;
#else
#endif
#endif

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

#include "util.h"

// #define USE_27MB_FILE

// Whether to exit after insert operation
static bool insert_only = false;

double LOAD_TIME, INS_TIME, TXNS_TIME;
size_t LOAD_MEM, INS_MEM, TXNS_MEM, BENCH_MEM;

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
    if (p == MAP_FAILED)
        exit(1);

    return p;
}

//==============================================================
// LOAD
//==============================================================
template<typename KeyType>
inline void load(int wl,
                 int kt,
                 int index_type,
                 KeyType* init_keys,
                 KeyType* keys,
                 uint64_t* values,
                 int* ranges,
                 int* ops) {
  std::string init_file;
  std::string txn_file;

  // If we do not use the 27MB file then use old set of files; Otherwise
  // use 27 MB email workload
#ifndef USE_27MB_FILE
  // 0 = a, 1 = c, 2 = e
  if (kt == EMAIL_KEY && wl == WORKLOAD_A) {
    init_file = "workloads/email_loada_zipf_int_100M.dat";
    txn_file = "workloads/email_txnsa_zipf_int_100M.dat";
  } else if (kt == EMAIL_KEY && wl == WORKLOAD_C) {
    init_file = "workloads/email_loadc_zipf_int_100M.dat";
    txn_file = "workloads/email_txnsc_zipf_int_100M.dat";
  } else if (kt == EMAIL_KEY && wl == WORKLOAD_E) {
    init_file = "workloads/email_loade_zipf_int_100M.dat";
    txn_file = "workloads/email_txnse_zipf_int_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_A) {
    init_file = "workloads/string_workloada_input_50M.dat";
    txn_file =  "workloads/string_workloada_txns_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_C) {
    init_file = "workloads/string_workloadc_input_50M.dat";
    txn_file =  "workloads/string_workloadc_txns_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_E) {
    init_file = "workloads/string_workloade_input_50M.dat";
    txn_file =  "workloads/string_workloade_txns_100M.dat";
  } else if (kt == UUID_KEY && wl == WORKLOAD_A) {
    init_file = "workloads/uuid_workloada_input_50M.dat";
    txn_file =  "workloads/uuid_workloada_txns_100M.dat";
  } else if (kt == UUID_KEY && wl == WORKLOAD_C) {
    init_file = "workloads/uuid_workloadc_input_50M.dat";
    txn_file =  "workloads/uuid_workloadc_txns_100M.dat";
  } else if (kt == UUID_KEY  && wl == WORKLOAD_E) {
    init_file = "workloads/uuid_workloade_input_50M.dat";
    txn_file =  "workloads/uuid_workloade_txns_100M.dat";
  } else {
    fprintf(stderr, "Unknown workload or key type: %d, %d\n", wl, kt);
    exit(1);
  }
#else
  if (kt == EMAIL_KEY && wl == WORKLOAD_A) {
    init_file = "workloads/email_load.dat";
    txn_file = "workloads/email_a.dat";
  } else if (kt == EMAIL_KEY && wl == WORKLOAD_C) {
    init_file = "workloads/email_load.dat";
    txn_file = "workloads/email_c.dat";
  } else if (kt == EMAIL_KEY && wl == WORKLOAD_E) {
    init_file = "workloads/email_load.dat";
    txn_file = "workloads/email_e.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_A) {
    init_file = "workloads/string_workloada_input_50M.dat";
    txn_file =  "workloads/string_workloada_txns_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_C) {
    init_file = "workloads/string_workloadc_input_50M.dat";
    txn_file =  "workloads/string_workloadc_txns_100M.dat";
  } else if (kt == STRING_KEY && wl == WORKLOAD_E) {
    init_file = "workloads/string_workloade_input_50M.dat";
    txn_file =  "workloads/string_workloade_txns_100M.dat";
  } else {
    fprintf(stderr, "Unknown workload or key type: %d, %d\n", wl, kt);
    exit(1);
  }
#endif

  std::ifstream infile_load(init_file);

  std::string op;
  std::string key_str;
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
    #ifdef USE_STRING_KEY
    infile_load >> key_str;
    #endif
    #ifdef USE_UUID_KEY
    int c;
    key_str.clear();
    for (int i = 0; i < 16; ++i) {
      infile_load >> c;
      key_str.push_back((char)c);
    }
    #endif
    // std::cerr << count << ": " <<  key_str << std::endl;
    key.setFromString(key_str);
    // std::cerr << count << ": " <<  key << std::endl;
    init_keys[count] = key;
  }
  std::cout << "Finished loading initial entries (" << count << " out of the expected " << INIT_LIMIT << ")\n";

  #ifdef DEBUG_LOADING
  std::cerr << "Here's the list of the loads:\n";
  for (count = 0; count < INIT_LIMIT; ++count)
    std::cerr << init_keys[count] << "\n";
  std::cerr << std::endl;
  #endif

  // uint64_t value = 0;
  void *base_ptr = malloc(8);
  uint64_t base = (uint64_t)(base_ptr);
  free(base_ptr);

  KeyType *init_keys_data = init_keys;//init_keys.data();

  if (value_type == 0) {
    for (count = 0; count < INIT_LIMIT; ++count) {
      // value = base + rand();
      values[count] = base + rand();
    }
  }
  else {
    for (count = 0; count < INIT_LIMIT; ++count)
      values[count] = (uint64_t)init_keys_data[count].data;
  }

  // For insert only mode we return here
  if(insert_only == true) {
    return;
  }

  std::ifstream infile_txn(txn_file);
  for (count = 0; count < LIMIT && infile_txn.good(); ++count) {
    infile_txn >> op;
    // std::cerr << "op is: " << op << std::endl;
    #ifdef USE_STRING_KEY
    infile_txn >> key_str;
    #endif
    #ifdef USE_UUID_KEY
    int c;
    key_str.clear();
    for (int i = 0; i < 16; ++i) {
      infile_txn >> c;
      key_str.push_back((char)c);
    }
    #endif
    // std::cerr << count << ": " <<  key_str << std::endl;
    key.setFromString(key_str);
    if (op.compare(insert) == 0) {
      ops[count] = OP_INSERT;
      keys[count] = key;
      ranges[count] = 1;
    }
    else if (op.compare(read) == 0) {
      ops[count] = OP_READ;
      keys[count] = key;
    }
    else if (op.compare(update) == 0) {
      ops[count] = OP_UPSERT;
      keys[count] = key;
    }
    else if (op.compare(scan) == 0) {
      infile_txn >> range;
      ops[count] = OP_SCAN;
      // ops[count] = OP_READ;
      keys[count] = key;
      ranges[count] = range;
    }
    else {
      std::cout << "UNRECOGNIZED CMD: " << op << std::endl;
      return;
    }
  }
  std::cout << "Finished loading workload file with transactions (" << count << " out of the expected " << LIMIT << ")\n";

  #ifdef DEBUG_LOADING
  std::cerr << "Here's the list of the transactions:\n";
  for (count = 0; count < LIMIT; ++count)
    std::cerr << ops[count] << " " << keys[count] << " " << ranges[count] << "\n";
  std::cerr << std::endl;
  #endif

}

//==============================================================
// EXEC
//==============================================================
template<typename KeyType,
         typename KeyCmp,
         typename KeyEq,
         typename KeyHash>
inline void exec(int wl,
                 int index_type,
                 int num_thread,
                 KeyType* init_keys,
                 KeyType* keys,
                 uint64_t* values,
                 int* ranges,
                 int* ops) {
  size_t mem0 = MemUsage("before inserts");
  Index<KeyType, uint64_t, KeyCmp> *idx = nullptr;
  if (_INNER_SLOTS == 16) {
    if (_LEAF_SLOTS == 16)
      idx = getInstance<KeyType, KeyCmp, KeyEq, KeyHash, 16, 16>(index_type, key_type);
    else if (_LEAF_SLOTS == 128)
      idx = getInstance<KeyType, KeyCmp, KeyEq, KeyHash, 16, 128>(index_type, key_type);
    else {
      std::cout << "\n\nUNSUPPORTED LEAF NODE SLOT NUMBER\n\n";
      exit(0);
    }
  } else if (_INNER_SLOTS == 128)  {
    if (_LEAF_SLOTS == 16)
      idx = getInstance<KeyType, KeyCmp, KeyEq, KeyHash, 128, 16>(index_type, key_type);
    else if (_LEAF_SLOTS == 128)
      idx = getInstance<KeyType, KeyCmp, KeyEq, KeyHash, 128, 128>(index_type, key_type);
    else {
      std::cout << "\n\nUNSUPPORTED LEAF NODE SLOT NUMBER\n\n";
      exit(0);
    }
  } else {
    std::cout << "\n\nUNSUPPORTED INNER NODE SLOT NUMBER\n\n";
    exit(0);
  }

  // WRITE ONLY TEST--------------
  int count = INIT_LIMIT;
  double start_time = get_now();
  auto func = [idx, &init_keys, num_thread, &values](uint64_t thread_id, bool) {
    size_t total_num_key = INIT_LIMIT;
    size_t key_per_thread = total_num_key / num_thread;
    size_t start_index = key_per_thread * thread_id;
    size_t end_index = start_index + key_per_thread;

    threadinfo *ti = threadinfo::make(threadinfo::TI_MAIN, -1);

    int counter = 0;
    for(size_t i = start_index;i < end_index;i++) {
      idx->insert(init_keys[i], i, ti);
      counter++;
      if(counter % 4096 == 0) {
        ti->rcu_quiesce();
      }
    }

    ti->rcu_quiesce();

    return;
  };

  StartThreads(idx, num_thread, func, false);

  double end_time = get_now();
  size_t mem1 = MemUsage("after inserts, before txns");
  INS_MEM = mem1 - mem0;
  double tput = count / (end_time - start_time) / 1000000; //Mops/sec

  if(index_type == TYPE_SKIPLIST) {
    fprintf(stderr, "SkipList size = %lu\n", idx->GetIndexSize());
  }
  
  INS_TIME = tput;
  std::cout << "\033[1;32m";
  std::cout << "insert " << tput;
  std::cout << "\033[0m" << "\n";

  if(insert_only == true) {
    delete idx;
    return;
  }

  //READ/UPDATE/SCAN TEST----------------
  start_time = get_now();
  // int txn_num = GetTxnCount(ops, index_type);
  uint64_t sum = 0;

#ifdef PAPI_IPC
  //Variables for PAPI
  float real_time, proc_time, ipc;
  long long ins;
  int retval;

  if((retval = PAPI_ipc(&real_time, &proc_time, &ins, &ipc)) < PAPI_OK) {
    printf("PAPI error: retval: %d\n", retval);
    exit(1);
  }
#endif

#ifdef PAPI_CACHE
  int events[3] = {PAPI_L1_TCM, PAPI_L2_TCM, PAPI_L3_TCM};
  long long counters[3] = {0, 0, 0};
  int retval;

  if ((retval = PAPI_start_counters(events, 3)) != PAPI_OK) {
    fprintf(stderr, "PAPI failed to start counters: %s\n", PAPI_strerror(retval));
    exit(1);
  }
#endif

  if(INIT_LIMIT < LIMIT) {
    fprintf(stderr, "Values array too small\n");
    exit(1);
  }

  // fprintf(stderr, "# of Txn: %d\n", txn_num);

  auto func2 = [num_thread,
                idx,
                &keys,
                &values,
                &ranges,
                &ops](uint64_t thread_id, bool) {
    size_t total_num_op = LIMIT;
    size_t op_per_thread = total_num_op / num_thread;
    size_t start_index = op_per_thread * thread_id;
    size_t end_index = start_index + op_per_thread;

    std::vector<uint64_t> v;
    v.reserve(10);

    threadinfo *ti = threadinfo::make(threadinfo::TI_MAIN, -1);

    int counter = 0;
    for(size_t i = start_index;i < end_index;i++) {
      int op = ops[i];

      if (op == OP_INSERT) { //INSERT
        idx->insert(keys[i], static_cast<uint64_t>(i), ti);
      }
      else if (op == OP_READ) { //READ
        v.clear();
        idx->find(keys[i], &v, ti);
      }
      else if (op == OP_UPSERT) { //UPDATE
        idx->upsert(keys[i], static_cast<uint64_t>(i), ti);
      }
      else if (op == OP_SCAN) { //SCAN
        idx->scan(keys[i], ranges[i], ti);
      }

      counter++;
      if(counter % 4096 == 0) {
        ti->rcu_quiesce();
      }
    }

    ti->rcu_quiesce();

    return;
  };

  StartThreads(idx, num_thread, func2, false);

  end_time = get_now();
  size_t mem2 = MemUsage("after txns");
  TXNS_MEM = mem2 - mem1;

#ifdef PAPI_IPC
  if((retval = PAPI_ipc(&real_time, &proc_time, &ins, &ipc)) < PAPI_OK) {
    printf("PAPI error: retval: %d\n", retval);
    exit(1);
  }

  std::cout << "Time = " << real_time << "\n";
  std::cout << "Tput = " << LIMIT/real_time << "\n";
  std::cout << "Inst = " << ins << "\n";
  std::cout << "IPC = " << ipc << "\n";
#endif

#ifdef PAPI_CACHE
  if ((retval = PAPI_read_counters(counters, 3)) != PAPI_OK) {
    fprintf(stderr, "PAPI failed to read counters: %s\n", PAPI_strerror(retval));
    exit(1);
  }

  std::cout << "L1 miss = " << counters[0] << "\n";
  std::cout << "L2 miss = " << counters[1] << "\n";
  std::cout << "L3 miss = " << counters[2] << "\n";
#endif

  tput = LIMIT / (end_time - start_time) / 1000000; //Mops/sec

  std::cout << "\033[1;31m";
  TXNS_TIME = (tput + (sum - sum));
  if (wl == WORKLOAD_A) {  
    std::cout << "read/update " << (tput + (sum - sum));
  }
  else if (wl == WORKLOAD_C) {
    std::cout << "read " << (tput + (sum - sum));
  }
  else if (wl == WORKLOAD_E) {
    std::cout << "insert/scan " << (tput + (sum - sum));
  }
  else {
    std::cout << "read/update " << (tput + (sum - sum));
  }

  std::cout << "\033[0m" << "\n";

  delete idx;

  return;
}

template<typename KeyType,
         typename KeyCmp,
         typename KeyEq,
         typename KeyHash>
void runBenchmarks(int wl, int kt, int index_type, int num_thread, int repeat_counter)  {
  auto mem0 = MemUsage("before loads");
  auto ts0 = get_now();

  KeyType* init_keys;
  KeyType* keys;
  uint64_t* values;
  int* ranges;
  int* ops;

  void* ptr;

  ptr = allocFromOS(INIT_LIMIT * sizeof(KeyType));
  init_keys = new (ptr) KeyType[INIT_LIMIT];

  ptr = allocFromOS(INIT_LIMIT * sizeof(uint64_t));
  values = new (ptr) uint64_t[INIT_LIMIT];

  ptr = allocFromOS(LIMIT * sizeof(KeyType));
  keys = new (ptr) KeyType[LIMIT];

  ptr = allocFromOS(LIMIT * sizeof(int));
  ranges = new (ptr) int[LIMIT];

  ptr = allocFromOS(LIMIT * sizeof(int));
  ops = new (ptr) int[LIMIT];

  load<KeyType>(wl, kt, index_type, init_keys, keys, values, ranges, ops);
  auto ts1 = get_now();
  fprintf(stderr, "Finish loading (Mem = %lu)\n", MemUsage());
  auto mem1 = MemUsage("after loads");
  LOAD_MEM = mem1-mem0;

  // long long unsigned all_loaded =
  //   init_keys.capacity()*sizeof(init_keys[0])
  //   + keys.capacity() * sizeof(keys[0])
  //   + values.capacity() * sizeof(values[0])
  //   + ranges.capacity() * sizeof(ranges[0])
  //   + ops.capacity() * sizeof(ops[0]);
  // std::cout << "SHOULD HAVE loaded so many bytes: " << all_loaded << std::endl;

  while(repeat_counter > 0) {
    // auto mem2 = MemUsage();
    exec<KeyType, KeyCmp, KeyEq, KeyHash>(wl, index_type, num_thread, init_keys, keys, values, ranges, ops);
    // auto mem3 = MemUsage();
    BENCH_MEM = INS_MEM + TXNS_MEM;
    fprintf(stderr, "Finished execution (Mem = %lu)\n", MemUsage());
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
  }
}

int main(int argc, char *argv[]) {

  if (argc < 5) {
    std::cout << "Usage:\n";
    std::cout << "1. workload type: a, c, e\n";
    std::cout << "2. key distribution: string uuid\n";
    std::cout << "3. index type: bwtree skiplist masstree artolc btreeolc btreertm\n";
    std::cout << "               hot stx concur_hot\n";
    std::cout << "               stx_trie stx_seqtree stx_subtrie\n";
    std::cout << "               btreeolc_seqtree btreeolc_trie btreeolc_subtrie\n";
    std::cout << "4. Number of threads: (1 - 40)\n";
    std::cout << "Optional arguments:\n";
    std::cout << "   --leaf-slots=X: Make leaf nodes have X slots (16 and 128 supported)\n";
    std::cout << "   --inner-slots=X: Make inner nodes have X slots (16 and 128 supported)\n";
    std::cout << "   --repeat: Repeat 5 times\n";
    std::cout << "   --hyper: Whether to pin all threads on NUMA node 0\n";
    std::cout << "   --insert-only: Whether to only execute insert operations\n";
    std::cout << "   --repeat: Repeat 5 times\n";
    return 1;
  }

  int wl;
  if (strcmp(argv[1], "a") == 0) {
    wl = WORKLOAD_A;
  } else if (strcmp(argv[1], "c") == 0) {
    wl = WORKLOAD_C;
  } else if (strcmp(argv[1], "e") == 0) {
    wl = WORKLOAD_E;
  } else {
    fprintf(stderr, "Unknown workload type: %s\n", argv[1]);
    exit(1);
  }

  // Then read key type
  int kt;
  if (strcmp(argv[2], "email") == 0) {
    kt = EMAIL_KEY;
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

    return 1;
  } else {
    fprintf(stderr, "Number of threads: %d\n", num_thread);
  }

  fprintf(stderr, "Leaf delta chain threshold: %d; Inner delta chain threshold: %d\n",
          LEAF_DELTA_CHAIN_LENGTH_THRESHOLD,
          INNER_DELTA_CHAIN_LENGTH_THRESHOLD);


  // Then read all remianing arguments
  int repeat_counter = 1;
  char **argv_end = argv + argc;
  for(char **v = argv + 5;v != argv_end;v++) {
    if(strcmp(*v, "--hyper") == 0) {
      // Enable hyoerthreading for scheduling threads
      hyperthreading = true;
    } else if(strcmp(*v, "--insert-only") == 0) {
      insert_only = true;
    } else if(strcmp(*v, "--repeat") == 0) {
      repeat_counter = 5;
    } else if (strncmp(*v, "--load-num=", 11) == 0) {
      size_t num;
      sscanf(*v, "--load-num=%zd", &num);
      std::cout << "Requested to load this number of items: " << num << std::endl;   	
      if (num > 50*1000000) {
        std::cout << " the --load-num argument must no greater than 50000000" << std::endl;
        exit(0);	
      }
      INIT_LIMIT = num;
    } else if (strncmp(*v, "--txns-num=", 11) == 0) {
      size_t num;
      sscanf(*v, "--txns-num=%zd", &num);
      std::cout << "Requested to execute this number of txns: " << num << std::endl;   	
      if (num > 100*1000000) {
        std::cout << " the --txns-num argument must no greater than 100000000" << std::endl;
        exit(0);	
      }
      LIMIT = num;
    } else if (strncmp(*v, "--leaf-slots=", 13) == 0) {
      size_t leaf_num;
      sscanf(*v, "--leaf-slots=%zd", &leaf_num);
      std::cout << "Specified leaf-slot number: " << leaf_num << std::endl;   	
//      if ((leaf_num != 16) && (leaf_num != 32) && (leaf_num != 64) && (leaf_num != 128) && (leaf_num != 254) && (leaf_num != 256) && (leaf_num != 512) && (leaf_num != 1024)) {
//	     std::cout << " The leaf should be one of following: 16, 32, 64, 128, 254, 256, 512, 1024" << std::endl;
      if ((leaf_num != 16) && (leaf_num != 128)) {
        std::cout << " the --leaf-slots argument must be one of following: 16, 128" << std::endl;
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
    }
  }

  if(hyperthreading == true) {
    fprintf(stderr, "  Hyperthreading enabled\n");
  }

  if(insert_only == true) {
    fprintf(stderr, "  Insert-only mode\n");
  }

#ifdef USE_27MB_FILE
  fprintf(stderr, "  Using 27MB workload file\n");
#endif

  if(repeat_counter != 1) {
    fprintf(stderr, "  We run the workload part for %d times\n", repeat_counter);
  }

  fprintf(stderr, "index type = %d\n", index_type);

  if (kt == EMAIL_KEY) {
    fprintf(stderr, "\n\n\nERROR: EMAIL KEYS DO NOT WORK\n\n\n");
    exit(1);
  } else if (kt == UUID_KEY)
    runBenchmarks<uuidType, uuidCmp, uuidEq, uuidHash>(wl, kt, index_type, num_thread, repeat_counter);
  else if (kt == STRING_KEY)
    runBenchmarks<StrType, StrCmp, StrEq, StrHash>(wl, kt, index_type, num_thread, repeat_counter);
  return 0;
}
