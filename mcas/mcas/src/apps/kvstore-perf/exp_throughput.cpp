#include "exp_throughput.h"

#include "data.h"
#include "program_options.h"

#include "boost/date_time/posix_time/posix_time.hpp"

#include <fstream>
#include <unistd.h> /* HOST_NAME_MAX, gethostname */
#include <csignal>
#include <fstream>

std::mutex ExperimentThroughput::_iops_lock;
unsigned long ExperimentThroughput::_iops;
bool ExperimentThroughput::_stop= false;

std::uniform_int_distribution<int> distribution(0,99);

namespace
{
	std::string gethostname()
	{
		char buffer[HOST_NAME_MAX];
		if ( ::gethostname(buffer, sizeof buffer) )
		{
			auto e = errno;
			throw std::system_error(std::error_code(e, std::system_category()), "gethostname failed");
		}
		return buffer;
	}
}

ExperimentThroughput::ExperimentThroughput(const ProgramOptions &options)
  : Experiment("throughput", options)
  , _i_rd(0)
  , _i_wr(0)
  , _populated(pool_num_objects(), false)
  , _start_time()
  , _report_time()
  , _report_interval(std::chrono::seconds(options.report_interval))
  , _rand_engine()
  , _rand_pct(0, 99)
  , _ga_pct(options.get_attr_pct)
  , _rd_pct(options.read_pct)
  , _ie_pct(options.insert_erase_pct)
  , _op_count_ga{"get_attr", 0, 0}
  , _op_count_rd{"read", 0, 0}
  , _op_count_in{"insert", 0, 0}
  , _op_count_up{"update", 0, 0}
  , _op_count_er{"erase", 0, 0}
  , _sw_ga()
  , _sw_rd()
  , _sw_wr()
  , _continuous(options.continuous)
  , _hostname(gethostname())
  , _rnd{}
  , _k0_rnd(33, 126)
{
}

void ExperimentThroughput::handler(int)
{
  _stop = true;
}

void ExperimentThroughput::initialize_custom(unsigned /* core */)
{
  _start_time = std::chrono::high_resolution_clock::now();
  std::signal(SIGINT, handler);
}

/*
 * A unit of work is one of three operations:
 *  - a "get MEMORY_TYPE" operation. This is about as close to a no-op a we can come.
 *  - a get operation
 *  - an insert or erase, depending on whether the element exists
 *  - an insert or update, depending on whether the elements exists
 */
