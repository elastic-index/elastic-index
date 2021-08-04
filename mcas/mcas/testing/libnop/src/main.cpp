#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <boost/core/demangle.hpp>
#include <typeinfo>
#include <iostream>

#include <nop/serializer.h>
#include <nop/structure.h>
#include <nop/utility/stream_writer.h>
#include <nop/utility/stream_reader.h>

namespace example {

struct Person {
  std::string name;
  std::uint32_t age_years;
  std::uint8_t height_inches;
  std::uint16_t weight_pounds;
  NOP_STRUCTURE(Person, name, age_years, height_inches, weight_pounds);
};

}  // namespace example

namespace ado
{

template <typename X>
class list
{
public:
  list(const std::string& name) {
    /* create list object */
  }
  
  void push_back(const X& item) {
    _serializer.Write("op::push_back");
    _serializer.Write(item);
    /* send to mcas */
  }
private:
  using Writer = nop::StreamWriter<std::stringstream>;  
  nop::Serializer<Writer> _serializer;  
};

}

class ADO
{
public:
  template <typename T>
  void invoke(const std::string& name, T arg)
  {
    _serializer.Write(arg);
  }

  template <typename T, typename... Args>
  void invoke(const std::string& name, T arg, Args... args) {
    invoke(name, arg);
    invoke(name, args...);
  }

private:

  using Writer = nop::StreamWriter<std::stringstream>;  
  nop::Serializer<Writer> _serializer;
};

template <typename T>
void create_ADO(const T& arg)
{
  using Writer = nop::StreamWriter<std::stringstream>;  
  nop::Serializer<Writer> serializer;
  serializer.Write(arg);
}

int main(int argc, char** argv) {
  using nop::Deserializer;
  using nop::ErrorStatus;
  using nop::Serializer;
  using nop::Status;
  using nop::StreamReader;
  using nop::StreamWriter;
  using namespace nop;

  create_ADO(std::vector<std::vector<int>>{});

  auto name = typeid(std::vector<std::vector<int>>).name();
  printf("type:%s\n", name);
  std::cout << boost::core::demangle( name ) << std::endl; // prints X<int>
  {
    ADO ado;
    int x=0;
    unsigned long y = 123;
    std::vector<int> z{1,2,3};
    ado.invoke("my_op", x,y,z);
  }

  {
    ado::list<std::vector<int>> list("myList");
    list.push_back(std::vector<int>{1,2,3});
  }
  
  using Writer = nop::StreamWriter<std::stringstream>;

  nop::Serializer<Writer> serializer;

  int x = 666;
  serializer.Write(std::vector<int>{1,3,7,11});
  serializer.Write(x);

  const std::string data = serializer.writer().stream().str();
  std::cout << "Wrote " << data.size() << " bytes. " << std::endl;


  nop::Deserializer<nop::StreamReader<std::stringstream>> deserializer{std::move(serializer.writer().stream())};

  std::vector<int> p;
  auto return_status = deserializer.Read(&p);

  for(auto& i : p)
    printf("i=%d\n", i);

  int xx;
  return_status = deserializer.Read(&xx);
  printf("xx=%d\n",xx);

  return 0;
}
