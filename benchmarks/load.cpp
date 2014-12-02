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

#include <iostream>
#include <string>
#include <thread>
#include <future>
#include <vector>
#include <chrono>

#include <boost/program_options.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include <handystats/math_utils.hpp>
#include <handystats/core.hpp>
#include <handystats/metrics_dump.hpp>
#include <handystats/measuring_points.hpp>

#include <handystats/experimental/backends/file_logger.hpp>

#include "command_executor.hpp"

uint64_t threads = 1;
uint64_t timers = 0;
uint64_t counters = 0;
uint64_t gauges = 0;
std::vector<uint64_t> steps;
std::chrono::milliseconds output_interval = std::chrono::seconds(1);

std::atomic<uint64_t> rate(0);
std::atomic<int64_t> end_time(0);

// command() function is executed in multiple threads
// NOTE: rand() function may be not thread-safe.
void command() {
	// 0 .. counters .. counters + gauges .. counters + gauges + timers
	const double total_metrics = counters + gauges + timers;

	double counter_prob = double(counters) / total_metrics;
	double gauge_prob = double(counters + gauges) / total_metrics;
//	double timer_prob = 1.0;

	double choice = double(rand()) / RAND_MAX;

	if (handystats::math_utils::cmp(choice, counter_prob) < 0) {
		HANDY_COUNTER_INCREMENT(("load_test.counter.%d", rand() % counters), rand());
	}
	else if (handystats::math_utils::cmp(choice, gauge_prob) < 0) {
		HANDY_GAUGE_SET(("load_test.gauge.%d", rand() % gauges), rand());
	}
	else {
		if (rand() & 1) {
			HANDY_TIMER_START(("load_test.timer.%d", rand() % timers), std::hash<std::thread::id>()(std::this_thread::get_id()));
		}
		else {
			HANDY_TIMER_STOP(("load_test.timer.%d", rand() % timers), std::hash<std::thread::id>()(std::this_thread::get_id()));
		}
	}
}

void print_stats() {
	auto metrics_dump = HANDY_METRICS_DUMP();

	if (!metrics_dump || metrics_dump->first.empty() || metrics_dump->second.empty()) {
		return;
	}

	std::cout << "============= "
		<< " RPS: " << rate.load() << " "
		<< " ETR: " <<
			(
				handystats::chrono::nanoseconds(end_time.load()) -
				handystats::chrono::internal_clock::now().time_since_epoch()
			)
			.count(handystats::chrono::time_unit::SEC)
			<< "s "
		<< " =============" << std::endl;
	std::cout << "timestamp: "
		<< boost::get<int64_t>(
				metrics_dump->second.at("handystats.system_timestamp").value()
			) / 1000
		<< std::endl
		;
	std::cout << std::endl;

	{
		const char* name = "handystats.message_queue.size";
		const auto& message_queue_size = metrics_dump->first.at(name);
		std::cout << name << ":" << std::endl;
		std::cout << "         value: " << message_queue_size.get<handystats::statistics::tag::value>() << std::endl;
		std::cout << "    moving-avg: " << message_queue_size.get<handystats::statistics::tag::moving_avg>() << std::endl;
//		std::cout << "     timestamp: " << message_queue_size.get<handystats::statistics::tag::timestamp>() << std::endl;
		std::cout << std::endl;
	}

	{
		const char* name = "handystats.message_queue.pop_count";
		const auto& pop_count = metrics_dump->first.at(name);
		std::cout << name << ":" << std::endl;
		std::cout << "          rate: " << pop_count.get<handystats::statistics::tag::throughput>() << std::endl;
//		std::cout << "     timestamp: " << pop_count.get<handystats::statistics::tag::timestamp>() << std::endl;
		std::cout << std::endl;
	}

	{
		const char* name = "load_test.timer.0";
		const auto& timer = metrics_dump->first.at(name);
		std::cout << name << ":" << std::endl;
		std::cout << "          p25: " << timer.get<handystats::statistics::tag::quantile>().at(0.25) << std::endl;
		std::cout << "          p50: " << timer.get<handystats::statistics::tag::quantile>().at(0.50) << std::endl;
		std::cout << "          p75: " << timer.get<handystats::statistics::tag::quantile>().at(0.75) << std::endl;
		std::cout << "          p95: " << timer.get<handystats::statistics::tag::quantile>().at(0.95) << std::endl;
		std::cout << "    frequency: " << timer.get<handystats::statistics::tag::frequency>() << std::endl;
//		std::cout << "    timestamp: " << timer.get<handystats::statistics::tag::timestamp>() << std::endl;
		std::cout << std::endl;
	}
}

