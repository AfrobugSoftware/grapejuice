#pragma once 
#include <set>
#include <list>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>

#include <spdlog/spdlog.h>

#include "system.h"

#include <boost/mysql.hpp>


namespace filo{
	/**
	 * Represents a pool of connections in the system
	 * Threads request for connections from the pool and return them back on finish
	 * */
	class conn_pool{
	public:
		struct stat{
			size_t pool_size;
			size_t borrowed_size;
		};

		using value_t = boost::mysql::field;
		using data_t = std::vector<boost::mysql::row>;
		using conn_ptr = std::shared_ptr<boost::mysql::tcp_ssl_connection>;
		constexpr const static size_t spool_size = 20;

		conn_pool(size_t pool_size);
		virtual ~conn_pool();
		stat stats();
		conn_ptr borrow();
		void unborrow(conn_ptr conn);



	private:
		void err_callback(conn_ptr, const std::error_code& ec);
		conn_ptr create();
		size_t m_pool_size;

		std::mutex m_mutex;
		std::set<conn_ptr> m_borrowed;
		std::list<conn_ptr> m_pool;


	};

	extern filo::conn_pool& get_conn_pool(); 
};