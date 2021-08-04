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


#ifndef MCAS_HSTORE_HOP_HASH_LOG_H
#define MCAS_HSTORE_HOP_HASH_LOG_H

#include <common/logging.h>
#include <iosfwd>
#include <string>
#include <sstream>

#define LOG_LOCATION_STATIC __FILE__, ":", __LINE__, " ", __func__, "() "
#define LOG_LOCATION LOG_LOCATION_STATIC, "this(", common::p_fmt(this), ") "

namespace hop_hash_log_impl
{
	void wr(const std::string &s);
}

template <bool>
	struct hop_hash_log;

template <>
	struct hop_hash_log<true>
	{
	private:
		static void wr(std::ostream &)
		{
		}

		template<typename T, typename... Args>
			static void wr(std::ostream &o, const T & e, const Args & ... args)
			{
				o << e;
				wr(o, args...);
			}

	public:
		template<typename... Args>
			static void write(const Args & ... args)
			{
				std::ostringstream o;
				wr(o, args...);
				hop_hash_log_impl::wr(o.str());
			}
	};

template <>
	struct hop_hash_log<false>
	{
		template<typename... Args>
			static void write(const Args & ...)
			{
			}
	};

#endif
