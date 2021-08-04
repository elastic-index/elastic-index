
#include <nop/serializer.h>
#include <nop/structure.h>
#include <nop/utility/stream_writer.h>
#include <nop/utility/stream_reader.h>

using Deserializer = nop::Deserializer<nop::StreamReader<std::stringstream>>;  
using Serializer = nop::Serializer<nop::StreamWrite<std::stringstream>>;

class ADO_data_instance
{
public:

  /** 
   * Destructor used to clean up 
   * 
   */
  ~ADO_data_instance();
  
  /** 
   * Invoke a method on the ADO data instance
   * 
   * @param method_name Name of method
   * @param params Parameters to method, serialized by libnop
   * @param result Result (serialized return and out parameters)
   */
  void invoke(const std::string& method_name, /* template functions not supported. how? */
              const Deserializer& params,
              Serializer& result);

              
};

class Cpp_JIT_compiler
{
public:

  /** 
   * Used to create an instance of an ADO in persistent memory and return a 
   * dispatcher (ADO_data_instance) for invocation
   * 
   * @param heap_allocator 
   * @param class_type 
   * @param ctor_params 
   * @param template_type_params 
   * 
   * @return 
   */
  ADO_data_instance * create_instance(std::allocator heap_allocator, /* not sure how to hook this in */
                                      const std::string& class_type,
                                      const Deserializer& ctor_params,
                                      const Deserializer& template_type_params);

  
};
