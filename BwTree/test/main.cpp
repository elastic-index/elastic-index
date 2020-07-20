
#include "test_suite.h"
#include "benchmark_full.h"

#ifndef LEAF_NUM
#error "Define LEAF_NUM"
#endif

#ifndef INNER_NUM
#error "Define INNER_NUM"
#endif

/*
 * GetThreadNum() - Returns the number of threads used for multithreaded testing
 *
 * By default 40 threads are used
 */
uint64_t GetThreadNum() {
    uint64_t thread_num = 1;
    bool ret = Envp::GetValueAsUL("THREAD_NUM", &thread_num);
    if(ret == false) {
      throw "THREAD_NUM must be an unsigned ineteger!";
    } else {
      printf("Using thread_num = %lu\n", thread_num);
    }

    return thread_num;
}

constexpr int node_slot (int n)
{
	switch (n) {
		case 16:
			return 16;
		default:
			return 16;
	} 
}

enum IndexType {
  ART,
  HOT,
  BWTREE,
  BTREE,
  BLINDI_TRIE,
  BLINDI_SEQTREE,
  BLINDI_SUBTRIE,
  BLINDI_BTREE_HYBRID_NODES,
  SKIP_LIST_LOCK_FREE,
  SKIP_LIST_SINGLE_LOCK,
  SKIP_LIST_LAZY,
  SKIP_LIST_SEQ,
  BTREEOLC,
  BTREEOLC_BLINDI_SEQ,
  BTREEOLC_BLINDI_TRIE,
  BTREEOLC_BLINDI_SUBTRIE
};

enum KeyType {
  LONG_SEQ,
  LONG_RAND,
  UUID_RAND,
  STRING_RAND
};

template<typename KeyType>
Index<KeyType, uint64_t>* makeBTreeIndexes(IndexType idx_type) {
  switch (idx_type) {
    case BLINDI_BTREE_HYBRID_NODES:
      return new BlindiBTreeHybridNodes<KeyType, uint64_t, SeqTreeBlindiNode, INNER_NUM, LEAF_NUM>("SeqTreeBlindi");
    case BLINDI_SEQTREE:
      return new BlindiBTreeSeqTreeType<KeyType, uint64_t, SeqTreeBlindiNode, INNER_NUM, LEAF_NUM>("SeqTreeBlindi");
    case BLINDI_SUBTRIE:
      return new BlindiBTreeSubTrieType<KeyType, uint64_t, SubTrieBlindiNode, INNER_NUM, LEAF_NUM>("SubTrieBlindi");
    case BLINDI_TRIE:
      return new BlindiBTreeTrieType<KeyType, uint64_t, TrieBlindiNode, INNER_NUM, LEAF_NUM>("TrieBlindi");
    case BTREE:
      return new BTreeType<KeyType, uint64_t, INNER_NUM, LEAF_NUM>;
    case BTREEOLC:
      return new BlindiBTreeOLCType<KeyType, uint64_t, INNER_NUM, LEAF_NUM>();
    case BTREEOLC_BLINDI_SEQ:
      return new BlindiSubtrieBTreeOLCType<KeyType, uint64_t, INNER_NUM, LEAF_NUM>();
    case BTREEOLC_BLINDI_TRIE:
      return new BlindiTrieBTreeOLCType<KeyType, uint64_t, INNER_NUM, LEAF_NUM>();
    default : {
       std::cout << "BTREE_INDEXES" << " inner_num " << INNER_NUM <<  " leaf_num " << LEAF_NUM << std::endl;
       std::cout << " no support in inner or leaf node" << std::endl;
       exit(1);
    }
  }
}

template<typename KeyType>
Index<KeyType, uint64_t>* makeIndex(IndexType idx_type) {
  std::cout << "idx_type " <<  idx_type << " inner_num " << INNER_NUM <<  " leaf_num " << LEAF_NUM << std::endl;
  switch (idx_type) {
    case ART:
      return new ARTType<KeyType, uint64_t>();
    case HOT:
      return new HOTType<KeyType, uint64_t>();
    case BWTREE:
      return new BwTreeType<KeyType, uint64_t>();
    case BTREE:
    case BLINDI_BTREE_HYBRID_NODES:
    case BLINDI_TRIE:
    case BLINDI_SEQTREE:
    case BLINDI_SUBTRIE:
    case BTREEOLC:
    case BTREEOLC_BLINDI_SEQ:
    case BTREEOLC_BLINDI_TRIE:
    case BTREEOLC_BLINDI_SUBTRIE:
	return makeBTreeIndexes<KeyType>(idx_type);
    case SKIP_LIST_LOCK_FREE:
      return new LockFreeSkipListType<KeyType, uint64_t>();
    case SKIP_LIST_LAZY:
      return new LazySkipListType<KeyType, uint64_t>();
    case SKIP_LIST_SINGLE_LOCK:
      return new SingleLockSkipListType<KeyType, uint64_t>();
    case SKIP_LIST_SEQ:
      return new SeqSkipListType<KeyType, uint64_t>();
    default:
      return 0;
  }
}

template<typename KeyType>
void runBenchmark(IndexType idx_type, bool randInit, size_t key_num, int thread_num, CacheMeter *cache) {
  Index<KeyType, uint64_t> *idx = makeIndex<KeyType>(idx_type);
  Benchmark<KeyType, uint64_t> b(key_num, randInit);
  b.seqInsert(idx, key_num, (int)thread_num, cache);
//  b.seqRead(idx, key_num, (int)thread_num, cache);
  b.randRead(idx, key_num, (int)thread_num, cache);
//  b.zipfRead(idx, key_num, (int)thread_num);
	idx->PrintStats();
}

