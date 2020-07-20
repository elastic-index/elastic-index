
/*
 * test_suite.cpp
 *
 * This files includes basic testing infrastructure
 *
 * by Ziqi Wang
 */

#include "test_suite.h"

/*
 * PinToCore() - Pin the current calling thread to a particular core
 */
void PinToCore(size_t core_id) {
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(core_id, &cpu_set);

  int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set);

  printf("pthread_setaffinity_np() returns %d\n", ret);

  return;
}
