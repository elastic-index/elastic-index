/* note: we do not include component source, only the API definition */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <gtest/gtest.h>
#pragma GCC diagnostic pop

#include <api/components.h>
#include <api/cluster_itf.h>
#include <common/utils.h>

using namespace component;

namespace {

// The fixture for testing class Foo.
class Zyre_test : public ::testing::Test {

 protected:

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:
  
  virtual void SetUp() {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  virtual void TearDown() {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }
  
  // Objects declared here can be used by all tests in the test case
  static component::ICluster * _zyre;
};

component::ICluster * Zyre_test::_zyre;

TEST_F(Zyre_test, Instantiate)
{
  /* create object instance through factory */
  component::IBase * comp = component::load_component("libcomponent-zyre.so",
                                                      component::cluster_zyre_factory);

  ASSERT_TRUE(comp);
  ICluster_factory * fact = static_cast<ICluster_factory *>(comp->query_interface(ICluster_factory::iid()));

  _zyre = fact->create(3, "bigboy","enp216s0f0", 20000);
  //_zyre = fact->create("bigboy","tcp://localhost");
  
  fact->release_ref();
}

TEST_F(Zyre_test, Methods)
{
  auto uuid = _zyre->uuid();
  ASSERT_FALSE(uuid.empty());
  PLOG("uuid:%s",uuid.c_str());

  _zyre->dump_info();
}

TEST_F(Zyre_test, Start)
{
  _zyre->start_node();
  sleep(2);
}

TEST_F(Zyre_test, JoinGroup)
{
  _zyre->group_join("foobar");
  sleep(2);
}

TEST_F(Zyre_test, Shout)
{
  _zyre->shout("foobar", "X-MSG", "Hello there");
  sleep(1);
}

TEST_F(Zyre_test, LeaveGroup)
{
  _zyre->group_leave("foobar");
}


TEST_F(Zyre_test, Stop)
{
  _zyre->stop_node();
}


TEST_F(Zyre_test, Cleanup)
{
  _zyre->release_ref();
}



} // namespace

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  auto r = RUN_ALL_TESTS();

  return r;
}
