/*
  Copyright [2017-2020] [IBM Corporation]
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
  http://www.apache.org/licenses/LICENSE-2.0
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "rust_wrapper.h"
#include <libpmem.h>
#include <api/interfaces.h>
#include <api/kvstore_itf.h>
#include <common/logging.h>
#include <common/dump_utils.h>
#include <common/time.h>
#include <common/types.h>
#include <string>
#include <ado_proto.h>

static unsigned debug_level() { return 2; }

extern "C"
{
  /* these map to versions in lib.rs */
  struct Value {
    void * buffer;
    size_t buffer_size;    
  };

  struct Response {
    void * buffer;
    size_t buffer_size;
    size_t used_size;
    uint32_t layer_id;
  };

  status_t ffi_register_mapped_memory(uint64_t shard_vaddr, uint64_t local_vaddr, size_t len);
  
  status_t ffi_do_work(void* callback_ptr,
                       uint64_t work_id,
                       const char * key,
                       const size_t key_len,
                       const Value * attached_value,
                       const Value * detached_value,
                       const uint8_t * request,
                       const size_t request_len,
                       const bool new_root,                       
                       Response * response);

  status_t ffi_launch_event(const uint64_t auth_id,
                            const char * pool_name,
                            const size_t pool_name_len,
                            const size_t pool_size,
                            const unsigned int pool_flags,
                            const unsigned int memory_type,
                            const size_t expected_obj_count,
                            const char * json_params,
                            const size_t json_params_len);

  void ffi_cluster_event(const char * sender,
                         const size_t sender_len,
                         const char * type,
                         const size_t type_len,
                         const char * message,
                         const size_t message_len);

  Value callback_allocate_pool_memory(void * callback_ptr, size_t size) {
    assert(callback_ptr);
    auto p_this = reinterpret_cast<ADO_rust_wrapper_plugin *>(callback_ptr);
    void *ptr = nullptr;
    auto result = p_this->cb_allocate_pool_memory(size, 64 /* alignment */, ptr);
    if(result != S_OK) throw General_exception("allocate_pool_memory_failed");
    return {ptr, size};
  }

  status_t callback_free_pool_memory(void * callback_ptr, Value value) {
    assert(callback_ptr);
    auto p_this = reinterpret_cast<ADO_rust_wrapper_plugin *>(callback_ptr);
    return p_this->cb_free_pool_memory(value.buffer_size, value.buffer);
  }

  status_t callback_create_key(void * callback_ptr,
                               uint64_t work_id,
                               const char * key,
                               size_t value_size,
                               int flags,
                               Value* out_value,
                               component::IKVStore::key_t* out_key_handle) {
    assert(callback_ptr);
    assert(out_value);
    
    auto p_this = reinterpret_cast<ADO_rust_wrapper_plugin *>(callback_ptr);
    const char * key_ptr;
    auto rc = p_this->cb_create_key(work_id,
                                    key,
                                    value_size,
                                    flags,
                                    out_value->buffer,
                                    &key_ptr,
                                    out_key_handle);
    
    if(rc == S_OK) {
      out_value->buffer_size = value_size;
    }
    return rc;
  }

  status_t callback_open_key(void * callback_ptr,
                             uint64_t work_id,
                             const char * key,
                             int flags,
                             Value* out_value,
                             component::IKVStore::key_t* out_key_handle) {
    assert(callback_ptr);
    assert(out_value);
    
    auto p_this = reinterpret_cast<ADO_rust_wrapper_plugin *>(callback_ptr);
    const char * key_ptr;
    auto rc = p_this->cb_open_key(work_id,
                                  key,
                                  flags,
                                  out_value->buffer,
                                  out_value->buffer_size,                                  
                                  &key_ptr,
                                  out_key_handle);

    if(debug_level() >= 2)
      PLOG("rust-wrapper: callback_open_key (%s) out_value=%p", key, out_value->buffer);
    return rc;
  }

  status_t callback_unlock_key(void * callback_ptr,
                               uint64_t work_id,
                               component::IKVStore::key_t key_handle) {
    assert(callback_ptr);
    auto p_this = reinterpret_cast<ADO_rust_wrapper_plugin *>(callback_ptr);
    auto rc = p_this->cb_unlock(work_id, key_handle);
    if(debug_level() >= 2)
      PLOG("callback_unlock_key: %p %lx %p rc=%d", callback_ptr, work_id, key_handle, rc);
    return rc;
  }

  void * callback_get_pool_info(void * callback_ptr) {
    assert(callback_ptr);
    auto p_this = reinterpret_cast<ADO_rust_wrapper_plugin *>(callback_ptr);
    if(debug_level() >= 2)
      PLOG("callback_get_pool_info: %p", callback_ptr);
    std::string info;
    if(p_this->cb_get_pool_info(info) != S_OK)
      throw General_exception("cb_get_pool_info failed unexpectedly");
    
    char * result = reinterpret_cast<char*>(::malloc(info.size() + 1));
    memcpy(result, info.c_str(), info.size());
    result[info.size()]='\0';
    return result;
  }
  
  status_t callback_resize_value(void * callback_ptr,
                                 uint64_t work_id,
                                 const char * key,
                                 size_t new_value_size,
                                 Value* out_new_value) {
    assert(callback_ptr);
    assert(out_new_value);
    std::string key_name(key);
    auto p_this = reinterpret_cast<ADO_rust_wrapper_plugin *>(callback_ptr);
    void * new_val_addr = nullptr;
    auto rc = p_this->cb_resize_value(work_id, key_name, new_value_size, new_val_addr);

    out_new_value->buffer = new_val_addr;
    out_new_value->buffer_size = rc == S_OK ? new_value_size : 0;
    
    return rc;
  }

  status_t callback_iterate(void * callback_ptr,
                            const common::epoch_time_t t_begin,
                            const common::epoch_time_t t_end,
                            void *& iterator,
                            component::IKVStore::pool_reference_t& out_reference) {
    assert(callback_ptr);

    auto p_this = reinterpret_cast<ADO_rust_wrapper_plugin *>(callback_ptr);

    auto result = p_this->cb_iterate(t_begin,
                                     t_end,
                                     reinterpret_cast<component::IKVStore::pool_iterator_t&>(iterator),
                                     out_reference);      
    return result;
  }


  status_t callback_find_key(void * callback_ptr,
                             const char * key_expression,
                             const size_t key_len,
                             const component::IKVIndex::find_t find_type,
                             offset_t& position,
                             char*& out_matched_key,
                             size_t& out_matched_key_len) {
    assert(callback_ptr);
    auto p_this = reinterpret_cast<ADO_rust_wrapper_plugin *>(callback_ptr);
    std::string expr(key_expression, key_len);
    std::string out_match;
    offset_t loc = 0;
    auto result = p_this->cb_find_key(expr, position, find_type, loc, out_match);

    if(result == S_OK) {
      out_matched_key_len = out_match.size();
      out_matched_key = reinterpret_cast<char*>(::malloc(out_matched_key_len + 1));
      memcpy(out_matched_key, out_match.c_str(), out_matched_key_len);
      out_matched_key[out_matched_key_len] = '\0';
      if(debug_level() >= 2)
        PLOG("rust-wrapper::callback_find_key: match=%s begin=%lu foundat=%lu", out_match.c_str(), position, loc);
    }
    else {
      out_matched_key_len = 0;
      out_matched_key = nullptr;
    }
    
    return result;
  }
                           
  status_t callback_configure(void * callback_ptr,
                              uint32_t option)
  {
    assert(callback_ptr);
    auto p_this = reinterpret_cast<ADO_rust_wrapper_plugin *>(callback_ptr);
    return p_this->cb_configure(option);
  }
  
  void debug_break() {
    asm("int3");
  }
  
}


