
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <new>
#include "memusage.h"
#include "test_suite.h"

template <typename KeyType, typename ValueType>
class Benchmark {

    typedef Index<KeyType, ValueType> IndexTy;

    struct Entry {
        KeyType key;
        ValueType value;
    };


    // This is the array that represents the DB table
    Entry* array;
    KeyType* queryArray;

    void* allocFromOS(size_t size) {

	if ((size % 4096))
		size += 4096 - (size % 4096);

        void* p = mmap(0, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                       -1, 0);
        if (p == MAP_FAILED) exit(1);

        return p;
    }

public:
    Benchmark(const size_t keyNum, const bool randInit) {

        void* ptr = allocFromOS(keyNum * sizeof(Entry));
        array = new (ptr) Entry[keyNum];
        ptr = allocFromOS(keyNum * sizeof(KeyType));
        queryArray = new (ptr) KeyType[keyNum];

        printf("Initializing external data array of size = %f MB (key size=%zd key num=%zd)\n",
               sizeof(Entry) * keyNum / 1024.0 / 1024.0, sizeof(KeyType), keyNum);

        for (size_t i = 0; i < keyNum; i++) {
            memset(&array[i], 0, sizeof(array[i]));
        }

        if (randInit) {
            SimpleInt64Random<0, 0x100000000> r;
            for (size_t i = 0; i < sizeof(Entry) * keyNum / sizeof(uint64_t); i++)
                ((unsigned uint64_t *) array)[i] = r(i, (uint64_t) this);
        } else {
            for (size_t i = 0; i < keyNum; i++)
                memcpy(&array[i].key, &i, sizeof(i));
        }

        printf("Initializing query array of size = %f MB (key size=%zd key num=%zd)\n",
               sizeof(KeyType) * keyNum / 1024.0 / 1024.0, sizeof(KeyType), keyNum);

        for (size_t i = 0; i < keyNum; i++) {
            queryArray[i] = array[i].key;
        }
    }

    void seqInsert(IndexTy* index, const size_t key_num, const int num_thread, CacheMeter *cache) {

        // This is used to record time taken for each individual thread
        double thread_time[num_thread];
        for (int i = 0; i < num_thread; i++) {
            thread_time[i] = 0.0;
        }

        auto func = [key_num, &thread_time, this, cache, num_thread]
                    (uint64_t thread_id, IndexTy* index) {
            size_t start_key = key_num / num_thread * (long)thread_id;
            size_t end_key = start_key + key_num / num_thread;

            // Declare timer and start it immediately
            Timer timer {true};

            if (cache) cache->Start();

            for (size_t i = start_key; i < end_key; i+=1) {
//		    if (!(i % 200)) {
			    index->Insert(array[i].key, i);
//		    }
            }
//            for (size_t i = end_key; i > start_key; i--) {
//                index->Insert(array[i-1].key, i-1);
//            }

            //cache.Stop();
            double duration = timer.Stop();

            // Print L3 total accesses and cache misses
            if (cache) {
                cache->Stop();
                std::cout << num_thread << " Threads " << index->name() << ":"
                          << " " << cache->GetCounterName(0) << "/op: " << (double)cache->GetCounter(0) / (end_key-start_key)
                          << " " << cache->GetCounterName(1) << "/op: " << (double)cache->GetCounter(1) / (end_key-start_key)
                          << " " << cache->GetCounterName(2) << "/op: " << (double)cache->GetCounter(2) / (end_key-start_key)
                          << " " << cache->GetCounterName(3) << "/op: " << (double)cache->GetCounter(3) / (end_key-start_key)
                          << "\n";
            }

            thread_time[thread_id] = duration;
        };

        size_t mem_before = getMemUsage();
        LaunchParallelTestID(index, num_thread, func, index);
        size_t mem_after = getMemUsage();
	
        double elapsed_seconds = 0.0;
        for (int i = 0; i < num_thread; i++) {
            elapsed_seconds += thread_time[i];
        }

        std::cout << num_thread << " Threads " << index->name() << ": overall "
                  << (key_num / (1024.0 * 1024.0) * num_thread) / elapsed_seconds
                  << " million insert/sec" << "\n";
        std::cout << num_thread << " Threads " << index->name() << ": index size "
                  << (mem_after - mem_before) / (1024.0*1024.0) << " MB\n";
    }

    void seqRead(IndexTy* index, const size_t key_num, const int num_thread, CacheMeter* cache) {
        int iter = 1;

        // This is used to record time taken for each individual thread
        double thread_time[num_thread];
        for (int i = 0; i < num_thread; i++) {
            thread_time[i] = 0.0;
        }

        auto func = [key_num, iter, this, &thread_time, cache, num_thread]
                    (uint64_t thread_id, IndexTy* index) {
            std::vector<uint64_t> v {};
            KeyType k;

            v.reserve(1);

            Timer timer {true};
            if (cache)
                cache->Start();

            for (int j = 0; j < iter; j++) {
                size_t* i = (size_t *) &k;
                for (*i = 0; *i < key_num; (*i)++) {
                    index->GetValue(k, v);
                    // std::cerr << "Calling GetValue with: " << (&k) << " " << (&v) << std::endl;
                    // if (!(array[v[0]].key == k)) throw 1;
                    v.clear();
                }
            }

            double duration = timer.Stop();

            if (cache) {
                cache->Stop();
                std::cout << num_thread << " Threads " << index->name() << ":"
                          << " " << cache->GetCounterName(0) << "/op: " << (double)cache->GetCounter(0) / key_num
                          << " " << cache->GetCounterName(1) << "/op: " << (double)cache->GetCounter(1) / key_num
                          << " " << cache->GetCounterName(2) << "/op: " << (double)cache->GetCounter(2) / key_num
                          << " " << cache->GetCounterName(3) << "/op: " << (double)cache->GetCounter(3) / key_num
                          << "\n";
            }

            thread_time[thread_id] = duration;

            #if 0
            std::cout << "[Thread " << thread_id << " Done] @ " \
                      << (iter * key_num / (1024.0 * 1024.0)) / duration \
                      << " million read/sec" << "\n";
            #endif
        };

        LaunchParallelTestID(index, num_thread, func, index);

        double elapsed_seconds = 0.0;
        for (int i = 0; i < num_thread; i++) {
            elapsed_seconds += thread_time[i];
        }

        std::cout << num_thread << " Threads " << index->name() << ": overall "
                  << (iter * key_num / (1024.0 * 1024.0) * num_thread * num_thread) / elapsed_seconds
                  << " million read/sec" << "\n";
    }

