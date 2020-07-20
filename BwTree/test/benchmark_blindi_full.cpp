
/*
 * benchmark_btree_full.cpp - This file contains test suites for command
 *                            benchmark-btree-full
 *
 * Requires stx::blindi_btree_multimap and spinlock in ./benchmark directory
 */

#include "test_suite.h"
#include "../benchmark/spinlock/spinlock.h"

// Only use boost for full-speed build
#ifdef NDEBUG 
// For boost reference:
//   http://www.boost.org/doc/libs/1_58_0/doc/html/thread/synchronization.html#thread.synchronization.mutex_types.shared_mutex
#define USE_BOOST
#pragma message "Using boost::thread for shared_mutex"
#endif

#ifdef USE_BOOST
#include <boost/thread/shared_mutex.hpp>
using namespace boost;
#endif

void BenchmarkBlindiBTreeRandInsert(BlindiBTreeTrieType *t, int key_num, int num_thread, long int *array) {

  // This is used to record time taken for each individual thread
  double thread_time[num_thread];
  for(int i = 0;i < num_thread;i++) {
    thread_time[i] = 0.0;
  }

  // This generates a permutation on [0, key_num)
  Permutation<long long int> perm{(size_t)key_num, 0};

  #ifndef USE_BOOST
  // Declear a spinlock protecting the data structure
  spinlock_t lock;
  rwlock_init(lock);
  #else
  shared_mutex lock;
  #endif

  auto func = [key_num,
               &thread_time,
               array,
               num_thread,
               &lock,
               &perm](uint64_t thread_id, BlindiBTreeTrieType *t) {
    long int start_key = key_num / num_thread * (long)thread_id;
    long int end_key = start_key + key_num / num_thread;

    // Declare timer and start it immediately
    Timer timer{true};

    #ifndef USE_BOOST
    write_lock(lock);
    #else
    lock.lock();
    #endif

    for(int i = start_key;i < end_key;i++) {
      t->insert(array[perm[i]], array[perm[i]]);
    }

    #ifndef USE_BOOST
    write_unlock(lock);
    #else
    lock.unlock();
    #endif

    double duration = timer.Stop();

    thread_time[thread_id] = duration;

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << (key_num / num_thread) / (1024.0 * 1024.0) / duration \
              << " million random insert/sec" << "\n";

    return;
  };

  LaunchParallelTestID(nullptr, num_thread, func, t);

  double elapsed_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    elapsed_seconds += thread_time[i];
  }

  std::cout << num_thread << " Threads BlindiBTree Multimap: overall "
            << (key_num / (1024.0 * 1024.0) * num_thread) / elapsed_seconds
            << " million random insert/sec" << "\n";
}

/*
 * BenchmarkBwTreeSeqInsert() - As name suggests
 */
void BenchmarkBlindiBTreeSeqInsert(BlindiBTreeTrieType *t, 
                                   int key_num, 
                                   int num_thread,
                                   long int *array) {

  // This is used to record time taken for each individual thread
  double thread_time[num_thread];
  for(int i = 0;i < num_thread;i++) {
    thread_time[i] = 0.0;
  }
  
  #ifndef USE_BOOST
  // Declear a spinlock protecting the data structure
  spinlock_t lock;
  rwlock_init(lock);
  #else
  shared_mutex lock;
  #endif

  auto func = [key_num, 
               &thread_time, 
               array,
               num_thread,
               &lock](uint64_t thread_id, BlindiBTreeTrieType *t) {
    long int start_key = key_num / num_thread * (long)thread_id;
    long int end_key = start_key + key_num / num_thread;

    // Declare timer and start it immediately
    Timer timer{true};

    #ifndef USE_BOOST
    write_lock(lock);
    #else
    lock.lock();
    #endif

    for(long int i = start_key;i < end_key;i++) {
      t->insert(array[i], array[i]);
    }

    #ifndef USE_BOOST
    write_unlock(lock);
    #else
    lock.unlock();
    #endif

    double duration = timer.Stop();

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << (key_num / num_thread) / (1024.0 * 1024.0) / duration \
              << " million insert/sec" << "\n";

    thread_time[thread_id] = duration;

    return;
  };

  LaunchParallelTestID(nullptr, num_thread, func, t);

  double elapsed_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    elapsed_seconds += thread_time[i];
  }

  std::cout << num_thread << " Threads BlindiBTree Multimap: overall "
            << (key_num / (1024.0 * 1024.0) * num_thread) / elapsed_seconds
            << " million insert/sec" << "\n";
            
  return;
}

/*
 * BenchmarkBlindiBTreeSeqRead() - As name suggests
 */