status_t ADO_rust_wrapper_plugin::register_mapped_memory(void *shard_vaddr,
                                                       void *local_vaddr,
                                                       size_t len) {

  return ffi_register_mapped_memory(reinterpret_cast<uint64_t>(shard_vaddr),
                                    reinterpret_cast<uint64_t>(local_vaddr),
                                    len);
}

void ADO_rust_wrapper_plugin::launch_event(const uint64_t auth_id,
                                           const std::string& pool_name,
                                           const size_t pool_size,
                                           const unsigned int pool_flags,
                                           const unsigned int memory_type,
                                           const size_t expected_obj_count,
                                           const std::vector<std::string>& params) {
  const char * pool_name_ptr = pool_name.c_str();
  size_t pool_name_len = pool_name.size();

  /* translate param vector */
  if(params.size() > 0) {
    std::stringstream ss;
    
    for(auto p : params) {
      ss << p.substr(1, p.size() - 2) << ",";
    }
    ss.seekp(-1,ss.cur);
    
    std::string pstr = ss.str();
    ffi_launch_event(auth_id,
                     pool_name_ptr,
                     pool_name_len,
                     pool_size,
                     pool_flags,
                     memory_type,
                     expected_obj_count,
                     pstr.c_str(),
                     pstr.size());
    
  }
  else {
    ffi_launch_event(auth_id,
                     pool_name_ptr,
                     pool_name_len,
                     pool_size,
                     pool_flags,
                     memory_type,
                     expected_obj_count,
                     nullptr,
                     0);
  }
}


