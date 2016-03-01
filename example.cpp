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

	std::cout << "func returned " << f.get() << "\n";
	work_ptr.reset();

	t.get();





}