void BenchmarkBlindiBTreeSeqRead(BlindiBTreeTrieType *t, 
                                 int key_num,
                                 int num_thread) {
  int iter = 1;
  
  // This is used to record time taken for each individual thread
  double thread_time[num_thread];
  for(int i = 0;i < num_thread;i++) {
    thread_time[i] = 0.0;
  }
  
  #ifndef USE_BOOST
  // Declear a spinlock protecting the data structure
  spinlock_t lock;
  rwlock_init(lock);
  #else
  shared_mutex lock;
  #endif
  
  auto func = [key_num, 
               iter, 
               &thread_time, 
               &lock,
               num_thread](uint64_t thread_id, BlindiBTreeTrieType *t) {
    std::vector<long> v{};

    v.reserve(1);

    Timer timer{true};

    #ifndef USE_BOOST
    read_lock(lock);
    #else
    lock.lock_shared();
    #endif

    for(int j = 0;j < iter;j++) {
      for(long int i = 0;i < key_num;i++) {
        t->GetValue(i, v);
        v.clear();
      }
    }

    #ifndef USE_BOOST
    read_unlock(lock);
    #else
    lock.unlock_shared();
    #endif

    double duration = timer.Stop();

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << (iter * key_num / (1024.0 * 1024.0)) / duration \
              << " million read/sec" << "\n";
              
    thread_time[thread_id] = duration;

    return;
  };

  LaunchParallelTestID(nullptr, num_thread, func, t);
  
  double elapsed_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    elapsed_seconds += thread_time[i];
  }

  std::cout << num_thread << " Threads BlindiBTree Multimap: overall "
            << (iter * key_num / (1024.0 * 1024.0) * num_thread * num_thread) / elapsed_seconds
            << " million read/sec" << "\n";

  return;
}

/*
 * BenchmarkBlindiBTreeRandRead() - As name suggests
 */
void BenchmarkBlindiBTreeRandRead(BlindiBTreeTrieType *t, 
                                  int key_num,
                                  int num_thread) {
  int iter = 1;
  
  // This is used to record time taken for each individual thread
  double thread_time[num_thread];
  for(int i = 0;i < num_thread;i++) {
    thread_time[i] = 0.0;
  }
  
  #ifndef USE_BOOST
  // Declear a spinlock protecting the data structure
  spinlock_t lock;
  rwlock_init(lock);
  #else
  shared_mutex lock;
  #endif
  
  auto func2 = [key_num, 
                iter, 
                &thread_time,
                &lock,
                num_thread](uint64_t thread_id, BlindiBTreeTrieType *t) {
    std::vector<long> v{};

    v.reserve(1);
    
    // This is the random number generator we use
    SimpleInt64Random<0, 30 * 1024 * 1024> h{};

    Timer timer{true};

    #ifndef USE_BOOST
    read_lock(lock);
    #else
    lock.lock_shared();
    #endif

    for(int j = 0;j < iter;j++) {
      for(long int i = 0;i < key_num;i++) {
        //int key = uniform_dist(e1);
        long int key = (long int)h((uint64_t)i, thread_id);

        t->GetValue(key, v);
        v.clear();
      }
    }

    #ifndef USE_BOOST
    read_unlock(lock);
    #else
    lock.unlock_shared();
    #endif

    double duration = timer.Stop();

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << (iter * key_num / (1024.0 * 1024.0)) / duration \
              << " million read (random)/sec" << "\n";
              
    thread_time[thread_id] = duration;

    return;
  };

  LaunchParallelTestID(nullptr, num_thread, func2, t);

  double elapsed_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    elapsed_seconds += thread_time[i];
  }

  std::cout << num_thread << " Threads BlindiBTree Multimap: overall "
            << (iter * key_num / (1024.0 * 1024.0) * num_thread * num_thread) / elapsed_seconds
            << " million read (random)/sec" << "\n";

  return;
}

/*
 * BenchmarkBlindiBTreeRandLocklessRead() - As name suggests
 */
void BenchmarkBlindiBTreeRandLocklessRead(BlindiBTreeTrieType *t, 
                                          int key_num,
                                          int num_thread) {
  int iter = 1;
  
  // This is used to record time taken for each individual thread
  double thread_time[num_thread];
  for(int i = 0;i < num_thread;i++) {
    thread_time[i] = 0.0;
  }
  
  auto func2 = [key_num, 
                iter, 
                &thread_time,
                num_thread](uint64_t thread_id, BlindiBTreeTrieType *t) {
    std::vector<long> v{};

    v.reserve(1);
    
    // This is the random number generator we use
    SimpleInt64Random<0, 30 * 1024 * 1024> h{};

    Timer timer{true};

    for(int j = 0;j < iter;j++) {
      for(long int i = 0;i < key_num;i++) {
        //int key = uniform_dist(e1);
        long int key = (long int)h((uint64_t)i, thread_id);
        
        t->GetValue(key, v);
        v.clear();
      }
    }

    double duration = timer.Stop();

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << (iter * key_num / (1024.0 * 1024.0)) / duration \
              << " million read (random)/sec" << "\n";
              
    thread_time[thread_id] = duration;

    return;
  };

  LaunchParallelTestID(nullptr, num_thread, func2, t);

  double elapsed_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    elapsed_seconds += thread_time[i];
  }

  std::cout << num_thread << " Threads BlindiBTree Multimap: overall "
            << (iter * key_num / (1024.0 * 1024.0) * num_thread * num_thread) / elapsed_seconds
            << " million read (random, lockless)/sec" << "\n";

  return;
}