    void randRead(IndexTy* index, const size_t key_num, const int num_thread, CacheMeter *cache) {
        int iter = 1;

        // This is used to record time taken for each individual thread
        double thread_time[num_thread];
        for (int i = 0; i < num_thread; i++) {
            thread_time[i] = 0.0;
        }

        auto func2 = [key_num, iter, this, &thread_time, cache, num_thread]
                     (uint64_t thread_id, IndexTy* index) {
            std::vector<uint64_t> v {};

            v.reserve(1);

            // This is the random number generator we use
            SimpleInt64Random<0, 0x100000000> h {};

            Timer timer {true};

            if (cache)
                cache->Start();

            for (int j = 0; j < iter; j++) {
                for (size_t i = 0; i < key_num; i+=1) {
//		    if (!(i % 201)) {
			long int idx = (long int)h((uint64_t)i, thread_id) % key_num;
			index->GetValue(queryArray[idx], v);
			v.clear();
//		    }	
                }
            }

            //cache.Stop();
            double duration = timer.Stop();

            if (cache) {
                cache->Stop();
                std::cout << num_thread << " Threads " << index->name() << ":"
                          << " " << cache->GetCounterName(0) << "/op: " << (double)cache->GetCounter(0) / key_num
                          << " " << cache->GetCounterName(1) << "/op: " << (double)cache->GetCounter(1) / key_num
                          << " " << cache->GetCounterName(2) << "/op: " << (double)cache->GetCounter(2) / key_num
                          << " " << cache->GetCounterName(3) << "/op: " << (double)cache->GetCounter(3) / key_num
                          << "\n";
            }

            thread_time[thread_id] = duration;

            #if 0
            std::cout << "[Thread " << thread_id << " Done] @ " \
                      << (iter * key_num / (1024.0 * 1024.0)) / duration \
                      << " million read (random)/sec" << "\n";
            #endif
        };

        LaunchParallelTestID(index, num_thread, func2, index);

        double elapsed_seconds = 0.0;
        for (int i = 0; i < num_thread; i++) {
            elapsed_seconds += thread_time[i];
        }

        std::cout << num_thread << " Threads " << index->name() << ": overall "
                  << (iter * key_num / (1024.0 * 1024.0) * num_thread * num_thread) / elapsed_seconds
                  << " million read (random)/sec" << "\n";

        return;
    }


    void zipfRead(IndexTy* index, const size_t key_num, const int num_thread) {
        int iter = 1;

        // This is used to record time taken for each individual thread
        double thread_time[num_thread];
        for (int i = 0; i < num_thread; i++) {
            thread_time[i] = 0.0;
        }

        // Generate zipfian distribution into this list
        std::vector<long> zipfian_key_list {};
        zipfian_key_list.reserve(key_num);

        // Initialize it with time() as the random seed
        Zipfian zipf {(uint64_t)key_num, 0.99, (uint64_t)time(NULL)};

        // Populate the array with random numbers
        for (size_t i = 0; i < key_num; i++) {
            zipfian_key_list.push_back(zipf.Get());
        }

        auto func2 = [key_num, iter, &thread_time, &zipfian_key_list, this, num_thread]
                     (uint64_t thread_id, IndexTy* index) {
            // This is the start and end index we read into the zipfian array
            size_t start_index = key_num / num_thread * (long)thread_id;
            size_t end_index = start_index + key_num / num_thread;

            std::vector<uint64_t> v {};

            v.reserve(1);

            Timer timer {true};
            //CacheMeter cache{true};

            for (int j = 0; j < iter; j++) {
                for (size_t i = start_index; i < end_index; i+=1) {
//		    if (!(i % 201)) {
			long int idx = zipfian_key_list[i];
			index->GetValue(queryArray[idx], v);
			v.clear();
//		    }	
                }
            }

            //cache.Stop();
            double duration = timer.Stop();

            thread_time[thread_id] = duration;

            #if 0
            std::cout << "[Thread " << thread_id << " Done] @ " \
                      << (iter * (end_index - start_index) / (1024.0 * 1024.0)) / duration \
                      << " million read (zipfian)/sec" << "\n";
            #endif

            //cache.PrintL3CacheUtilization();
            //cache.PrintL1CacheUtilization();
        };

        LaunchParallelTestID(index, num_thread, func2, index);

        double elapsed_seconds = 0.0;
        for (int i = 0; i < num_thread; i++) {
            elapsed_seconds += thread_time[i];
        }

        std::cout << num_thread << " Threads " << index->name() << ": overall "
                  << (iter * key_num / (1024.0 * 1024.0)) / (elapsed_seconds / num_thread)
                  << " million read (zipfian)/sec" << "\n";
    }

};

