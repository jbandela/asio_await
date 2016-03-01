#pragma once

#define BOOST_THREAD_VERSION 4
#include <boost/thread/future.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <tuple>
#include <utility>



namespace asio_await {

	struct use_boost_future_t {};
	constexpr use_boost_future_t use_boost_future;


	namespace detail {

		template<class... A>
		struct boost_promise_helper {

			template<int dummy, class... Types>
			struct promise_type {
				using type = std::tuple<A...>;
			};
			template<int dummy>
			struct promise_type<dummy> {
				using type = void;
			};
			template<int dummy, class T>
			struct promise_type<dummy, T> {
				using type = T;
			};


			using promise_value_type = typename promise_type<0, A...>::type;

			boost::promise<promise_value_type> value;

			boost_promise_helper(use_boost_future_t) {}
			boost_promise_helper(const boost_promise_helper&) {}
			boost_promise_helper(boost_promise_helper&&) = default;

			template<class P>
			static void helper(P& p) { p.set_value(); }

			template<class P, class T>
			static void helper(P& p, T&& t) { p.set_value(std::forward<T>(t)); }

			template<class P, class T, class U, class... R>
			static void helper(P& p, T&& t, U&& u, R&&... r) { p.set_value(std::tuple<A...>{std::forward<T>(t), std::forward<U>(u), std::forward<R>(r)...}); }



			template<class... T>
			void operator()(boost::system::error_code ec, T&&... t) {
				try {
					if (ec) {
						value.set_exception(boost::system::system_error{ ec });
					}
					else {
						helper(value, std::forward<T>(t)...);
					}
				}
				catch (std::exception&) {
					auto e = boost::current_exception();
					value.set_exception(e);

				}
			}


		};
	}
}

namespace boost {
	namespace asio {
		template<class... A>
		struct handler_type<asio_await::use_boost_future_t, void(boost::system::error_code, A...)> {
			using type = asio_await::detail::boost_promise_helper<A...>;

		};

		template<class... A>
		struct async_result<asio_await::detail::boost_promise_helper<A...>> {

			using type = boost::future<typename asio_await::detail::boost_promise_helper<A...>::promise_value_type>;

			type value;

			explicit async_result(asio_await::detail::boost_promise_helper<A...>& h) :value{ h.value.get_future() } {}

			auto get() {
				return std::move(value);
			}



		};

	}

}

namespace asio_await {
	template<class F, class YC>
	auto await_future(F&& f, YC&& yc) {
		using real_handler = typename boost::asio::handler_type<boost::asio::yield_context, void()>::type;
		real_handler handler{ std::forward<YC>(yc) };
		std::decay_t<F> retf;

		typename boost::asio::async_result<real_handler> ret{ handler };

		std::forward<F>(f).then([&retf, handler](auto f)mutable {
			retf = std::move(f);
			using boost::asio::asio_handler_invoke;
			asio_handler_invoke(handler, &handler);
		});

		ret.get();
		return retf;
	}

	template<class F, class YC>
	auto await(F&& f, YC&& yc) {
		return await_future(std::forward<F>(f), std::forward<YC>(yc)).get();
	}


	namespace detail {
		template<class P, class F, class YC>
		void set_promise_value(P&& p, F&& f, YC&& yc) {
			std::forward<P>(p).set_value(std::forward<F>(f)(std::forward<YC>(yc)));
		}
		template<class F, class YC>
		void set_promise_value(boost::promise<void>& p, F&& f, YC&& yc) {
			std::forward<F>(f)(std::forward<YC>(yc));
			p.set_value();
		}

	}

	template<class F>
	auto spawn_async(boost::asio::io_service& io, F f) {

		boost::promise<decltype(f(std::declval<boost::asio::yield_context>()))> ret{};
		auto fut = ret.get_future();
		auto func = [ret = std::move(ret), f = std::move(f)](boost::asio::yield_context yc) mutable{
			try {
				detail::set_promise_value(ret, f, yc);
			}
			catch (std::exception&) {
				ret.set_exception(std::current_exception());
			}
		};
		boost::asio::spawn(io, std::move(func));
		return fut;

	}
}

