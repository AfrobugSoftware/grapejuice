#include "databasemysql.h"


boost::mysql::datetime pof::base::to_mysql_datetime(const pof::base::data::datetime_t& tt) {
	auto dt = std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration,
		pof::base::data::clock_t, std::chrono::system_clock::duration>(tt);
	return boost::mysql::datetime(dt);
}

boost::mysql::blob pof::base::to_mysql_uuid(const pof::base::data::duuid_t& duuid) {
	return boost::mysql::blob(duuid.begin(), duuid.end());
}


pof::base::databasemysql::databasemysql(boost::asio::io_context& ios, boost::asio::ssl::context& ssl)
	: m_resolver(boost::asio::make_strand(ios.get_executor())),  mIos(ios), mSsl(ssl){
}

boost::asio::awaitable<std::error_code> pof::base::databasemysql::connect(conn_ptr conn,std::string hostname, std::string port, std::string user, std::string pwd)
{
	if (hostname.empty() || port.empty() || user.empty() || pwd.empty())
			throw std::system_error(std::make_error_code(pof::base::errc::no_database_hostname));

	auto [ec1, endpoints] = co_await m_resolver.async_resolve(hostname, port, tuple_awaitable);
	if (ec1) 
		throw std::system_error(ec1);
	
	boost::mysql::handshake_params params{ user, pwd };
	auto [ec] = co_await conn->async_connect(*endpoints.begin(), params, tuple_awaitable);
	if (ec)
		throw std::system_error(ec);
	else {
		m_pool.push_back(conn);
	}
	co_return std::error_code{};
}

bool pof::base::databasemysql::push(std::shared_ptr<pof::base::query<databasemysql>> query)
{
	std::unique_lock<std::shared_mutex> lock(m_querymut);
	try {
		bool is_running = !m_queryque.empty();
		m_queryque.push_back(query);
		lock.unlock();
		if (!is_running) {
			auto sp = borrow(); //get a connection
			query->m_connection = sp;
			if(sp) boost::asio::co_spawn(sp->get_executor(), runquery(), boost::asio::detached);
		}
		else {
			spdlog::error("No connection to the database to borrow");
			m_queryque.pop_back(); //remove the query we added
		}
	}
	catch (...) {
		//what do we do here
	}
	return true;
}

void pof::base::databasemysql::setupssl()
{
}

boost::asio::awaitable<void> pof::base::databasemysql::runquery()
{
	for (;;) {
		if (m_isconnected) {
			std::unique_lock<std::shared_mutex> lock(m_querymut);
			if (m_queryque.empty()) break;

			std::shared_ptr<dataquerybase> dq = m_queryque.front();
			lock.unlock();
			std::error_code ec;
			try {
				//execute the query
				co_await(*dq)();
			}
			catch (...) {
				std::rethrow_exception(std::current_exception());
			}

			std::unique_lock<std::shared_mutex> lk(m_querymut);
			m_queryque.pop_front();
			if (m_queryque.empty()) break;
		}
		else {
			//need to suspend the corouine here
			boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
			timer.expires_after(std::chrono::seconds(1));

			auto[ec] = co_await timer.async_wait(tuple_awaitable);
		}
	}
}


void pof::base::databasemysql::disconnect()
{
}

pof::base::databasemysql::conn_ptr pof::base::databasemysql::borrow()
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	//no free connections
	if (m_pool.empty()) {
		//are theere any crashed connections listed as borrowed
		for (auto& sp : m_borrowed) {
			if (sp.use_count() <= 1) {
				//this connection has been abandoned 
				try {
					//co_spawn an ansync close
					spdlog::info("Creating a new connection to replace old");
					auto sp = create();
					m_borrowed.erase(sp);
					m_borrowed.insert(sp);
					return sp;

				}
				catch (std::exception& ec) {
					throw std::system_error(std::make_error_code(pof::base::errc::no_connection_avaliable));
				}
			}
		}
		//throw inavaliable 
		throw std::system_error(std::make_error_code(pof::base::errc::no_connection_avaliable));
	}
	auto sp = m_pool.front();
	m_pool.pop_front();
	m_borrowed.insert(sp);
	return sp;
}

void pof::base::databasemysql::unborrow(conn_ptr conn)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_pool.push_back(conn);
	m_borrowed.erase(conn);
}

pof::base::databasemysql::conn_ptr pof::base::databasemysql::create()
{
	return conn_ptr();
}
