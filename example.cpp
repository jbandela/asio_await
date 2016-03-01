//          Copyright John R. Bandela 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <iostream>
#include <boost/asio/steady_timer.hpp>
#include "asio_await.hpp"

int main() {

	boost::asio::io_service io;

	auto func = [&](boost::asio::yield_context yc) {
		boost::asio::steady_timer timer{ io };
		timer.expires_from_now(std::chrono::seconds{ 5 });

		auto fut = timer.async_wait(asio_await::use_boost_future);
		std::cout << "Got future\n";

		asio_await::await_future(fut, yc);

		std::cout << "Done with timer\n";
		return 1;
	};


	auto work_ptr = std::make_unique<boost::asio::io_service::work>(io);

	auto f = asio_await::spawn_async(io, func);
	auto t = boost::async(boost::launch::async, [&]() {io.run();});

	auto val = f.get();
	std::cout << "func returned " << val << "\n";
	work_ptr.reset();

	t.get();





}
