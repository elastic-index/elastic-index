/*
  Copyright [2020] [IBM Corporation]
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

/** 
 * C-only wrapper to MCAS client component. Mainly used for
 * foreign-function interfacing.
 * 
 */
#ifndef __MCAS_API_WRAPPER_H__
#define __MCAS_API_WRAPPER_H__

#include <stdint.h>
#include <unistd.h>
#include <bits/types/struct_iovec.h>

#ifdef __cplusplus
extern "C"
{
#endif
  
  typedef void * mcas_session_t; /*< handle to MCAS session */
  typedef void * mcas_memory_handle_t; /*< handle to registered memory */
  typedef int status_t;
  
  typedef struct {
    void * internal;
    void * response_data;
  } mcas_async_handle_t; /*< handle for asynchronous operations */
  
  typedef uint32_t      mcas_flags_t;
  typedef uint64_t      offset_t;
  typedef unsigned char byte;

  typedef struct {
    void *   ptr;
    size_t   len;
    uint32_t layer_id;
  }
  layer_response_t;
  
  typedef layer_response_t * mcas_response_array_t;

  typedef struct { /*< pool handle carrying session */
    mcas_session_t session;
    uint64_t handle;
  } mcas_pool_t;
  
  typedef uint32_t      mcas_ado_flags_t;
  typedef uint64_t      addr_t;

  /* see kvstore_itf.h */
  static const mcas_flags_t FLAGS_NONE      = 0x0;
  static const mcas_flags_t FLAGS_READ_ONLY = 0x1; /* lock read-only */
  static const mcas_flags_t FLAGS_SET_SIZE    = 0x2;
  static const mcas_flags_t FLAGS_CREATE_ONLY = 0x4;  /* only succeed if no existing k-v pair exist */
  static const mcas_flags_t FLAGS_DONT_STOMP  = 0x8;  /* do not overwrite existing k-v pair */
  static const mcas_flags_t FLAGS_NO_RESIZE   = 0x10; /* if size < existing size, do not resize */
  static const mcas_flags_t FLAGS_MAX_VALUE   = 0x10;

  static const mcas_memory_handle_t MEMORY_HANDLE_NONE = 0;
  
  /* see mcas_itf.h */
  static const mcas_ado_flags_t ADO_FLAG_NONE = 0;
  /*< operation is asynchronous */
  static const mcas_ado_flags_t ADO_FLAG_ASYNC = (1 << 0);
  /*< create KV pair if needed */
  static const mcas_ado_flags_t ADO_FLAG_CREATE_ON_DEMAND = (1 << 1);
  /*< create only - allocate key,value but don't call ADO */
  static const mcas_ado_flags_t ADO_FLAG_CREATE_ONLY = (1 << 2);
  /*< do not overwrite value if it already exists */
  static const mcas_ado_flags_t ADO_FLAG_NO_OVERWRITE = (1 << 3);
  /*< create value but do not attach to key, unless key does not exist */
  static const mcas_ado_flags_t ADO_FLAG_DETACHED = (1 << 4);
  /*< only take read lock */
  static const mcas_ado_flags_t ADO_FLAG_READ_ONLY = (1 << 5);
  /*< zero any newly allocated value memory */
  static const mcas_ado_flags_t ADO_FLAG_ZERO_NEW_VALUE = (1 << 6);

  typedef enum {
                ATTR_VALUE_LEN                = 1, /* length of a value associated with key */
                ATTR_COUNT                    = 2, /* number of objects */
                ATTR_CRC32                    = 3, /* get CRC32 of a value */
                ATTR_AUTO_HASHTABLE_EXPANSION = 4, /* set to true if the hash table should expand */
                ATTR_PERCENT_USED             = 5, /* get percent used pool capacity at current size */
                ATTR_WRITE_EPOCH_TIME         = 6, /* epoch time at which the key-value pair was last
                                                      written or locked with STORE_LOCK_WRITE */
                ATTR_MEMORY_TYPE              = 7, /* type of memory */
  } mcas_attribute;

  enum {
        MEMORY_TYPE_DRAM        = 0x1,
        MEMORY_TYPE_PMEM_DEVDAX = 0x2,
        MEMORY_TYPE_UNKNOWN     = 0xFF,
  };

  
  /** 
   * Open session to MCAS endpoint
   * 
   * @param server_addr Address of endpoint (e.g., 10.0.0.101::11911)
   * @param net_device Network device (e.g., mlx5_0, eth0)
   * @param debug_level Debug level (0=off)
   * @param patience Timeout patience in seconds (default 30)
   * 
   * @return Handle to mcas session
   */
  status_t mcas_open_session_ex(const char * server_addr,
                                const char * net_device,
                                unsigned debug_level,
                                unsigned patience,
                                mcas_session_t* out_session);

  inline status_t mcas_open_session(const char * server_addr,
                                    const char * net_device,
                                    mcas_session_t* out_session) {
    return mcas_open_session_ex(server_addr, net_device, 0, 30, out_session);
  }

  /** 
   * Close session
   * 
   * @param session Handle returned by mcas_open_session
   * 
   * @return 0 on success, -1 on error
   */
  status_t mcas_close_session(const mcas_session_t session);

  /** 
   * Allocate 64B aligned memory for use in direct calls
   * 
   * @param session Session handle
   * @param size Size to allocate in bytes
   * @param out_ptr [out] Pointer to allocation
   * @param out_handle [out] Memory handle
   * 
   * @return S_OK or int from ::posix_memalign
   */
  status_t mcas_allocate_direct_memory(const mcas_session_t session,
                                       const size_t size,
                                       void ** out_ptr,
                                       void ** out_handle);

  /** 
   * Free direct memory
   * 
   * @param ptr 
   */
  void mcas_free_direct_memory(void * ptr);

  /** 
   * Create a new pool
   * 
   * @param session Session handle
   * @param pool_name Unique pool name
   * @param size Size of pool in bytes (for keys,values and metadata)
   * @param flags Creation flags
   * @param expected_obj_count Expected maximum object count (optimization)
   * @param base Optional base address
   *
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_create_pool_ex(const mcas_session_t session,
                               const char * pool_name,
                               const size_t size,
                               const mcas_flags_t flags,
                               const uint64_t expected_obj_count,
                               const addr_t base_addr,
                               mcas_pool_t * out_pool_handle);

  status_t mcas_create_pool(const mcas_session_t session,
                            const char * pool_name,
                            const size_t size,
                            const mcas_flags_t flags,
                            mcas_pool_t * out_pool_handle);

  /** 
   * Open existing pool
   * 
   * @param session Session handle
   * @param pool_name Pool name
   * @param flags Optional flags 
   * @param base_addr Optional base address
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_open_pool_ex(const mcas_session_t session,
                             const char * pool_name,
                             const mcas_flags_t flags,
                             const addr_t base_addr,
                             mcas_pool_t * out_pool_handle);

  status_t mcas_open_pool(const mcas_session_t session,
                          const char * pool_name,
                          const mcas_flags_t flags,
                          mcas_pool_t * out_pool_handle);
  

  /** 
   * Close a pool
   * 
   * @param pool Pool handle
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_close_pool(const mcas_pool_t pool);


  /** 
   * Delete pool
   * 
   * @param session Session handle
   * @param pool_name Pool name (pool should be closed)
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_delete_pool(const mcas_session_t session,
                            const char * pool_name);

  /** 
   * Close and delete pool
   * 
   * @param pool Pool handle
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_close_delete_pool(const mcas_pool_t pool);

  /** 
   * Set configuration (e.g., AddIndex::VolatileTree)
   * 
   * @param pool Pool handle
   * @param setting Configuring string
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_configure_pool(const mcas_pool_t pool,
                               const char * setting);


  /** 
   * Basic put operation
   * 
   * @param pool Pool handle
   * @param key 
   * @param value 
   * @param value_len 
   * @param flags 
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_put_ex(const mcas_pool_t pool,
                       const char * key,
                       const void * value,
                       const size_t value_len,
                       const unsigned int flags);

  status_t mcas_put(const mcas_pool_t pool,
                    const char * key,
                    const char * value,
                    const unsigned int flags);

  /** 
   * Register memory for direct transfer operations
   * 
   * @param session Session handle
   * @param addr Start address
   * @param len Size of region in bytes
   * @param out_handle Memory handle
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_register_direct_memory(const mcas_session_t session,
                                       const void * addr,
                                       const size_t len,
                                       mcas_memory_handle_t* out_handle);
  
  /** 
   * Unregister memory from direct transfer
   * 
   * @param session Session handle
   * @param handle Handle from register_direct_memory call
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_unregister_direct_memory(const mcas_session_t session,
                                         const mcas_memory_handle_t handle);
  
  /** 
   * Zero-copy put
   * 
   * @param pool Handle to pool
   * @param key Key
   * @param value Value pointer
   * @param value_len Value length
   * @param handle Optional handle (0 for none)
   * @param flags Optional flags
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_put_direct_ex(const mcas_pool_t pool,
                              const char * key,
                              const void * value,
                              const size_t value_len,
                              const mcas_memory_handle_t handle,
                              const unsigned int flags);

  status_t mcas_put_direct(const mcas_pool_t pool,
                           const char * key,
                           const void * value,
                           const size_t value_len);


  /** 
   * Asynchronous put operation
   * 
   * @param pool Pool handle
   * @param key Key
   * @param value Value ptr
   * @param value_len Value len
   * @param flags Optional flags
   * @param out_async_handle Out async handle
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_async_put_ex(const mcas_pool_t pool,
                             const char * key,
                             const void * value,
                             const size_t value_len,
                             const unsigned int flags,
                             mcas_async_handle_t * out_async_handle);

  status_t mcas_async_put(const mcas_pool_t pool,
                          const char * key,
                          const char * value,
                          const unsigned int flags,
                          mcas_async_handle_t * out_async_handle);
  
  /** 
   * Asynchronous zero-copy put
   * 
   * @param pool Handle to pool
   * @param key Key
   * @param value Value pointer
   * @param value_len Value length
   * @param handle Optional handle (0 for none)
   * @param flags Optional flags
   * @param out_async_handle Out async handle
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_async_put_direct_ex(const mcas_pool_t pool,
                                    const char * key,
                                    const void * value,
                                    const size_t value_len,
                                    const mcas_memory_handle_t handle,
                                    const unsigned int flags,
                                    mcas_async_handle_t * out_async_handle);

  /** 
   * Basic get operation
   * 
   * @param pool Pool handle
   * @param key Key name
   * @param out_value Out value data (release with mcas_free_memory)
   * @param out_value_len Out value length
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_get(const mcas_pool_t pool,
                    const char * key,
                    void** out_value,
                    size_t* out_value_len);

  /** 
   * Zero-copy transfer get operation
   * 
   * @param pool Pool handle
   * @param key Key
   * @param value Pointer to target buffer
   * @param inout_size_value Size of target buffer, then size of transfer
   * @param handle Handle to direct registered memory
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_get_direct_ex(const mcas_pool_t pool,
                              const char * key,
                              void * value,
                              size_t * inout_size_value,
                              mcas_memory_handle_t handle);

  status_t mcas_get_direct(const mcas_pool_t pool,
                           const char * key,
                           void * value,
                           size_t * inout_size_value);


  /** 
   * Asynchronous zero-copy transfer get operation
   * 
   * @param pool Pool handle
   * @param key Key
   * @param out_value Pointer to target buffer
   * @param inout_size_value Size of target buffer, then size of transfer
   * @param handle Handle to direct registered memory
   * @param out_async_handle Out async handle
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_async_get_direct_ex(const mcas_pool_t pool,
                                    const char * key,
                                    void * out_value,
                                    size_t * inout_size_value,
                                    mcas_memory_handle_t handle,
                                    mcas_async_handle_t * out_async_handle);

  status_t mcas_async_get_direct(const mcas_pool_t pool,
                                 const char * key,
                                 void * out_value,
                                 size_t * inout_size_value,
                                 mcas_async_handle_t * out_async_handle);


  /** 
   * Get direct sub-region of value space memory
   * 
   * @param pool Pool handle
   * @param offset Offset in value space
   * @param out_buffer Destination buffer
   * @param inout_size Size of destination buffer/size to copy, then size transferred
   * @param handle Memory handle
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_get_direct_offset_ex(const mcas_pool_t pool,
                                     const offset_t offset,
                                     void * out_buffer,
                                     size_t * inout_size,
                                     mcas_memory_handle_t handle);

  status_t mcas_get_direct_offset(const mcas_pool_t pool,
                                  const offset_t offset,
                                  void * out_buffer,
                                  size_t * inout_size);


  /** 
   * Asynchronous version of mcas_get_direct_offset_ex
   * 
   * @param pool Pool handle
   * @param offset Offset in value space
   * @param out_buffer Destination buffer
   * @param inout_size Size of destination buffer/size to copy, then size transferred
   * @param handle Memory handle
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_async_get_direct_offset_ex(const mcas_pool_t pool,
                                           const offset_t offset,
                                           void * out_buffer,
                                           size_t * inout_size,
                                           mcas_memory_handle_t handle,
                                           mcas_async_handle_t * out_async_handle);

  status_t mcas_async_get_direct_offset(const mcas_pool_t pool,
                                        const offset_t offset,
                                        void * out_buffer,
                                        size_t * inout_size,
                                        mcas_async_handle_t * out_async_handle);

  /** 
   * Put direct sub-region of value space memory
   * 
   * @param pool Pool handle
   * @param offset Offset in value space
   * @param out_buffer Destination buffer
   * @param inout_size Size of destination buffer/size to copy, then size transferred
   * @param handle Memory handle
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_put_direct_offset_ex(const mcas_pool_t pool,
                                     const offset_t offset,
                                     const void *const buffer,
                                     size_t * inout_size,
                                     mcas_memory_handle_t handle);

  status_t mcas_put_direct_offset(const mcas_pool_t pool,
                                  const offset_t offset,
                                  const void *const buffer,
                                  size_t * inout_size);

  /** 
   * Asynchronous version of mcas_put_direct_offset_ex
   * 
   * @param pool Pool handle
   * @param offset Offset in value space
   * @param out_buffer Destination buffer
   * @param inout_size Size of destination buffer/size to copy, then size transferred
   * @param handle Memory handle
   * @param mcas_async_handle_t Out asynchronous handle
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_async_put_direct_offset_ex(const mcas_pool_t pool,
                                           const offset_t offset,
                                           const void *const buffer,
                                           size_t * inout_size,
                                           mcas_memory_handle_t handle,
                                           mcas_async_handle_t * out_async_handle);
  
  status_t mcas_async_put_direct_offset(const mcas_pool_t pool,
                                        const offset_t offset,
                                        const void *const buffer,
                                        size_t * inout_size,
                                        mcas_async_handle_t * out_async_handle);
  

  /** 
   * Check for completion of asynchronous task
   * 
   * @param session Session handle
   * @param handle Async work handle
   * 
   * @return 0 on completion or < 0 on still waiting (E_BUSY=-9)
   */
  status_t mcas_check_async_completion(const mcas_session_t session,
                                       mcas_async_handle_t handle);


  /** 
   * Find a key using secondary index
   * 
   * @param pool Pool handle
   * @param key_expression Expression for key (e.g., "prefix:carKey")
   * @param offset Offset from which to search
   * @param out_matched_offset Out offset of last match
   * @param out_matched_key Copy of key matched; free with POSIX free() call
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_find(const mcas_pool_t pool,
                     const char * key_expression,
                     const offset_t offset,
                     offset_t* out_matched_offset,
                     char** out_matched_key);

  /** 
   * Erase key-value pair from pool
   * 
   * @param pool Pool handle
   * @param key Key
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_erase(const mcas_pool_t pool,
                      const char * key);


  /** 
   * Asynchronous erase key-value pair from pool
   * 
   * @param pool Pool handle
   * @param key Key
   * @param handle Out asynchronous task handle
   * 
   * @return 
   */
  status_t mcas_async_erase(const mcas_pool_t pool,
                            const char * key,
                            mcas_async_handle_t * handle);
  
  /** 
   * Free memory returned by get operations
   * 
   * @param session Session handle
   * @param p Pointer to region to free
   * 
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_free_memory(const mcas_session_t session, void * p);


  /** 
   * Return number of objects in the pool
   * 
   * @param pool Pool handle
   * 
   * @return Number of objects in the pool
   */
  size_t mcas_count(const mcas_pool_t pool);


  /**
   * Get attribute for key or pool (see enum Attribute)
   *
   * @param pool Pool handle
   * @param key Optional key (use NULL if not needed)
   * @param attr Attribute to retrieve
   * @param out_value Array of attribute values. Free with POSIX 'free'
   * @param out_value_count Size of array of out values
   *
   * @return 0 on success, < 0 on failure
   */   
  status_t mcas_get_attribute(const mcas_pool_t pool,
                              const char * key, 
                              mcas_attribute attr,
                              uint64_t** out_value,
                              size_t* out_value_count);

  /** 
   * Free response vector data
   * 
   * @param out_response_vector Response array
   */
  void mcas_free_responses(mcas_response_array_t out_response_vector);

  /**
   * Used to invoke an operation on an active data object (see mcas_itf.h)
   *
   * @param pool Pool handle
   * @param key Key. Note, if key is empty, the work request is key-less.
   * @param request Request data
   * @param request_len Length of request data in bytes
   * @param flags Flags for invocation (see ADO_FLAG_CREATE_ONLY, ADO_FLAG_READ_ONLY)
   * @param value_size Optional parameter to define value size to create for
   * @param out_response_vector Responses from invocation as an array of iovec
   * @param out_response_vector_count Number of iovecs in result
   *
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_invoke_ado(const mcas_pool_t pool,
                           const char * key,
                           const void * request,
                           const size_t request_len,
                           const mcas_ado_flags_t flags,
                           const size_t value_size,
                           mcas_response_array_t * out_response_vector,
                           size_t * out_response_vector_count);
                    
  /**
   * Asynchonously used to invoke an operation on an active data object (see mcas_itf.h)
   *
   * @param pool Pool handle
   * @param key Key. Note, if key is empty, the work request is key-less.
   * @param request Request data
   * @param request_len Length of request data in bytes
   * @param flags Flags for invocation (see ADO_FLAG_CREATE_ONLY, ADO_FLAG_READ_ONLY)
   * @param value_size Optional parameter to define value size to create for
   * @param handle Out asynchronous task handle
   *
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_async_invoke_ado(const mcas_pool_t pool,
                                 const char * key,
                                 const void * request,
                                 const size_t request_len,
                                 const mcas_ado_flags_t flags,
                                 const size_t value_size,
                                 mcas_async_handle_t * handle);

  /** 
   * Check for mcas_async_invoke_ado result
   * 
   * @param pool Pool handle
   * @param handle Handle from mcas_async_invoke_ado
   * @param out_response_vector Out pointer to array of iovec
   * @param out_response_vector_count Out number of elements in vector
   * 
   * @return 0 on completion; response that is freed with 'mcas_free_responses'
   */
  status_t mcas_check_async_invoke_ado(const mcas_pool_t pool,
                                       mcas_async_handle_t handle,
                                       mcas_response_array_t * out_response_vector,
                                       size_t * out_response_vector_count);
  

  /**
   * Put a value then invoke an operation on an active data
   * object (see mcas_itf.h)
   *
   * @param pool Pool handle
   * @param key Key. Note, if key is empty, the work request is key-less.
   * @param request Request data
   * @param request_len Length of request data in bytes
   * @param value Value data
   * @param value_len Length of value data in bytes
   * @param root_len Length to allocate for root value (with ADO_FLAG_DETACHED)
   * @param flags Flags for invocation (see ADO_FLAG_CREATE_ONLY, ADO_FLAG_READ_ONLY)
   * @param out_response_vector Responses from invocation as an array of iovec
   * @param out_response_vector_count Number of iovecs in result
   *
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_invoke_put_ado(const mcas_pool_t pool,
                               const char * key,
                               const void * request,
                               const size_t request_len,
                               const void * value,
                               const size_t value_len,
                               const size_t root_len,
                               const mcas_ado_flags_t flags,
                               mcas_response_array_t * out_response_vector,
                               size_t * out_response_vector_count);

  /**
   * Asynchonously put a value then invoke an operation on an
   * active data object (see mcas_itf.h)
   *
   * @param pool Pool handle
   * @param key Key. Note, if key is empty, the work request is key-less.
   * @param request Request data
   * @param request_len Length of request data in bytes
   * @param value Value data
   * @param value_len Length of value data in bytes
   * @param flags Flags for invocation (see ADO_FLAG_CREATE_ONLY, ADO_FLAG_READ_ONLY)
   * @param handle Out asynchronous task handle
   *
   * @return 0 on success, < 0 on failure
   */
  status_t mcas_async_invoke_put_ado(const mcas_pool_t pool,
                                     const char * key,
                                     const void * request,
                                     const size_t request_len,
                                     const void * value,
                                     const size_t value_len,
                                     const size_t root_len,
                                     const mcas_ado_flags_t flags,
                                     mcas_async_handle_t * handle);
  
  /** 
   * Check for mcas_async_invoke_put_ado result
   * 
   * @param pool Pool handle
   * @param handle Handle from mcas_async_invoke_put_ado
   * @param out_response_vector Out pointer to array of iovec
   * @param out_response_vector_count Out number of elements in vector
   * 
   * @return 0 on completion; response that is freed with 'mcas_free_responses'
   */
  status_t mcas_check_async_invoke_put_ado(const mcas_pool_t pool,
                                           mcas_async_handle_t handle,
                                           mcas_response_array_t * out_response_vector,
                                           size_t * out_response_vector_count);

  /** 
   * Free response from invoke_ado or invoke_put_ado
   * 
   * @param out_response_vector Response vector
   * @param out_response_vector_count Num elements in response vector
   * 
   * @return 0 on success
   */
  status_t mcas_free_response(mcas_response_array_t out_response_vector,
                              size_t out_response_vector_count);

  /** 
   * Helper to index into response array
   * 
   * @param response_vector Response vector
   * @param index Index of requested response
   * @param out_ptr [out] Pointer to response buffer
   * @param out_len [out] Size of response buffer
   * 
   * @return 0 on success
   */
  status_t mcas_get_response(mcas_response_array_t response_vector,
                             size_t index,
                             void ** out_ptr,
                             size_t * out_len);

  /** 
   * Debug operation
   * 
   * @param pool Pool handle
   * @param cmd Debug command 
   * @param arg Argument
   */
  void mcas_debug(const mcas_pool_t pool,
                  const unsigned cmd,
                  const uint64_t arg);

                    
#ifdef __cplusplus
}
#endif

#endif // __MCAS_API_WRAPPER_H__