bool ExperimentThroughput::do_work(unsigned core)
{
  // handle first time setup
  if ( _first_iter )
  {
    // seed the pool with elements from _data
    _populate_pool_to_capacity(core);
    std::fill(_populated.begin() + pool_element_start(), _populated.begin() + pool_element_end(), true);

    wait_for_delayed_start(core);

    PLOG("[%u] Starting Throughput experiment (get_attr_pct %u rd_pct %u value len:%lu)..."
      , core
      , _ga_pct
      , _rd_pct
      , g_data->value_len());
    _first_iter = false;

    /* DAX initialization is serialized by thread (due to libpmempool behavior).
     * It is in some sense unfair to start measurement in initialize_custom,
     * which occurs before DAX initialization. Reset start of measurement to first
     * put operation.
     */
    _start_time = std::chrono::high_resolution_clock::now();
    _report_time = _start_time;
    if ( _duration_directed )
    {
      _end_time_directed = _start_time + *_duration_directed;
    }
  }

  op_count *ct = &_op_count_rd;
  auto rnd_pct = _rand_pct(_rand_engine);
  /* If fully populated and the randomly chosen operation is "read" */
  try
  {
    if ( rnd_pct < _ga_pct )
    {
      /* the random operation is "get attribute" */
      std::vector<std::uint64_t> memory_type{0};
      {
        StopwatchInterval si(_sw_ga);
        auto rc = store()->get_attribute(pool(),
                                         component::IKVStore::MEMORY_TYPE,
                                         memory_type);
        assert(rc == S_OK);
      }
#if 0
        PLOG("%s in %zu key %s", ct->name, _i_rd, data.key.c_str());
#endif
      ++*ct;
    }
    else if ( rnd_pct < _ga_pct + _rd_pct )
    {
      /* the random operation is "read" */
      auto p = std::find(_populated.begin() + _i_rd, _populated.end(), true);
      if ( p == _populated.end() )
      {
        p = std::find(_populated.begin(), _populated.end(), true);
      }
      if ( p != _populated.end() )
      {
        _i_rd = p - _populated.begin();
        void * pval = 0;
        size_t pval_len;
        const KV_pair &data = g_data->_data[_i_rd];
        {
          StopwatchInterval si(_sw_rd);
          auto rc = store()->get(pool(), data.key, pval, pval_len);
          assert(rc == S_OK);
        }
#if 0
        PLOG("%s in %zu key %s", ct->name, _i_rd, data.key.c_str());
#endif
        store()->free_memory(pval);
        ++*ct;
      }
      ++_i_rd;
    }
    else if ( rnd_pct < _ga_pct + _rd_pct + _ie_pct )
    {
      /* the random operation is "insert or erase" */
      std::uniform_int_distribution<std::size_t> pos_rnd(std::size_t(pool_element_start()), std::size_t(pool_element_end() - 1));
      auto pos = pos_rnd(_rnd);
      const KV_pair &data = g_data->_data[pos];

      /* Randomize key for insert so the same keys are not continually reused
       * Alas, g_data is shared among threads. Cannot change it.
       */
#if 0
      if ( ! _populated[pos] )
      {
        data.key[0] = _k0_rnd(_rnd);
      }
#endif
      ct = _populated[pos] ? &_op_count_er : &_op_count_in;
      auto new_val = common::random_string(g_data->value_len());
      StopwatchInterval si(timer);
      auto rc =
        _populated[pos]
        ? store()->erase(pool(), data.key)
        : store()->put(pool(), data.key, new_val.c_str(), g_data->value_len())
        ;
      if ( rc != S_OK )
      {
        std::ostringstream e;
        e << "pool_element_end = " << pool_element_end() << " " << (_populated[pos] ? "erase" : "put(insert)") << " rc != S_OK: " << rc << " @ pos = " << pos;
        PERR("[%u] %s. Exiting.", core, e.str().c_str());
        throw std::runtime_error(e.str());
      }
#if 0
      PLOG("%s in %zu key %s", ct->name, pos, data.key.c_str());
#endif
      ++*ct;
      _populated[pos].flip();
    }
    else
    {
      auto pos = std::size_t(_i_wr);
      bool upd = _populated[pos];
      ct = upd ? &_op_count_up : &_op_count_in;
      const KV_pair &data = g_data->_data[pos];
      {
        StopwatchInterval si(_sw_wr);
        auto rc = store()->put(pool(), data.key, data.value, data.value_len);
        if ( rc != S_OK )
        {
          std::ostringstream e;
          e << "pool_element_end = " << pool_element_end() << " " << (upd ? "put(replace)" : "put(insert)") << " rc != S_OK: " << rc << " @ pos = " << pos;
          PERR("[%u] %s. Exiting.", core, e.str().c_str());
          throw std::runtime_error(e.str());
        }
      }
#if 0
      PLOG("%s in %zu key %s", ct->name, _i_wr, data.key.c_str());
#endif
      _populated[std::size_t(_i_wr)] = true;
      ++*ct;
      ++_i_wr;
    }
  }
  catch ( std::exception &e )
  {
    PERR("%s in %s threw exception %s! Ending experiment.", ct->name, test_name().c_str(), e.what());
    throw;
  }
  catch ( ... )
  {
    PERR("%s in %s threw unknown object! Ending experiment.", ct->name, test_name().c_str());
    throw;
  }

  auto now = std::chrono::high_resolution_clock::now();
  if ( _report_interval <= now - _report_time )
  {
    auto ptime = boost::posix_time::microsec_clock::universal_time();
    auto ptime_str = to_iso_extended_string(ptime);

    double secs = to_seconds(now - _report_time);
#if 0
    const unsigned long iops_rd = static_cast<unsigned long>(double(_op_count_rd.interval) / secs);
    const unsigned long iops_in = static_cast<unsigned long>(double(_op_count_in.interval) / secs);
    const unsigned long iops_up = static_cast<unsigned long>(double(_op_count_up.interval) / secs);
    const unsigned long iops_er = static_cast<unsigned long>(double(_op_count_er.interval) / secs);
    PLOG(
      "time %s %s core %u IOps rd %lu in %lu up %lu er %lu"
      , ptime_str.c_str()
      , _hostname.c_str()
      , core
      , iops_rd
      , iops_in
      , iops_up
      , iops_er
    );
#else
    const unsigned long iops = static_cast<unsigned long>(double(_op_count_ga.interval + _op_count_rd.interval + _op_count_in.interval + _op_count_up.interval + _op_count_er.interval) / secs);
    PLOG(
      "time %s %s core %u IOps %lu"
      , ptime_str.c_str()
      , _hostname.c_str()
      , core
      , iops
    );
#endif
    _report_time += _report_interval;
    _op_count_ga.interval = 0;
    _op_count_rd.interval = 0;
    _op_count_in.interval = 0;
    _op_count_up.interval = 0;
    _op_count_er.interval = 0;
  }

  if ( _continuous || _end_time_directed )
  {
    _i_rd %= pool_element_end();
    _i_wr %= pool_num_objects();
  }

  auto do_more =
    ( _end_time_directed
      ? std::chrono::high_resolution_clock::now() < *_end_time_directed
      : std::size_t(_i_wr) != pool_num_objects()
    ) && ! _stop
    ;

  if ( ! do_more )
  {
    PINF("[%u] throughput: reached total number of objects. Exiting.", core);
  }

  return do_more;
}

std::chrono::high_resolution_clock::duration ExperimentThroughput::elapsed(std::chrono::high_resolution_clock::time_point p)
{
  return p - _start_time;
}

double ExperimentThroughput::to_seconds(std::chrono::high_resolution_clock::duration d)
{
  return std::chrono::duration<double>(d).count();
}

void ExperimentThroughput::cleanup_custom(unsigned core)
{
  std::signal(SIGINT, SIG_DFL);
  auto duration = elapsed(std::chrono::high_resolution_clock::now());
  _sw_ga.stop();
  _sw_rd.stop();
  _sw_wr.stop();

  PLOG("stopwatch : %g secs", _sw_ga.get_time_in_seconds() + _sw_rd.get_time_in_seconds() + _sw_wr.get_time_in_seconds());
  double secs = to_seconds(duration);
  PLOG("wall clock: %g secs", secs);
  PLOG("op count : ga %lu rd %lu wr %lu er %lu", _op_count_ga.total, _op_count_rd.total, _op_count_in.total + _op_count_up.total, _op_count_er.total);

  unsigned long iops = static_cast<unsigned long>(double(_op_count_ga.total + _op_count_rd.total + _op_count_in.total + _op_count_up.total + _op_count_er.total) / secs);
  PLOG("core %u IOps %lu", core, iops);

  {
    std::lock_guard<std::mutex> g(_iops_lock);
    _iops += iops;
  }
}

void ExperimentThroughput::summarize()
{
  if ( _log_file && ! _log_file->empty() ) {
    std::ofstream ofs(*_log_file);
    ofs << "total IOPS: " << _iops << "\n";
  }
  PMAJOR("total IOPS: %lu", _iops);
}
