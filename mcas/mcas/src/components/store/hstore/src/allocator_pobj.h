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


#ifndef MCAS_HSTORE_ALLOCATOR_POBJ_H
#define MCAS_HSTORE_ALLOCATOR_POBJ_H

#include "deallocator_pobj.h"
#include "pool_pobj.h"

#include "hop_hash_log.h"
#include "persister_pmem.h"
#include "pobj_bad_alloc.h"
#include "pointer_pobj.h"
#include "trace_flags.h"

#pragma GCC diagnostic push
#if defined __clang__
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#endif
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <libpmemobj.h>
#pragma GCC diagnostic pop

#include <cerrno>
#include <cstdlib> /* size_t, ptrdiff_t */
#include <cstdint> /* uint64_t */

template <typename T, typename Deallocator = deallocator_pobj<T>>
	struct allocator_pobj;

template <>
	struct allocator_pobj<void>
	{
	public:
		using pointer = pointer_pobj<void, 0U>;
		using const_pointer = pointer_pobj<const void, 0U>;
		using value_type = void;
		template <typename U, typename Persister>
			struct rebind
			{
				using other = allocator_pobj<U, Persister>;
			};
	};

template <typename T, typename Deallocator>
	struct allocator_pobj
		: public Deallocator
		, public pool_pobj
	{
	private:
		using deallocator_type = Deallocator;
	public:
		using typename deallocator_type::size_type;
		using typename deallocator_type::difference_type;
		using typename deallocator_type::pointer;
		using typename deallocator_type::const_pointer;
		using typename deallocator_type::reference;
		using typename deallocator_type::const_reference;
		using typename deallocator_type::value_type;
		template <typename U>
			struct rebind
			{
				using other = allocator_pobj<U, Deallocator>;
			};

		allocator_pobj(PMEMobjpool * pool_, std::uint64_t type_num_) noexcept
			: pool_pobj(pool_, type_num_)
		{}

		allocator_pobj(const allocator_pobj &a_) noexcept
			: allocator_pobj(a_.pool(), a_.type_num())
		{}

		template <typename U>
			allocator_pobj(const allocator_pobj<U> &a_) noexcept
				: allocator_pobj(a_.pool(), a_.type_num())
			{}

		allocator_pobj &operator=(const allocator_pobj &a_) = delete;

		auto allocate(
			size_type s
			, allocator_pobj<void>::const_pointer /* hint */ =
					allocator_pobj<void>::const_pointer{}
			, const char *
#if HSTORE_TRACE_PALLOC
				why
#endif
					= nullptr
		) -> pointer
		{
			PMEMoid oid;
			auto r =
				pmemobj_alloc(
					pool()
					, &oid
					, s * sizeof(T)
					, type_num()
					, nullptr
					, nullptr
				);
			if ( r != 0 )
			{
				throw 0, pobj_bad_alloc(s, sizeof(T), errno);
			}
#if HSTORE_TRACE_PALLOC
			{
				auto ptr = static_cast<char *>(pmemobj_direct(oid));
				hop_hash_log::write(LOG_LOCATION,
					, (why ? why : "(unaligned no reason)")
					, " [", ptr
					, "..", common::p_fmt(ptr + s * sizeof(T))
					, ")"
				);
			}
#endif
			return pointer_pobj<T, 0U>(oid);
		}
	};


#endif