/*
 * BenchmarkBlindiBTreeZipfRead() - As name suggests
 */
void BenchmarkBlindiBTreeZipfRead(BlindiBTreeTrieType *t, 
                                  int key_num,
                                  int num_thread) {
  int iter = 1;
  
  // This is used to record time taken for each individual thread
  double thread_time[num_thread];
  for(int i = 0;i < num_thread;i++) {
    thread_time[i] = 0.0;
  }
  
  #ifndef USE_BOOST
  // Declear a spinlock protecting the data structure
  spinlock_t lock;
  rwlock_init(lock);
  #else
  shared_mutex lock;
  #endif
  
  // Generate zipfian distribution into this list
  std::vector<long> zipfian_key_list{};
  zipfian_key_list.reserve(key_num);
  
  // Initialize it with time() as the random seed
  Zipfian zipf{(uint64_t)key_num, 0.99, (uint64_t)time(NULL)};
  
  // Populate the array with random numbers 
  for(int i = 0;i < key_num;i++) {
    zipfian_key_list.push_back(zipf.Get()); 
  }
  
  auto func2 = [key_num, 
                iter, 
                &thread_time,
                &lock,
                &zipfian_key_list,
                num_thread](uint64_t thread_id, BlindiBTreeTrieType *t) {
    // This is the start and end index we read into the zipfian array
    long int start_index = key_num / num_thread * (long)thread_id;
    long int end_index = start_index + key_num / num_thread;
    
    std::vector<long> v{};

    v.reserve(1);

    Timer timer{true};

    #ifndef USE_BOOST
    read_lock(lock);
    #else
    lock.lock_shared();
    #endif

    for(int j = 0;j < iter;j++) {
      for(long int i = start_index;i < end_index;i++) {
        long int key = zipfian_key_list[i];
        
        t->GetValue(key, v);
        v.clear();
      }
    }

    #ifndef USE_BOOST
    read_unlock(lock);
    #else
    lock.unlock_shared();
    #endif

    double duration = timer.Stop();

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << (iter * (end_index - start_index) / (1024.0 * 1024.0)) / duration \
              << " million read (zipfian)/sec" << "\n";
              
    thread_time[thread_id] = duration;

    return;
  };

  LaunchParallelTestID(nullptr, num_thread, func2, t);

  double elapsed_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    elapsed_seconds += thread_time[i];
  }

  std::cout << num_thread << " Threads BlindiBTree Multimap: overall "
            << (iter * key_num / (1024.0 * 1024.0)) / (elapsed_seconds / num_thread)
            << " million read (zipfian)/sec" << "\n";

  return;
}

/*
 * BenchmarkBlindiBTreeZipfLockLessRead() - As name suggests
 *
 * In this benchmark we just let it fire without any lock
 */
void BenchmarkBlindiBTreeZipfLockLessRead(BlindiBTreeTrieType *t, 
                                          int key_num,
                                          int num_thread) {
  int iter = 1;
  double thread_time[num_thread];
  for(int i = 0;i < num_thread;i++) {
    thread_time[i] = 0.0;
  }
  std::vector<long> zipfian_key_list{};
  zipfian_key_list.reserve(key_num);
  Zipfian zipf{(uint64_t)key_num, 0.99, (uint64_t)time(NULL)};
  for(int i = 0;i < key_num;i++) {
    zipfian_key_list.push_back(zipf.Get()); 
  }
  
  auto func2 = [key_num, 
                iter, 
                &thread_time,
                &zipfian_key_list,
                num_thread](uint64_t thread_id, BlindiBTreeTrieType *t) {
    long int start_index = key_num / num_thread * (long)thread_id;
    long int end_index = start_index + key_num / num_thread;
    
    std::vector<long> v{};

    v.reserve(1);

    Timer timer{true};

    for(int j = 0;j < iter;j++) {
      for(long int i = start_index;i < end_index;i++) {
        long int key = zipfian_key_list[i];
        t->GetValue(key, v);
        v.clear();
      }
    }

    double duration = timer.Stop();

    std::cout << "[Thread " << thread_id << " Done] @ " \
              << (iter * (end_index - start_index) / (1024.0 * 1024.0)) / duration \
              << " million read (zipfian, lockless)/sec" << "\n";
              
    thread_time[thread_id] = duration;

    return;
  };

  LaunchParallelTestID(nullptr, num_thread, func2, t);

  double elapsed_seconds = 0.0;
  for(int i = 0;i < num_thread;i++) {
    elapsed_seconds += thread_time[i];
  }

  std::cout << num_thread << " Threads BlindiBTree Multimap: overall "
            << (iter * key_num / (1024.0 * 1024.0)) / (elapsed_seconds / num_thread)
            << " million read (zipfian, lockless)/sec" << "\n";

  return;
}