status_t ADO_rust_wrapper_plugin::do_work(uint64_t work_id,
                                          const char * key,
                                          size_t key_len,
                                          IADO_plugin::value_space_t& values,
                                          const void *in_work_request, /* don't use iovec because of non-const */
                                          const size_t in_work_request_len,
                                          bool new_root,
                                          response_buffer_vector_t& response_buffers) {


  if(debug_level() >= 2) {
    PLOG("ADO_rust_wrapper_plugin::do_work (work_id=%lx key=%.*s)",
         work_id, boost::numeric_cast<int>(key_len), key);
    PLOG("ADO_rust_wrapper_plugin::do_work (work_request=%p, work_request_len=%lu)",
         in_work_request, in_work_request_len);
  }

  /* ADO_protocol_builder::MAX_MESSAGE_SIZE governs size of messages */

  /* allocate memory for the response */
  size_t response_buffer_size = ADO_protocol_builder::MAX_MESSAGE_SIZE;
  void * response_buffer = ::aligned_alloc(CACHE_LINE_SIZE, response_buffer_size);
  memset(response_buffer, 0, response_buffer_size);
  
  Response response{response_buffer, response_buffer_size, 0, 0};
  
  assert(values.size() > 0);
  Value attached_value{values[0].ptr, values[0].len};

  status_t rc;
  if(values.size() > 1) {
    Value detached_value{values[1].ptr, values[1].len};
    rc = ffi_do_work(reinterpret_cast<void*>(this),
                     work_id, key, key_len, &attached_value, &detached_value,
                     reinterpret_cast<const uint8_t*>(in_work_request),
                     in_work_request_len,
                     new_root,
                     &response);    
  }
  else {
    Value detached_value{nullptr, 0};
    rc = ffi_do_work(reinterpret_cast<void*>(this),
                     work_id, key, key_len, &attached_value, &detached_value,
                     reinterpret_cast<const uint8_t*>(in_work_request),
                     in_work_request_len,
                     new_root,
                     &response);
  }

  if(debug_level() >= 2)
    PLOG("ADO: do_work response=(%s)", reinterpret_cast<const char*>( response.buffer));
  
  return rc;
}

void ADO_rust_wrapper_plugin::notify_op_event(component::ADO_op op) {
  //  auto opstr = to_str(op);
}

void ADO_rust_wrapper_plugin::cluster_event(const std::string& sender,
                                            const std::string& type,
                                            const std::string& message) {
  ffi_cluster_event(sender.c_str(),
                    sender.size(),
                    type.c_str(),
                    type.size(),
                    message.c_str(),
                    message.size());
}


status_t ADO_rust_wrapper_plugin::shutdown() {
  /* here you would put graceful shutdown code if any */
  return S_OK;
}

/**
 * Factory-less entry point
 *
 */
extern "C" void *factory_createInstance(component::uuid_t interface_iid) {
  PLOG("instantiating ADO_rust_wrapper_plugin");
  if (interface_iid == interface::ado_plugin)
    return static_cast<void *>(new ADO_rust_wrapper_plugin());
  else
    return NULL;
}

#undef RESET_STATE
