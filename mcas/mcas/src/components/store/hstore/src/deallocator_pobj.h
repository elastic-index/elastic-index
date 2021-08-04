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


#ifndef MCAS_HSTORE_DEALLOCATOR_POBJ_H
#define MCAS_HSTORE_DEALLOCATOR_POBJ_H

#include "persister_pmem.h"
#include "pointer_pobj.h"
#include "trace_flags.h"

#pragma GCC diagnostic push
#if defined __clang__
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#endif
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <libpmemobj.h> /* pmemobj_free */
#pragma GCC diagnostic pop

#include <cstdlib> /* size_t, ptrdiff_t */

template <typename T, typename Persister = persister_pmem>
	struct deallocator_pobj;

template <>
	struct deallocator_pobj<void, persister_pmem>
		: public persister_pmem
	{
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = pointer_pobj<void, 0U>;
		using const_pointer = pointer_pobj<const void, 0U>;
		using value_type = void;
		template <typename U, typename P = persister_pmem>
			struct rebind
			{
				using other = deallocator_pobj<U, P>;
			};
	};

template <typename T, typename Persister>
	struct deallocator_pobj
		: public Persister
	{
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = pointer_pobj<T, 0U>;
		using const_pointer = pointer_pobj<const T, 0U>;
		using reference = T &;
		using const_reference = const T &;
		using value_type = T;

		template <typename U>
			struct rebind
			{
				using other = deallocator_pobj<U, Persister>;
			};

		deallocator_pobj() noexcept
		{}

		deallocator_pobj(const deallocator_pobj &) noexcept
		{}

		template <typename U>
			deallocator_pobj(const deallocator_pobj<U, Persister> &) noexcept
				: deallocator_pobj()
			{}

		deallocator_pobj &operator=(const deallocator_pobj &) = delete;

		pointer address(reference x) const noexcept
		{
			return pointer(pmemobj_oid(&x));
		}
		const_pointer address(const_reference x) const noexcept
		{
			return pointer(pmemobj_oid(&x));
		}

		void deallocate(
			pointer oid
			, size_type
#if HSTORE_TRACE_PALLOC
				s
#endif
		)
		{
#if HSTORE_TRACE_PALLOC
			{
				auto ptr = static_cast<char *>(pmemobj_direct(oid));
				hop_hash_log::write(LOG_LOCATION
					, "[", ptr
					, "..", common::p_fmt(ptr + s * sizeof(T))
					, ")"
				);
			}
#endif
			pmemobj_free(&oid);
		}
		auto max_size() const
		{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
			return PMEMOBJ_MAX_ALLOC_SIZE;
#pragma GCC diagnostic pop
		}
		void persist(const void *ptr, size_type len
			, const char *
#if HSTORE_TRACE_PERSIST
			what
#endif
				= "unspecified"
		) const
		{
#if HSTORE_TRACE_PERSIST
			hop_hash_log::write(LOG_LOCATION, what, " ["
				, ptr, ".."
				, common::p_fmt(static_cast<const char*>(ptr)+len)
				, ")"
			);
#endif
			Persister::persist(ptr, len);
		}
	};

#endif
