#include <api/components.h>
#include <api/kvstore_itf.h>
#include <limits.h>
#include <cstdlib>
#include <boost/program_options.hpp>
#include <cassert>
#include <common/json.h>
#include <common/logging.h>
#include <common/utils.h> /* MiB */
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>

using namespace component;
using namespace std;

namespace
{
	/* things which differ depending on the type of store used */
	struct custom_store
	{
		virtual ~custom_store() {}
		virtual std::size_t minimum_size(std::size_t s) const { return s; }
		virtual component::uuid_t factory() const = 0;
		virtual std::size_t presumed_allocation() const = 0;
	};

	struct custom_mapstore
		: public custom_store
	{
		virtual component::uuid_t factory() const override { return component::mapstore_factory; }
		std::size_t minimum_size(std::size_t s) const override { return std::max(std::size_t(8), s); }
		std::size_t presumed_allocation() const override { return 1ULL << DM_REGION_LOG_GRAIN_SIZE; }
	};

	struct custom_hstore
		: public custom_store
	{
		virtual component::uuid_t factory() const override { return component::hstore_factory; }
		std::size_t presumed_allocation() const override { return MiB(32); }
	};

	struct custom_hstore_cc
		: public custom_hstore
	{
		std::size_t presumed_allocation() const override { return MiB(32); }
	};

	custom_mapstore custom_mapstore_i{};
	custom_hstore custom_hstore_i{};
	custom_hstore_cc custom_hstore_cc_i{};
}

component::IKVStore* init(string& store, const custom_store &c, const string &daxconfig)
{
  string store_lib = "libcomponent-" + store + ".so";
  IBase* comp = load_component(store_lib.c_str(), c.factory());
  assert(comp);
  auto fact    = make_itf_ref(static_cast<IKVStore_factory*>(comp->query_interface(IKVStore_factory::iid())));
  auto kvstore = fact->create(
      0, {{+component::IKVStore_factory::k_name, "numa0"},
          {+component::IKVStore_factory::k_dax_config, daxconfig}
         });

  return kvstore;
}


struct Options {
  bool     aligned;
  bool     exponential;
  unsigned alignment;
  unsigned iteration;
  unsigned minsize;
  unsigned maxsize;
  string   store;
  string   daxconfig;
} g_options{};


int main(int argc, char* argv[])
try
{
  namespace po = boost::program_options;

  po::options_description            desc("Options");
  po::positional_options_description g_pos; /* no positional options */

  namespace c_json = common::json;
  using json = c_json::serializer<c_json::dummy_writer>;

  desc.add_options()("help,h", "Show help")("aligned", "Whether to make random size 2^n aligned")(
      "exponential", "Size distribution should be expoenntial (not linear). Default true iff --aligned is specified")(
      "alignment", po::value<unsigned>()->default_value(8), "Alignment hint for allocate memory")(
      "iteration", po::value<unsigned>()->default_value(1), "Iterations to run the test")(
      "minsize", po::value<unsigned>()->default_value(1), "Min size to allocate")(
      "maxsize", po::value<unsigned>()->default_value(4096), "Max size to allocate")(
      "store", po::value<std::string>()->default_value("mapstore"), "Store type to test: e.g., hstore, hstore-cc, mapstore")(
      "daxconfig", po::value<std::string>()->default_value(
        json::array(
          json::object(
            json::member("region_id", json::number(0))
            , json::member("path", "/dev/dax0.0")
            , json::member("addr", 0x9000000000)
          )
        ).str()
  )
);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(g_pos).run(), vm);

  if (vm.count("help") > 0) {
    std::cout << desc;
    return -1;
  }

  g_options.aligned   = vm.count("aligned");
  g_options.alignment = vm["alignment"].as<unsigned>();
  g_options.iteration = vm["iteration"].as<unsigned>();
  g_options.maxsize   = vm["maxsize"].as<unsigned>();
  g_options.minsize   = vm["minsize"].as<unsigned>();
  g_options.store     = vm["store"].as<std::string>();
  g_options.daxconfig = vm["daxconfig"].as<std::string>();
  g_options.exponential = vm.count("exponential") || g_options.aligned;

using p = custom_store *;
using m = std::map<std::string, p>;
	const m custom_map =
	{
		{ "mapstore", &custom_mapstore_i },
		{ "hstore", &custom_hstore_i },
		{ "hstore-cc", &custom_hstore_cc_i },
	};

	const auto c_it = custom_map.find(g_options.store);
	if ( c_it == custom_map.end() )
    {
      PLOG("store %s not recognized", g_options.store.c_str());
    }

  auto kvstore = init(g_options.store, *c_it->second, g_options.daxconfig);

  vector<double> utilizations;

  for (unsigned i = 0; i < g_options.iteration; i++) {
    PLOG("Running iteration %d", i);
    auto pool = kvstore->create_pool("test.pool", MiB(2));
    if (pool == component::IKVStore::POOL_ERROR) pool = kvstore->open_pool("test.pool");
    assert(pool != component::IKVStore::POOL_ERROR);
    unsigned total = 0;
    auto exp_range = std::log(g_options.maxsize) - std::log(g_options.minsize);
    auto lin_range = g_options.maxsize - g_options.minsize;

    /* a random size from minsize to maxsize */
    auto rand_size =
      [&exp_range, lin_range] () -> unsigned
      {
        unsigned size =
          g_options.exponential
          ? unsigned( g_options.minsize * exp(fmod(double(rand()), exp_range) ) )
          : g_options.minsize + ((rand() & INT_MAX) % lin_range)
          ;

        if ( g_options.aligned )
        {
          while ( size & ( size - 1 ) )
          {
            size &= ( size - 1 );
          }
        }
        return size;
      };

    auto size = rand_size();

    void* addr;
    while (kvstore->allocate_pool_memory(pool, c_it->second->minimum_size(size), g_options.alignment, addr) == S_OK) {
      total += size;
      size = rand_size();
    }

    /* The last allocate failed. Try it again, so we can step through with the debugger */
    kvstore->allocate_pool_memory(pool, c_it->second->minimum_size(size), g_options.alignment, addr);

    unsigned used = 0;
    {
      std::vector<uint64_t> attr;
      auto r = kvstore->get_attribute(pool, IKVStore::PERCENT_USED, attr, nullptr);
      if ( S_OK == r )
      {
        used = unsigned(attr[0]);
      }
      else
      {
        PLOG("Cannot ask about use of pool: rc %d", r);
      }
    }

    kvstore->close_pool(pool);
    kvstore->delete_pool("test.pool");
    double utilization = static_cast<double>(total) / double(c_it->second->presumed_allocation()) * 100;
    utilizations.push_back(utilization);

    PLOG("Iteration %d: Total allocated %d, Memory utilization measured %lf %% claimed %u %%", i + 1, total, utilization, used);
  }

  double min    = *min_element(utilizations.begin(), utilizations.end());
  double max    = *max_element(utilizations.begin(), utilizations.end());
  double mean   = accumulate(utilizations.begin(), utilizations.end(), 0.0) / static_cast<double>(utilizations.size());
  double median = utilizations[utilizations.size() / 2];

  PLOG("Max: %lf %%, Min: %lf %%, Mean: %lf %%, median: %lf %%", max, min, mean, median);
}
catch ( const Exception &e )
{
  PERR("Exception %s", e.cause());
  return 1;
}
catch ( const std::exception &e )
{
  PERR("exception %s", e.what());
  return 1;
}
