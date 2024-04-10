#pragma once
#pragma once
#ifdef  WIN32
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif //  WIN32

#include <boost/mysql.hpp>
#include <boost/noncopyable.hpp>



#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/as_tuple.hpp>


#include <spdlog/spdlog.h>
#include <memory>
#include <deque>
#include <shared_mutex>
#include <chrono>
#include <atomic>
#include <condition_variable>

#include "query.h"
#include "errc.h"




using namespace boost::asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;
constexpr auto tuple_awaitable = boost::asio::as_tuple(boost::asio::use_awaitable);
namespace pof {
	namespace base {
		class databasemysql : private boost::noncopyable
		{
		public:
			using connection_t = boost::mysql::tcp_ssl_connection;
			databasemysql(boost::asio::io_context& ios, boost::asio::ssl::context& ssl);

			boost::asio::awaitable<std::error_code> connect(std::string hostname, 
			std::string port,
			std::string user, std::string pwd);
			inline connection_t& connection() { return m_connection; }
			//Adds a query to the queue
			bool push(std::shared_ptr<pof::base::query<databasemysql>> query);

			void setupssl();
			boost::asio::awaitable<void> runquery();


			//also closes 
			void disconnect();

		private:
			std::shared_mutex m_querymut;
			std::deque<std::shared_ptr<pof::base::query<databasemysql>>> m_queryque;

			std::atomic<bool> m_isrunning;
			std::condition_variable mStmtConditionVarible;

			boost::asio::steady_timer m_timer;
			connection_t m_connection;
			boost::asio::ip::tcp::resolver m_resolver;
		};

		using dataquerybase = pof::base::query<databasemysql>;

		template<typename... Args>
		using datastmtquery = pof::base::querystmt<databasemysql, Args...>;
	}
};