int main(int argc, char **argv) {
  IndexType idx_type = ART;
  KeyType key_type = LONG_SEQ;
  size_t key_num = 30 * 1024 * 1024;
  CacheMeter *cache = NULL;

  int opt_index = 1;
  while(opt_index < argc) {
    char *opt_p = argv[opt_index];

    if (strcmp(opt_p, "--index=art") == 0) {
      idx_type = ART;
   } else if(strcmp(opt_p, "--index=hot") == 0) {
      idx_type = HOT;
    } else if(strcmp(opt_p, "--index=bwtree") == 0) {
      idx_type = BWTREE;
    } else if(strcmp(opt_p, "--index=btree") == 0) {
      idx_type = BTREE;
    } else if(strcmp(opt_p, "--index=blindi_btree_hybrid_nodes") == 0) {
      idx_type = BLINDI_BTREE_HYBRID_NODES;
    } else if(strcmp(opt_p, "--index=blindi_trie") == 0) {
      idx_type = BLINDI_TRIE;
    } else if(strcmp(opt_p, "--index=blindi_seqtree") == 0) {
      idx_type = BLINDI_SEQTREE;
    } else if(strcmp(opt_p, "--index=blindi_subtrie") == 0) {
      idx_type = BLINDI_SUBTRIE;
    } else if(strcmp(opt_p, "--index=skip_list_seq") == 0) {
      idx_type = SKIP_LIST_SEQ;
    } else if(strcmp(opt_p, "--index=skip_list_single_lock") == 0) {
      idx_type = SKIP_LIST_SINGLE_LOCK;
    } else if(strcmp(opt_p, "--index=skip_list_lazy") == 0) {
      idx_type = SKIP_LIST_LAZY;
    } else if(strcmp(opt_p, "--index=skip_list_lock_free") == 0) {
      idx_type = SKIP_LIST_LOCK_FREE;
    } else if(strcmp(opt_p, "--index=skip_list") == 0) {
      idx_type = SKIP_LIST_LOCK_FREE;
    } else if(strcmp(opt_p, "--index=btreeolc") == 0) {
      idx_type = BTREEOLC;
    // } else if(strcmp(opt_p, "--index=blindi_btreeolc_16_16") == 0) {
    //   idx_type = BLINDI_BTREEOLC_16_16;
    // } else if(strcmp(opt_p, "--index=blindi_btreeolc_32_32") == 0) {
    //   idx_type = BLINDI_BTREEOLC_32_32;
    // } else if(strcmp(opt_p, "--index=blindi_BTreeOLC_64_64") == 0) {
    //   idx_type = BLINDI_BTREEOLC_64_64;
    // } else if(strcmp(opt_p, "--index=blindi_BTreeOLC_128_128") == 0) {
    //   idx_type = BLINDI_BTREEOLC_128_128;
    } else if(strcmp(opt_p, "--index=btreeolc_blindi_seqtree") == 0) {
      idx_type = BTREEOLC_BLINDI_SEQ;
    } else if(strcmp(opt_p, "--index=btreeolc_blindi_trie") == 0) {
      idx_type = BTREEOLC_BLINDI_TRIE;
    } else if(strcmp(opt_p, "--index=btreeolc_blindi_subtrie") == 0) {
      idx_type = BTREEOLC_BLINDI_SUBTRIE;
    } else if(strcmp(opt_p, "--key=long-seq") == 0) {
      key_type = LONG_SEQ;
    } else if(strcmp(opt_p, "--key=long-rand") == 0) {
      key_type = LONG_RAND;
    } else if(strcmp(opt_p, "--key=uuid") == 0) {
      key_type = UUID_RAND;
    } else if(strcmp(opt_p, "--key=string") == 0) {
      key_type = STRING_RAND;
    } else if(strncmp(opt_p, "--key_num=", 10) ==  0) {
      char t;
      sscanf(opt_p, "--key_num=%zd%c", &key_num, &t);
      switch (t) {
        case 'g':
        case 'G':
          key_num *= 1024;
        case 'm':
        case 'M':
          key_num *= 1024;
        case 'k':
        case 'K':
          key_num *= 1024;
        case 'D':
        case 'd':
          key_num *= 1;

          break;
      }
      assert(key_num < 0x100000000);
    } else if(strncmp(opt_p, "--perf=", 7) == 0) {
      cache = new CacheMeter{false, &opt_p[7]};
    } else {
      printf("ERROR: Unknown option: %s\n", opt_p);
      return 0;
    }

    opt_index++;
  }

  //////////////////////////////////////////////////////
  // Next start running test cases
  //////////////////////////////////////////////////////

  printf("Using key size = %ld (%f million)\n",
         key_num,
         key_num / (1024.0 * 1024.0));

  uint64_t thread_num = GetThreadNum();

  switch (key_type) {
    case LONG_SEQ:
      runBenchmark<uint64_t>(idx_type, false, key_num, thread_num, cache);
      break;
    case LONG_RAND:
      runBenchmark<uint64_t>(idx_type, true, key_num, thread_num, cache);
      break;
    case UUID_RAND:
      runBenchmark<UUID>(idx_type, true, key_num, thread_num, cache);
      break;
    case STRING_RAND:
      runBenchmark<String>(idx_type, true, key_num, thread_num, cache);
  }

  return 0;
}
