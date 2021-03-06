// Copyright (c) 2014 Yandex LLC. All rights reserved.

#ifndef HANDYSTATS_MEASURING_POINTS_COUNTER_PROXY_HPP_
#define HANDYSTATS_MEASURING_POINTS_COUNTER_PROXY_HPP_

#include <string>

#include <handystats/metrics/counter.hpp>

#include <handystats/measuring_points/counter.hpp>

namespace handystats { namespace measuring_points {

class counter_proxy {
public:
	/*
	 * Ctors without sending init event
	 */
	counter_proxy(const std::string& name)
		: name(name)
	{}

	counter_proxy(const char* name)
		: name(name)
	{}

	/*
	 * Ctors with init parameters
	 */
	counter_proxy(const std::string& name,
			const metrics::counter::value_type& init_value,
			const metrics::counter::time_point& timestamp = metrics::counter::clock::now()
		)
		: name(name)
	{
		HANDY_COUNTER_INIT(name.substr(), init_value, timestamp);
	}

	counter_proxy(const char* name,
			const metrics::counter::value_type& init_value,
			const metrics::counter::time_point& timestamp = metrics::counter::clock::now()
		)
		: name(name)
	{
		HANDY_COUNTER_INIT(this->name.substr(), init_value, timestamp);
	}

	/*
	 * Proxy init event
	 */
	void init(
			const metrics::counter::value_type& init_value = metrics::counter::value_type(),
			const metrics::counter::time_point& timestamp = metrics::counter::clock::now()
		)
	{
		HANDY_COUNTER_INIT(name.substr(), init_value, timestamp);
	}

	/*
	 * Proxy increment event
	 */
	void increment(
			const metrics::counter::value_type& value = 1,
			const metrics::counter::time_point& timestamp = metrics::counter::clock::now()
		)
	{
		HANDY_COUNTER_INCREMENT(name.substr(), value, timestamp);
	}

	/*
	 * Proxy decrement event
	 */
	void decrement(
			const metrics::counter::value_type& value = 1,
			const metrics::counter::time_point& timestamp = metrics::counter::clock::now()
		)
	{
		HANDY_COUNTER_DECREMENT(name.substr(), value, timestamp);
	}

	/*
	 * Proxy change event
	 */
	void change(
			const metrics::counter::value_type& value,
			const metrics::counter::time_point& timestamp = metrics::counter::clock::now()
		)
	{
		HANDY_COUNTER_CHANGE(name.substr(), value, timestamp);
	}

private:
	const std::string name;
};

}} // namespace handystats::measuring_points::proxy

#endif // HANDYSTATS_MEASURING_POINTS_COUNTER_PROXY_HPP_
