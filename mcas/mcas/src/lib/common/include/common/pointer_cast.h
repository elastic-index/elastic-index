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


#ifndef _MCAS_COMMON_POINTER_CAST_H_
#define _MCAS_COMMON_POINTER_CAST_H_

/* Unsafe conversion from U* to T* without reniterpret_cast */

namespace common
{
  template <typename T>
    T *pointer_cast(void *p)
    {
      return static_cast<T *>(p);
    }
  template <typename T>
    const T *pointer_cast(const void *p)
    {
      return static_cast<const T *>(p);
    }
}

#endif
