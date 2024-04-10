#include "databasemysql.h"

pof::base::databasemysql::databasemysql(boost::asio::io_context& ios, boost::asio::ssl::context& ssl)
	: m_resolver(boost::asio::make_strand(ios.get_executor())), 
	  m_connection(boost::asio::make_strand(ios.get_executor()), ssl), 
	  m_timer(ios.get_executor()) {
}

boost::asio::awaitable<std::error_code> pof::base::databasemysql::connect(std::string hostname, std::string port, std::string user, std::string pwd)
{
	spdlog::info("connecting to {} on port {}, user : {}, pwd {}", hostname, port, user, pwd);
	if (hostname.empty() || port.empty() || user.empty() || pwd.empty()) co_return std::make_error_code(pof::base::errc::no_database_hostname);
	auto [ec1, endpoints] = co_await m_resolver.async_resolve(hostname, port, tuple_awaitable);
	spdlog::info("{:ec}", std::error_code(ec1));
	if (ec1) co_return ec1;
	
	boost::mysql::handshake_params params{ user, pwd };
	auto [ec] = co_await m_connection.async_connect(*endpoints.begin(), params, tuple_awaitable);
	spdlog::info("{:ec}", std::error_code(ec));
	
	co_return ec;
}

bool pof::base::databasemysql::push(std::shared_ptr<pof::base::query<databasemysql>> query)
{
	std::unique_lock<std::shared_mutex> lock(m_querymut);
	bool is_running = !m_queryque.empty();
	m_queryque.push_back(query);
	lock.unlock();
	if (!is_running) {
		boost::asio::co_spawn(m_connection.get_executor(), runquery(), boost::asio::detached);
	}
	return true;
}

void pof::base::databasemysql::setupssl()
{
}

boost::asio::awaitable<void> pof::base::databasemysql::runquery()
{
	for (;;) {
		std::unique_lock<std::shared_mutex> lock(m_querymut);
		std::shared_ptr<dataquerybase> dq = m_queryque.front();
		lock.unlock();
		std::error_code ec;
		try {
			//execute the query
			co_await (*dq)();
		}
		catch (boost::system::system_error& err) {
			ec = err.code();
			//spdlog::error("{:ec}", ec);
		}
		catch (std::system_error& exp) {
			ec = exp.code();
			//spdlog::error("{:ec}", ec);
		}

		
		std::unique_lock<std::shared_mutex> lk(m_querymut);
		m_queryque.pop_front();
		if (m_queryque.empty()) break;
	}
}


void pof::base::databasemysql::disconnect()
{
}
