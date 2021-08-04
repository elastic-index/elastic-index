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

#include "rampant_alloc.h"

#include <assert.h>

namespace nupm
{


class Rampant_allocator::Rpmalloc
{
public:

};


Rampant_allocator::Rampant_allocator() :
  _allocator(std::make_unique<Rpmalloc>())
{
  assert(_allocator);
}

Rampant_allocator::~Rampant_allocator()
{
}

}


nupm::Rampant_allocator a;
