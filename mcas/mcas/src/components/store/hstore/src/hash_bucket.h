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


#ifndef _MCAS_HSTORE_HASH_BUCKET_H
#define _MCAS_HSTORE_HASH_BUCKET_H

#include "hstore_config.h"
#include "owner.h"
#include "content.h"

/*
 * hash_bucket, consisting of space for "owner" and "content".
 */

namespace impl
{
	template <typename Value>
		struct hash_bucket
			: public owner
			, public content<Value>
		{
			using owner_type = owner;
			using content_type = content<Value>;
			explicit hash_bucket(owner owner_)
				: owner{owner_}
				, content_type()
			{}
			explicit hash_bucket()
				: hash_bucket(owner())
			{}
#if ! TRACED_OWNER
			static_assert(sizeof(owner) <= 8, "Owner size exceeds presumed intended value (8)");
#endif
#if ! TRACED_CONTENT
#if ENABLE_TIMESTAMPS
			static_assert(sizeof(content<Value>) <= 56, "Content size exceeds presumed intended limit (56)");
#else
			static_assert(sizeof(content<Value>) <= 48, "Content size exceeds presumed intended limit (48)");
#endif
#endif
			/* A hash_bucket consists of two colocated pieces: owner and contents.
			* They do not move together, therefore copy or move of an entire
			* hash_bucket is an error.
			*/
			hash_bucket(const hash_bucket &) = delete;
			hash_bucket &operator=(const hash_bucket &) = delete;
			hash_bucket(hash_bucket &) = delete;
			hash_bucket &operator=(hash_bucket &) = delete;
		};
}

#endif
