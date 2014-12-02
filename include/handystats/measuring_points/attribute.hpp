/*
  Copyright (c) 2014 Yandex LLC. All rights reserved.

  This file is part of Handystats.

  Handystats is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Handystats is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HANDYSTATS_ATTRIBUTE_MEASURING_POINTS_HPP_
#define HANDYSTATS_ATTRIBUTE_MEASURING_POINTS_HPP_

#include <cstdint>

#include <string>

#include <handystats/macros.h>
#include <handystats/attribute.hpp>


namespace handystats { namespace measuring_points {

template <typename ValueType>
void attribute_set(
		std::string&& attribute_name,
		const ValueType& value,
		const handystats::attribute::time_point& timestamp = handystats::attribute::clock::now()
	);

template <>
void attribute_set<handystats::attribute::value_type>(
		std::string&& attribute_name,
		const handystats::attribute::value_type& value,
		const handystats::attribute::time_point& timestamp
	);

template <>
void attribute_set<bool>(
		std::string&& attribute_name,
		const bool& b,
		const handystats::attribute::time_point& timestamp
	);

template <>
void attribute_set<int>(
		std::string&& attribute_name,
		const int& i,
		const handystats::attribute::time_point& timestamp
	);

template <>
void attribute_set<unsigned>(
		std::string&& attribute_name,
		const unsigned& u,
		const handystats::attribute::time_point& timestamp
	);

template <>
void attribute_set<int64_t>(
		std::string&& attribute_name,
		const int64_t& i64,
		const handystats::attribute::time_point& timestamp
	);

template <>
void attribute_set<uint64_t>(
		std::string&& attribute_name,
		const uint64_t& u64,
		const handystats::attribute::time_point& timestamp
	);

template <>
void attribute_set<double>(
		std::string&& attribute_name,
		const double& d,
		const handystats::attribute::time_point& timestamp
	);

template <>
void attribute_set<std::string>(
		std::string&& attribute_name,
		const std::string& s,
		const handystats::attribute::time_point& timestamp
	);

}} // namespace handystats::measuring_points


#ifndef HANDYSTATS_DISABLE

	#define HANDY_ATTRIBUTE_SET(...) HANDY_PP_MEASURING_POINT_WRAPPER(handystats::measuring_points::attribute_set, __VA_ARGS__)

	#define HANDY_ATTRIBUTE_SET_BOOL(...) HANDY_PP_MEASURING_POINT_WRAPPER(handystats::measuring_points::attribute_set<bool>, __VA_ARGS__)

	#define HANDY_ATTRIBUTE_SET_INT(...) HANDY_PP_MEASURING_POINT_WRAPPER(handystats::measuring_points::attribute_set<int>, __VA_ARGS__)

	#define HANDY_ATTRIBUTE_SET_UINT(...) HANDY_PP_MEASURING_POINT_WRAPPER(handystats::measuring_points::attribute_set<unsigned>, __VA_ARGS__)

	#define HANDY_ATTRIBUTE_SET_INT64(...) HANDY_PP_MEASURING_POINT_WRAPPER(handystats::measuring_points::attribute_set<int64_t>, __VA_ARGS__)

	#define HANDY_ATTRIBUTE_SET_UINT64(...) HANDY_PP_MEASURING_POINT_WRAPPER(handystats::measuring_points::attribute_set<uint64_t>, __VA_ARGS__)

	#define HANDY_ATTRIBUTE_SET_DOUBLE(...) HANDY_PP_MEASURING_POINT_WRAPPER(handystats::measuring_points::attribute_set<double>, __VA_ARGS__)

	#define HANDY_ATTRIBUTE_SET_STRING(...) HANDY_PP_MEASURING_POINT_WRAPPER(handystats::measuring_points::attribute_set<std::string>, __VA_ARGS__)

#else

	#define HANDY_ATTRIBUTE_SET(...)

	#define HANDY_ATTRIBUTE_SET_BOOL(...)

	#define HANDY_ATTRIBUTE_SET_INT(...)

	#define HANDY_ATTRIBUTE_SET_UINT(...)

	#define HANDY_ATTRIBUTE_SET_INT64(...)

	#define HANDY_ATTRIBUTE_SET_UINT64(...)

	#define HANDY_ATTRIBUTE_SET_DOUBLE(...)

	#define HANDY_ATTRIBUTE_SET_STRING(...)

#endif

#endif // HANDYSTATS_ATTRIBUTE_MEASURING_POINTS_HPP_
