/*
   eXokernel Development Kit (XDK)

   Samsung Research America Copyright (C) 2013
   IBM Corporation (C) 2020

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.

   As a special exception, if you link the code in this file with
   files compiled with a GNU compiler to produce an executable,
   that does not cause the resulting executable to be covered by
   the GNU Lesser General Public License.  This exception does not
   however invalidate any other reasons why the executable file
   might be covered by the GNU Lesser General Public License.
   This exception applies to code released by its copyright holders
   in files containing the exception.
*/

/*
  Authors:
  Daniel G. Waddington <daniel.waddington@acm.org>
*/

#include <common/assert.h>
#include <common/logging.h>
#include <common/utils.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <numa.h>
#include <stdarg.h>
#include <stdio.h>

extern "C" void panic(const char *fmt, ...) {
  printf("\033[31m Panic: ");
  va_list list;
  va_start(list, fmt);
  vprintf(fmt, list);
  va_end(list);
  printf("\033[0m\n");

#if defined(__i386__) || defined(__x86_64__)
  asm("int3");
#else
  __builtin_trap();
#endif
}

/**
 * Touch memory at huge page strides
 *
 * @param addr Pointer to memory to touch
 * @param size Size of memory in bytes
 */
void touch_huge_pages(void *addr, size_t size) {
  SUPPRESS_NOT_USED_WARN volatile byte b;

  // Touch memory to trigger page mapping.
  for (volatile byte *p = static_cast<byte *>(addr);
       p < (static_cast<const byte *>(addr) + size);
       p += HUGE_PAGE_SIZE) {
    b = *p;  // R touch.
  }
}

/**
 * Touch memory at 4K page strides
 *
 * @param addr Pointer to memory to touch
 * @param size Size of memory in bytes
 */
void touch_pages(void *addr, size_t size) {
  SUPPRESS_NOT_USED_WARN volatile byte b;

  // Touch memory to trigger page mapping.
  for (volatile byte *p = static_cast<byte *>(addr);
       p < (static_cast<byte *>(addr) + size); p += PAGE_SIZE) {
    b = *p;  // R touch.
  }
}

#if defined(__linux__)
Cpu_bitset get_actual_affinities(const Cpu_bitset &logical_affinities,
                                 const int numa_node) {
  assert(numa_node >= 0 && numa_node < numa_num_configured_nodes());

  struct bitmask *node_cpumask = numa_allocate_cpumask();

  if (numa_node_to_cpus(numa_node, node_cpumask) < 0) {
    panic("numa_node_to_cpus() failed!");
  }

  // TODO: Use numa_bitmask_weight(...) from latest version of libnuma.
  size_t bitcnt = 0;
  size_t bitmask_size =
      8 * numa_bitmask_nbytes(node_cpumask);  // numa_num_possible_cpus();
  for (unsigned b = 0; b < bitmask_size; b++) {
    if (numa_bitmask_isbitset(node_cpumask, unsigned(b))) {
      bitcnt++;
    }
  }
  // PLOG("bitcnt = %lu", bitcnt);

  size_t la_bitcnt = logical_affinities.count();
  // PLOG("la_bitcnt = %lu", la_bitcnt);
  assert(bitcnt >= la_bitcnt);

  Cpu_bitset cpu_bitset;

  size_t cnt = 0;
  size_t l = 0;
  size_t a = 0;
  size_t c = 0;
  while (cnt < la_bitcnt) {
    for (; l < logical_affinities.size(); l++) {
      if (logical_affinities.test(l)) {
        break;
      }
    }

    if (l >= logical_affinities.size()) {
      break;
    }

    for (; a < bitmask_size; a++) {
      if (numa_bitmask_isbitset(node_cpumask, unsigned(a))) {
        if (l == c++) {
          cpu_bitset.set(a);
          cnt++;
          a++;
          l++;
          break;
        }
      }
    }

    if (a >= bitmask_size) {
      break;
    }
  }

  numa_free_cpumask(node_cpumask);

  return cpu_bitset;
}

bool check_ptr_valid(void *pointer, size_t len) {
  // int nullfd = open("/dev/random", O_WRONLY);
  // int rc = write(nullfd, pointer, len);
  // close(nullfd);
  // return (rc >= 0);
  char *p = static_cast<char *>(pointer);
  int total = 0;
  while (len > 0) {
    char x = *p;
    total += x;
    p++;
    len--;
  }
  return true;
}


namespace common
{

std::string get_DRAM_usage()
{
  using namespace std;

  ifstream stat_stream("/proc/self/stat",ios_base::in);

  string pid, comm, state, ppid, pgrp, session, tty_nr;
  string tpgid, flags, minflt, cminflt, majflt, cmajflt;
  string utime, stime, cutime, cstime, priority, nice, threads;
  string O, itrealvalue, starttime;

  unsigned long vsize;
  long rss;

  stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
              >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
              >> utime >> stime >> cutime >> cstime >> priority >> nice
              >> threads >> itrealvalue >> starttime >> vsize >> rss;

  stat_stream.close();

  long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
  auto resident_set = rss * page_size_kb;

  std::stringstream result;
  result << "RSS:" << resident_set/1024 << "MiB, VM:" << REDUCE_MiB(vsize) << "MiB";
  return result.str();
}

}


#endif  // __linux__
