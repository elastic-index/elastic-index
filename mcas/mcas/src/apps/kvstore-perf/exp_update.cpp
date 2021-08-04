#include "exp_update.h"

#include "data.h"

#include <common/str_utils.h>

#include <mutex>
#include <sstream>
#include <stdexcept>

ExperimentUpdate::ExperimentUpdate(const ProgramOptions &options)
  : Experiment("update", options)
  , _i(0)
  , _start_time()
  , _latencies()
  , _latency_stats()
{
}

void ExperimentUpdate::initialize_custom(unsigned /* core */)
{
  _latency_stats = BinStatistics(bin_count(), bin_threshold_min(), bin_threshold_max());
}

bool ExperimentUpdate::do_work(unsigned core)
{
  // handle first time setup
  if(_first_iter)
  {
    _pool_element_end = 0;

    // seed the pool with elements from _data
    _populate_pool_to_capacity(core);

    wait_for_delayed_start(core);

    PLOG("[%u] Starting Update experiment...", core);

    _first_iter = false;
  }

  // end experiment if we've reached the total number of components
  if ( _i == pool_num_objects() ) {
    PINF("[%u] update: reached total number of components. Exiting.", core);
    return false;
  }

  // generate a new random value with the same value length to use
  auto new_val = common::random_string(g_data->value_len());

  // check time it takes to complete a single put operation
  try
  {
    StopwatchInterval si(timer);
    auto rc = store()->put(pool(), g_data->key(_i), new_val.c_str(), g_data->value_len());
    if ( rc != S_OK )
    {
      std::ostringstream e;
      e << "pool_element_end = " << pool_element_end() << " put rc != S_OK: " << rc << " @ _i = " << _i;
      PERR("[%u] %s. Exiting.", core, e.str().c_str());
      throw std::runtime_error(e.str());
    }
  }
  catch ( std::exception &e )
  {
    PERR("put (update) call threw exception %s! Ending experiment.", e.what());
    throw;
  }
  catch ( ... )
  {
    PERR("%s", "put (update) call threw exception! Ending experiment.");
    throw;
  }

  double lap_time = timer.get_lap_time_in_seconds();
  double time_since_start = timer.get_time_in_seconds();

  _update_data_process_amount(core, _i);

  // store the information for later use
  _start_time.push_back(time_since_start);
  _latencies.push_back(lap_time);
  _latency_stats.update(lap_time);

  _enforce_maximum_pool_size(core, _i);

  ++_i;  // increment after running so all elements get used

/* Temporarily disabled since erase step isn't supported on all components yet -TJanssen 1/30/2019 */
#if 0
  if (_i == pool_element_end()) {
    try {
      _erase_pool_entries_in_range(_pool_element_start, pool_element_end());
      _populate_pool_to_capacity(core);
    }
    catch(...) {
      PERR("failed during erasing and repopulation");
      throw;
    }

    if (_verbose) {
      std::stringstream debug_message;
      debug_message << "pool repopulated: " << _i;
      _debug_print(core, debug_message.str());
    }
  }
#endif
  return true;
}

void ExperimentUpdate::cleanup_custom(unsigned core)
try {
  _debug_print(core, "cleanup_custom started");

  if ( is_verbose() )
  {
    std::stringstream stats_info;
    stats_info << "creating time_stats with " << bin_count() << " bins: [" << _start_time.front() << " - " << _start_time.at(_i-1) << "]. _i = " << _i << std::endl;
    _debug_print(core, stats_info.str());
  }

  double run_time = timer.get_time_in_seconds();
  double iops = double(_i) / run_time;
  PINF("[%u] %s: IOPS--> %2g (%lu operations over %2g seconds)", core, test_name().c_str(), iops, _i, run_time);
  _update_aggregate_iops(iops);

  double throughput = _calculate_current_throughput();
  PINF("[%u] update: THROUGHPUT: %.2f MB/s (%lu bytes over %.3f seconds)", core, throughput, total_data_processed(), run_time);

  if ( is_json_reporting() )
  {

  // compute _start_time_stats pre-lock
  BinStatistics start_time_stats = _compute_bin_statistics_from_vectors(_latencies, _start_time, bin_count(), _start_time.front(), _start_time.at(_i-1), _i);
  _debug_print(core, "time_stats created");

    // save everything
    std::lock_guard<std::mutex> g(g_write_lock);
    _debug_print(core, "cleanup_custom mutex locked");
    // get existing results, read to document variable
    rapidjson::Document document = _get_report_document();
    rapidjson::Value experiment_object(rapidjson::kObjectType);
    experiment_object
      .AddMember("IOPS", double(iops), document.GetAllocator())
      .AddMember("throughput (MB/s)", double(throughput), document.GetAllocator())
      .AddMember("latency", _add_statistics_to_report(_latency_stats, document), document.GetAllocator())
      .AddMember("start_time", _add_statistics_to_report(start_time_stats, document), document.GetAllocator())
      ;
    _print_highest_count_bin(_latency_stats, core);

    _report_document_save(document, core, experiment_object);

    _debug_print(core, "cleanup_custom mutex unlocking");
  }
}
catch(...) {
  PERR("%s", "cleanup_custom failed inside exp_put.h");
  throw;
}
