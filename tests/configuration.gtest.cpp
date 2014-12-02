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

#include <chrono>
#include <thread>
#include <map>
#include <string>

#include <gtest/gtest.h>

#include <handystats/core.hpp>
#include <handystats/metrics_dump.hpp>
#include <handystats/measuring_points.hpp>
#include <handystats/json_dump.hpp>

#include "config_impl.hpp"
#include "metrics_dump_impl.hpp"

#include "message_queue_helper.hpp"
#include "metrics_dump_helper.hpp"

class HandyConfigurationTest : public ::testing::Test {
protected:
	virtual void SetUp() {
	}
	virtual void TearDown() {
		HANDY_FINALIZE();
	}
};

TEST_F(HandyConfigurationTest, MetricsDumpConfiguration) {
	HANDY_CONFIG_JSON(
			"{\
				\"dump-interval\": 750\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);
	ASSERT_NEAR(handystats::config::opts->m_dump_interval.count(handystats::chrono::MSEC),
			750,
			1E-6
		);
}

TEST_F(HandyConfigurationTest, MetricsDumpToJsonTrueCheck) {
	HANDY_CONFIG_JSON(
			"{\
				\"dump-interval\": 2\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);
	ASSERT_NEAR(handystats::config::opts->m_dump_interval.count(handystats::chrono::MSEC),
			2,
			1E-6
		);

	HANDY_INIT();

	HANDY_GAUGE_SET("gauge.test", 15);

	handystats::wait_until_empty();
	handystats::metrics_dump::wait_until(handystats::chrono::system_clock::now());

	auto metrics_dump = HANDY_METRICS_DUMP();

	ASSERT_TRUE(metrics_dump->first.find("gauge.test") != metrics_dump->first.end());

	auto json_dump = HANDY_JSON_DUMP();

	ASSERT_FALSE(json_dump.empty());
}

TEST_F(HandyConfigurationTest, NoMetricsDumpCheck) {
	HANDY_CONFIG_JSON(
			"{\
				\"dump-interval\": 0\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);
	ASSERT_NEAR(handystats::config::opts->m_dump_interval.count(handystats::chrono::MSEC),
			0,
			1E-6
		);

	HANDY_INIT();

	HANDY_GAUGE_SET("gauge.test", 15);

	handystats::wait_until_empty();
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	auto metrics_dump = HANDY_METRICS_DUMP();

	ASSERT_TRUE(metrics_dump->first.empty());

	auto json_dump = HANDY_JSON_DUMP();

	ASSERT_EQ(json_dump, "{}");
}

TEST_F(HandyConfigurationTest, TimerConfigurationIdleTimeout) {
	HANDY_CONFIG_JSON(
			"{\
				\"dump-interval\": 2,\
				\"timer\": {\
					\"idle-timeout\": 100\
				}\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);
	ASSERT_NEAR(handystats::config::opts->m_timer.idle_timeout.count(handystats::chrono::MSEC),
			100,
			1E-6
		);

	HANDY_INIT();

	HANDY_TIMER_START("dead-timer");
	HANDY_TIMER_START("alive-timer");

	for (int cycle = 0; cycle < 100; ++cycle) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		HANDY_TIMER_HEARTBEAT("alive-timer");
	}

	HANDY_TIMER_STOP("dead-timer");
	HANDY_TIMER_STOP("alive-timer");

	handystats::wait_until_empty();
	handystats::metrics_dump::wait_until(handystats::chrono::system_clock::now());

	auto metrics_dump = HANDY_METRICS_DUMP();
	ASSERT_EQ(
			metrics_dump->first.at("dead-timer").get<handystats::statistics::tag::count>(),
			0
		);
	ASSERT_EQ(
			metrics_dump->first.at("alive-timer").get<handystats::statistics::tag::count>(),
			1
		);
}

TEST_F(HandyConfigurationTest, NoConfigurationUseDefaults) {
	HANDY_CONFIG_JSON(
			"{\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);
	ASSERT_EQ(
			handystats::config::opts->m_statistics.moving_interval.count(handystats::chrono::MSEC),
			handystats::config::statistics().moving_interval.count(handystats::chrono::MSEC)
		);
	ASSERT_EQ(handystats::config::opts->m_statistics.histogram_bins, handystats::config::statistics().histogram_bins);
	ASSERT_EQ(
			handystats::config::opts->m_timer.idle_timeout.count(handystats::chrono::MSEC),
			handystats::config::metrics::timer().idle_timeout.count(handystats::chrono::MSEC)
		);
}

TEST_F(HandyConfigurationTest, IncrementalStatisticsConfiguration) {
	HANDY_CONFIG_JSON(
			"{\
				\"defaults\": {\
					\"moving-interval\": 1234,\
					\"histogram-bins\": 200\
				}\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);
	ASSERT_NEAR(handystats::config::opts->m_statistics.moving_interval.count(handystats::chrono::MSEC),
			1234,
			1E-6
		);
	ASSERT_EQ(handystats::config::opts->m_statistics.histogram_bins, 200);
}

TEST_F(HandyConfigurationTest, EnableFalseConfigOption) {
	HANDY_CONFIG_JSON(
			"{\
				\"enable\": false,\
				\"dump-interval\": 1\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);
	ASSERT_FALSE(handystats::config::opts->m_core_enable);

	HANDY_INIT();

	for (int i = 0; i < 10; ++i) {
		HANDY_GAUGE_SET("test.gauge", i);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	ASSERT_TRUE(HANDY_METRICS_DUMP()->first.empty() && HANDY_METRICS_DUMP()->second.empty());
	ASSERT_EQ(HANDY_JSON_DUMP(), "{}");
}

TEST_F(HandyConfigurationTest, HistogramConfigOptionEnabled) {
	HANDY_CONFIG_JSON(
			"{\
				\"dump-interval\": 1,\
				\"defaults\": {\
					\"histogram-bins\": 25,\
					\"moving-interval\": 1000,\
					\"stats\": [\"histogram\"]\
				}\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);
	ASSERT_EQ(handystats::config::opts->m_statistics.histogram_bins, 25);
	ASSERT_EQ(handystats::config::opts->m_gauge.values.histogram_bins, 25);
	ASSERT_EQ(handystats::config::opts->m_gauge.values.tags, handystats::statistics::tag::histogram);

	HANDY_INIT();

	for (int i = 0; i < 10; ++i) {
		HANDY_GAUGE_SET("test.gauge", i);
	}

	for (int i = 0; i < 100; ++i) {
		HANDY_COUNTER_INCREMENT("test.counter", i);
	}

	handystats::wait_until_empty();
	handystats::metrics_dump::wait_until(handystats::chrono::system_clock::now());

	ASSERT_FALSE(HANDY_METRICS_DUMP()->first.empty() || HANDY_METRICS_DUMP()->second.empty());
	auto metrics_dump = HANDY_METRICS_DUMP();

	auto gauge_values = metrics_dump->first.at("test.gauge");

	ASSERT_TRUE(gauge_values.enabled(handystats::statistics::tag::histogram));
	ASSERT_EQ(gauge_values.get<handystats::statistics::tag::histogram>().size(), 10);

	auto counter_values = metrics_dump->first.at("test.counter");

	ASSERT_TRUE(counter_values.enabled(handystats::statistics::tag::histogram));
	ASSERT_EQ(counter_values.get<handystats::statistics::tag::histogram>().size(), 25);
}

TEST_F(HandyConfigurationTest, HistogramConfigOptionDisabled) {
	HANDY_CONFIG_JSON(
			"{\
				\"dump-interval\": 1,\
				\"defaults\": {\
					\"histogram-bins\": 20,\
					\"stats\": [],\
					\"moving-interval\": 1000\
				}\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);
	ASSERT_EQ(handystats::config::opts->m_statistics.histogram_bins, 20);

	HANDY_INIT();

	for (int i = 0; i < 10; ++i) {
		HANDY_GAUGE_SET("test.gauge", i);
	}

	handystats::wait_until_empty();
	handystats::metrics_dump::wait_until(handystats::chrono::system_clock::now());

	ASSERT_FALSE(HANDY_METRICS_DUMP()->first.empty() && HANDY_METRICS_DUMP()->second.empty());
	auto metrics_dump = HANDY_METRICS_DUMP();

	auto gauge_values = metrics_dump->first.at("test.gauge");

	ASSERT_FALSE(gauge_values.computed(handystats::statistics::tag::histogram));
}

TEST_F(HandyConfigurationTest, MetricsConfigOverwritesStatistcs) {
	HANDY_CONFIG_JSON(
			"{\
				\"dump-interval\": 1,\
				\"defaults\": {\
					\"histogram-bins\": 50,\
					\"stats\": [\"histogram\"],\
					\"moving-interval\": 1000\
				},\
				\"gauge\": {\
					\"stats\": []\
				}\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);
	ASSERT_EQ(handystats::config::opts->m_statistics.histogram_bins, 50);

	HANDY_INIT();

	for (int i = 0; i < 10; ++i) {
		HANDY_GAUGE_SET("test.gauge", i);
	}

	handystats::wait_until_empty();
	handystats::metrics_dump::wait_until(handystats::chrono::system_clock::now());

	ASSERT_FALSE(HANDY_METRICS_DUMP()->first.empty() && HANDY_METRICS_DUMP()->second.empty());
	auto metrics_dump = HANDY_METRICS_DUMP();

	auto gauge_values = metrics_dump->first.at("test.gauge");

	ASSERT_FALSE(gauge_values.computed(handystats::statistics::tag::histogram));
}

TEST_F(HandyConfigurationTest, PatternConfiguration) {
	HANDY_CONFIG_JSON(
			"{\
				\"dump-interval\": 1,\
				\"defaults\": {\
					\"moving-interval\": 1000,\
					\"stats\": [\"histogram\"]\
				},\
				\"*.gauge\": {\
					\"histogram-bins\": 15\
				},\
				\"*.counter\": {\
					\"histogram-bins\": 25\
				}\
			}"
		);

	ASSERT_FALSE(handystats::config::opts.use_count() == 0);

	HANDY_INIT();

	for (int i = 0; i < 100; ++i) {
		HANDY_GAUGE_SET("test.gauge", i);
	}

	for (int i = 0; i < 100; ++i) {
		HANDY_COUNTER_INCREMENT("test.counter", i);
	}

	handystats::wait_until_empty();
	handystats::metrics_dump::wait_until(handystats::chrono::system_clock::now());

	ASSERT_FALSE(HANDY_METRICS_DUMP()->first.empty() || HANDY_METRICS_DUMP()->second.empty());
	auto metrics_dump = HANDY_METRICS_DUMP();

	auto gauge_values = metrics_dump->first.at("test.gauge");

	ASSERT_TRUE(gauge_values.enabled(handystats::statistics::tag::histogram));
	ASSERT_EQ(gauge_values.get<handystats::statistics::tag::histogram>().size(), 15);

	auto counter_values = metrics_dump->first.at("test.counter");

	ASSERT_TRUE(counter_values.enabled(handystats::statistics::tag::histogram));
	ASSERT_EQ(counter_values.get<handystats::statistics::tag::histogram>().size(), 25);
}

TEST_F(HandyConfigurationTest, InvalidConfiguration) {
	ASSERT_FALSE(
			HANDY_CONFIG_JSON(
				"{\
					\"dump-interval\": -750\
				}"
			)
		);
	std::cerr << HANDY_CONFIG_ERROR() << std::endl;

	ASSERT_FALSE(
			HANDY_CONFIG_JSON(
				"{\
					\"enable\": 1\
				}"
			)
		);
	std::cerr << HANDY_CONFIG_ERROR() << std::endl;

	ASSERT_FALSE(
			HANDY_CONFIG_JSON(
				"{\
					\"gauge\": 1\
				}"
			)
		);
	std::cerr << HANDY_CONFIG_ERROR() << std::endl;

	ASSERT_FALSE(
			HANDY_CONFIG_JSON(
				"{\
					\"timer\": {\
						\"idle-timeout\": \"1s\"\
					}\
				}"
			)
		);
	std::cerr << HANDY_CONFIG_ERROR() << std::endl;

	ASSERT_FALSE(
			HANDY_CONFIG_JSON(
				"{\
					\"timer\": {\
						\"idle-timeout\": 10000,\
						\"stats\": [\"invalid-statistic\"]\
					}\
				}"
			)
		);
	std::cerr << HANDY_CONFIG_ERROR() << std::endl;

	ASSERT_FALSE(
			HANDY_CONFIG_JSON(
				"{\
					\"timer\": {\
						\"idle-timeout\": 10000,\
						\"stats\": \"throughput\"\
					}\
				}"
			)
		);
	std::cerr << HANDY_CONFIG_ERROR() << std::endl;

	ASSERT_FALSE(
			HANDY_CONFIG_FILE("surely-invalid-config-file.json")
		);
	std::cerr << HANDY_CONFIG_ERROR() << std::endl;
}
