#ifndef __EXP_THROUGHPUT_H__
#define __EXP_THROUGHPUT_H__

#include "experiment.h"

#include "statistics.h"
#include "stopwatch.h"

#include <chrono>
#include <cstdlib>
#include <random>
#include <mutex>

class ExperimentThroughput : public Experiment
{
  struct op_count
  {
    const char *name;
    unsigned long total;
    unsigned long interval;
    op_count &operator++() { ++total; ++interval; return *this; }
  };
  pool_entry_offset_t _i_rd;
  pool_entry_offset_t _i_wr;
  std::vector<bool> _populated;
  std::chrono::high_resolution_clock::time_point _start_time;
  std::chrono::high_resolution_clock::time_point _report_time;
  std::chrono::high_resolution_clock::duration _report_interval;
  std::default_random_engine _rand_engine;
  std::uniform_int_distribution<unsigned> _rand_pct;
  unsigned _ga_pct;
  unsigned _rd_pct;
  unsigned _ie_pct;
  op_count _op_count_ga;
  op_count _op_count_rd;
  op_count _op_count_in;
  op_count _op_count_up;
  op_count _op_count_er;
  static unsigned long _iops;
  static std::mutex _iops_lock;
  Stopwatch _sw_ga;
  Stopwatch _sw_rd;
  Stopwatch _sw_wr;
  bool _continuous;
  std::string _hostname;
  std::mt19937_64 _rnd;
  std::uniform_int_distribution<std::uint8_t> _k0_rnd;

  static bool _stop;

  std::chrono::high_resolution_clock::duration elapsed(std::chrono::high_resolution_clock::time_point);
  static void handler(int);
  static double to_seconds(std::chrono::high_resolution_clock::duration);

public:
  ExperimentThroughput(const ProgramOptions &options);
  void initialize_custom(unsigned core) override;
  bool do_work(unsigned core) override;
  void cleanup_custom(unsigned core) override;
  static void summarize();
};

#endif // __EXP_THROUGHPUT_H_
