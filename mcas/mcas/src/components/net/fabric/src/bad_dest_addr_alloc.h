/*
   Copyright [2017-2019] [IBM Corporation]
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


#ifndef _FABRIC_BAD_DEST_ADDR_ALLOC_H_
#define _FABRIC_BAD_DEST_ADDR_ALLOC_H_

#include <new> /* bad_alloc */

#include <cstddef> /* size_t */
#include <string> /* to_string */

struct bad_dest_addr_alloc
  : public std::bad_alloc
{
private:
  std::string _what;
public:
  explicit bad_dest_addr_alloc(std::size_t sz)
    : _what{"Failed to allocate " + std::to_string(sz) + " bytes for dest_addr"}
  {}
  const char *what() const noexcept { return _what.c_str(); }
};

#endif