int main(int argc, char** argv) {
	namespace po = boost::program_options;
	po::options_description desc("Options");
	desc.add_options()
		("help", "Print help messages")
		("handystats-config", po::value<std::string>(),
			"Handystats configuration (in JSON format)"
		)
		("threads", po::value<uint64_t>(&threads)->default_value(threads),
			"Number of worker threads"
		)
		("step", po::value<std::vector<uint64_t>>(&steps)->multitoken()->required(),
			"Load step pairs (rate, seconds), e.g. --step 100 10 --step 200 10"
		)
		("timers", po::value<uint64_t>(&timers)->default_value(timers),
			"Number of different timers (0 for no timers)"
		)
		("counters", po::value<uint64_t>(&counters)->default_value(counters),
			"Number of different counters (0 for no counters)"
		)
		("gauges", po::value<uint64_t>(&gauges)->default_value(gauges),
			"Number of different gauges (0 for no gauges)"
		)
		("output-interval", po::value<uint64_t>()->default_value(output_interval.count()),
			"Stats output interval (in milliseconds)"
		)
		("log-file", po::value<std::string>(),
			"Metrics log filename (if not specified file_logger will not run)"
		)
		("log-period", po::value<uint64_t>(),
			"Metrics log period (in milliseconds, if not specified or zero file_logger will not run)"
		)
	;

	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		if (vm.count("help")) {
			std::cout << desc << std::endl;
			return 0;
		}
		po::notify(vm);
	}
	catch(po::error& e) {
		std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
		std::cerr << desc << std::endl;
		return 1;
	}

	if (vm["threads"].as<uint64_t>() == 0) {
		std::cerr << "ERROR: number of threads must be greater than 0" << std::endl;
		return 1;
	}

	if (!vm["timers"].as<uint64_t>() && !vm["counters"].as<uint64_t>() && !vm["gauges"].as<uint64_t>()) {
		std::cerr << "ERROR: at least one metric type must be specified (timers, counters, gauges)" << std::endl;
		return 1;
	}

	if (vm.count("handystats-config")) {
		if (!HANDY_CONFIG_JSON(vm["handystats-config"].as<std::string>().c_str())) {
			std::cerr << HANDY_CONFIG_ERROR() << std::endl;
			return 1;
		}
	}

	output_interval = std::chrono::milliseconds(vm["output-interval"].as<uint64_t>());

	HANDY_INIT();

	std::atomic<bool> stop_flag(false);
	std::thread stats_printer(
			[&stop_flag] () {
				while (!stop_flag.load()) {
					const auto& start_time = handystats::chrono::internal_clock::now();
					print_stats();
					const auto& end_time = handystats::chrono::internal_clock::now();

					const auto& call_time = (end_time - start_time).count(handystats::chrono::time_unit::NSEC);
					std::this_thread::sleep_for(output_interval - std::chrono::nanoseconds(call_time));
				}
			}
		);

	std::unique_ptr<handystats::backends::file_logger> file_logger;
	if (vm.count("log-file") && vm.count("log-period")) {
		std::string log_filename = vm["log-file"].as<std::string>();
		uint64_t log_period_ms = vm["log-period"].as<uint64_t>();
		if (log_period_ms > 0) {
			const auto& log_period = handystats::chrono::milliseconds(log_period_ms);
			file_logger.reset(new handystats::backends::file_logger(log_filename, log_period));
			if (!file_logger->run()) {
				std::cerr << "Unable to start file_logger: " << file_logger->m_error << std::endl;
				HANDY_FINALIZE();
				return 1;
			}
		}
	}

	std::vector<std::thread> workers(threads);
	for (size_t step_index = 0; step_index < steps.size(); step_index += 2) {
		uint64_t step_rate = steps[step_index];
		uint64_t step_time_limit = steps[step_index + 1];

		if (step_time_limit == 0) {
			continue;
		}

		rate.store(step_rate);
		end_time.store(
				(
					handystats::chrono::internal_clock::now() +
					handystats::chrono::seconds(step_time_limit)
				)
				.time_since_epoch()
				.count(handystats::chrono::time_unit::NSEC)
			);

		if (step_rate == 0) {
			std::this_thread::sleep_for(std::chrono::seconds(step_time_limit));
			continue;
		}

		handystats::chrono::duration command_time_limit =
			handystats::chrono::duration::convert_to(
					handystats::chrono::time_unit::NSEC,
					handystats::chrono::seconds(1)
				) * threads / step_rate;
		handystats::chrono::duration time_limit = handystats::chrono::seconds(step_time_limit);

		for (auto& worker : workers) {
			worker = std::thread(
					[command_time_limit, time_limit]
					() {
						handystats::benchmarks::command_executor::run_for(
							[] () {
								command();
							},
							command_time_limit,
							time_limit
						);
					}
				);
		}

		for (auto& worker : workers) {
			worker.join();
		}
	}

	stop_flag.store(true);
	stats_printer.join();

	if (file_logger) {
		file_logger->stop();
	}

	HANDY_FINALIZE();
}
