#include <netmanager.h>

pof::base::net_manager::net_manager()
	: m_ssl{boost::asio::ssl::context_base::tlsv12_server}{
	auto ec = setupssl();
	
	m_workgaurd = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(m_io.get_executor());
	m_threadvec.reserve(std::thread::hardware_concurrency());
	for (int i = 0; i < std::thread::hardware_concurrency() - 1; i++) {
		m_threadvec.emplace_back(std::move(std::thread{ static_cast<size_t(net::io_context::*)()>(&net::io_context::run), std::ref(m_io) }));
	}	
}

bool pof::base::net_manager::stop()
{
	m_workgaurd.reset(nullptr);
	m_io.stop();
	for(auto& t : m_threadvec)
		t.join();
	m_threadvec.clear();
	return true;
}

std::error_code pof::base::net_manager::setupssl()
{
	auto fp = std::filesystem::current_path() / "certs" / "certs.pem";

	//this would change
	try {
		boost::system::error_code ec;
		//
		//m_ssl.set_verify_mode(net::ssl::verify_none);
		load_test_certificate();

		return ec;
	}
	catch (const std::system_error& err) {
		return err.code();
	}
}

void pof::base::net_manager::load_test_certificate()
{
	/*
	   The certificate was generated from bash on Ubuntu (OpenSSL 1.1.1f) using:

	   openssl dhparam -out dh.pem 2048
	   openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 10000 -out cert.pem -subj "/C=US/ST=CA/L=Los Angeles/O=Beast/CN=www.example.com"
   */

	std::string const cert =
		"-----BEGIN CERTIFICATE-----\n"
		"MIIDlTCCAn2gAwIBAgIUOLxr3q7Wd/pto1+2MsW4fdRheCIwDQYJKoZIhvcNAQEL\n"
		"BQAwWjELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNBMRQwEgYDVQQHDAtMb3MgQW5n\n"
		"ZWxlczEOMAwGA1UECgwFQmVhc3QxGDAWBgNVBAMMD3d3dy5leGFtcGxlLmNvbTAe\n"
		"Fw0yMTA3MDYwMTQ5MjVaFw00ODExMjEwMTQ5MjVaMFoxCzAJBgNVBAYTAlVTMQsw\n"
		"CQYDVQQIDAJDQTEUMBIGA1UEBwwLTG9zIEFuZ2VsZXMxDjAMBgNVBAoMBUJlYXN0\n"
		"MRgwFgYDVQQDDA93d3cuZXhhbXBsZS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IB\n"
		"DwAwggEKAoIBAQCz0GwgnxSBhygxBdhTHGx5LDLIJSuIDJ6nMwZFvAjdhLnB/vOT\n"
		"Lppr5MKxqQHEpYdyDYGD1noBoz4TiIRj5JapChMgx58NLq5QyXkHV/ONT7yi8x05\n"
		"P41c2F9pBEnUwUxIUG1Cb6AN0cZWF/wSMOZ0w3DoBhnl1sdQfQiS25MTK6x4tATm\n"
		"Wm9SJc2lsjWptbyIN6hFXLYPXTwnYzCLvv1EK6Ft7tMPc/FcJpd/wYHgl8shDmY7\n"
		"rV+AiGTxUU35V0AzpJlmvct5aJV/5vSRRLwT9qLZSddE9zy/0rovC5GML6S7BUC4\n"
		"lIzJ8yxzOzSStBPxvdrOobSSNlRZIlE7gnyNAgMBAAGjUzBRMB0GA1UdDgQWBBR+\n"
		"dYtY9zmFSw9GYpEXC1iJKHC0/jAfBgNVHSMEGDAWgBR+dYtY9zmFSw9GYpEXC1iJ\n"
		"KHC0/jAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBzKrsiYywl\n"
		"RKeB2LbddgSf7ahiQMXCZpAjZeJikIoEmx+AmjQk1bam+M7WfpRAMnCKooU+Utp5\n"
		"TwtijjnJydkZHFR6UH6oCWm8RsUVxruao/B0UFRlD8q+ZxGd4fGTdLg/ztmA+9oC\n"
		"EmrcQNdz/KIxJj/fRB3j9GM4lkdaIju47V998Z619E/6pt7GWcAySm1faPB0X4fL\n"
		"FJ6iYR2r/kJLoppPqL0EE49uwyYQ1dKhXS2hk+IIfA9mBn8eAFb/0435A2fXutds\n"
		"qhvwIOmAObCzcoKkz3sChbk4ToUTqbC0TmFAXI5Upz1wnADzjpbJrpegCA3pmvhT\n"
		"7356drqnCGY9\n"
		"-----END CERTIFICATE-----\n";

	std::string const key =
		"-----BEGIN PRIVATE KEY-----\n"
		"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCz0GwgnxSBhygx\n"
		"BdhTHGx5LDLIJSuIDJ6nMwZFvAjdhLnB/vOTLppr5MKxqQHEpYdyDYGD1noBoz4T\n"
		"iIRj5JapChMgx58NLq5QyXkHV/ONT7yi8x05P41c2F9pBEnUwUxIUG1Cb6AN0cZW\n"
		"F/wSMOZ0w3DoBhnl1sdQfQiS25MTK6x4tATmWm9SJc2lsjWptbyIN6hFXLYPXTwn\n"
		"YzCLvv1EK6Ft7tMPc/FcJpd/wYHgl8shDmY7rV+AiGTxUU35V0AzpJlmvct5aJV/\n"
		"5vSRRLwT9qLZSddE9zy/0rovC5GML6S7BUC4lIzJ8yxzOzSStBPxvdrOobSSNlRZ\n"
		"IlE7gnyNAgMBAAECggEAY0RorQmldGx9D7M+XYOPjsWLs1px0cXFwGA20kCgVEp1\n"
		"kleBeHt93JqJsTKwOzN2tswl9/ZrnIPWPUpcbBlB40ggjzQk5k4jBY50Nk2jsxuV\n"
		"9A9qzrP7AoqhAYTQjZe42SMtbkPZhEeOyvCqxBAi6csLhcv4eB4+In0kQo7dfvLs\n"
		"Xu/3WhSsuAWqdD9EGnhD3n+hVTtgiasRe9318/3R9DzP+IokoQGOtXm+1dsfP0mV\n"
		"8XGzQHBpUtJNn0yi6SC4kGEQuKkX33zORlSnZgT5VBLofNgra0THd7x3atOx1lbr\n"
		"V0QizvCdBa6j6FwhOQwW8UwgOCnUbWXl/Xn4OaofMQKBgQDdRXSMyys7qUMe4SYM\n"
		"Mdawj+rjv0Hg98/xORuXKEISh2snJGKEwV7L0vCn468n+sM19z62Axz+lvOUH8Qr\n"
		"hLkBNqJvtIP+b0ljRjem78K4a4qIqUlpejpRLw6a/+44L76pMJXrYg3zdBfwzfwu\n"
		"b9NXdwHzWoNuj4v36teGP6xOUwKBgQDQCT52XX96NseNC6HeK5BgWYYjjxmhksHi\n"
		"stjzPJKySWXZqJpHfXI8qpOd0Sd1FHB+q1s3hand9c+Rxs762OXlqA9Q4i+4qEYZ\n"
		"qhyRkTsl+2BhgzxmoqGd5gsVT7KV8XqtuHWLmetNEi+7+mGSFf2iNFnonKlvT1JX\n"
		"4OQZC7ntnwKBgH/ORFmmaFxXkfteFLnqd5UYK5ZMvGKTALrWP4d5q2BEc7HyJC2F\n"
		"+5lDR9nRezRedS7QlppPBgpPanXeO1LfoHSA+CYJYEwwP3Vl83Mq/Y/EHgp9rXeN\n"
		"L+4AfjEtLo2pljjnZVDGHETIg6OFdunjkXDtvmSvnUbZBwG11bMnSAEdAoGBAKFw\n"
		"qwJb6FNFM3JnNoQctnuuvYPWxwM1yjRMqkOIHCczAlD4oFEeLoqZrNhpuP8Ij4wd\n"
		"GjpqBbpzyVLNP043B6FC3C/edz4Lh+resjDczVPaUZ8aosLbLiREoxE0udfWf2dU\n"
		"oBNnrMwwcs6jrRga7Kr1iVgUSwBQRAxiP2CYUv7tAoGBAKdPdekPNP/rCnHkKIkj\n"
		"o13pr+LJ8t+15vVzZNHwPHUWiYXFhG8Ivx7rqLQSPGcuPhNss3bg1RJiZAUvF6fd\n"
		"e6QS4EZM9dhhlO2FmPQCJMrRVDXaV+9TcJZXCbclQnzzBus9pwZZyw4Anxo0vmir\n"
		"nOMOU6XI4lO9Xge/QDEN4Y2R\n"
		"-----END PRIVATE KEY-----\n";

	std::string const dh =
		"-----BEGIN DH PARAMETERS-----\n"
		"MIIBCAKCAQEArzQc5mpm0Fs8yahDeySj31JZlwEphUdZ9StM2D8+Fo7TMduGtSi+\n"
		"/HRWVwHcTFAgrxVdm+dl474mOUqqaz4MpzIb6+6OVfWHbQJmXPepZKyu4LgUPvY/\n"
		"4q3/iDMjIS0fLOu/bLuObwU5ccZmDgfhmz1GanRlTQOiYRty3FiOATWZBRh6uv4u\n"
		"tff4A9Bm3V9tLx9S6djq31w31Gl7OQhryodW28kc16t9TvO1BzcV3HjRPwpe701X\n"
		"oEEZdnZWANkkpR/m/pfgdmGPU66S2sXMHgsliViQWpDCYeehrvFRHEdR9NV+XJfC\n"
		"QMUk26jPTIVTLfXmmwU0u8vUkpR7LQKkwwIBAg==\n"
		"-----END DH PARAMETERS-----\n";

	m_ssl.set_password_callback(
		[](std::size_t,
			boost::asio::ssl::context_base::password_purpose)
		{
			return "test";
		});

	m_ssl.set_options(
		boost::asio::ssl::context::default_workarounds |
		boost::asio::ssl::context::no_sslv2 |
		boost::asio::ssl::context::single_dh_use);

	m_ssl.use_certificate_chain(
		boost::asio::buffer(cert.data(), cert.size()));

	m_ssl.use_private_key(
		boost::asio::buffer(key.data(), key.size()),
		boost::asio::ssl::context::file_format::pem);

	m_ssl.use_tmp_dh(
		boost::asio::buffer(dh.data(), dh.size()));
}


pof::base::net_manager::res_t pof::base::net_manager::bad_request(const std::string& err) const
{
	res_t res{ http::status::bad_request, 11 };

	res.set(http::field::server, USER_AGENT_STRING);
	res.set(http::field::content_type, "application/json");
	res.keep_alive(true);

	js::json obj = js::json::object();
	obj["result_status"] = "failed"s;
	obj["result_message"] = err;



	auto ret = obj.dump();
	res_t::body_type::value_type value;
	value.resize(ret.size());
	std::copy(ret.begin(), ret.end(), value.begin());

	res.body() = value;
	res.prepare_payload();
	return res;
}

pof::base::net_manager::res_t pof::base::net_manager::server_error(const std::string& err) const
{
	res_t res{ http::status::internal_server_error, 11 };

	res.set(http::field::server, USER_AGENT_STRING);
	res.set(http::field::content_type, "application/json");
	res.keep_alive(true);

	js::json obj = js::json::object();
	obj["result_status"] = "failed"s;
	obj["result_message"] = err;


	auto ret = obj.dump();
	res_t::body_type::value_type value;
	value.resize(ret.size());
	std::copy(ret.begin(), ret.end(), value.begin());

	res.body() = value;
	res.prepare_payload();
	return res;
}

pof::base::net_manager::res_t pof::base::net_manager::not_found(const std::string& err) const
{
	res_t res{ http::status::not_found, 11 };

	res.set(http::field::server, USER_AGENT_STRING);
	res.set(http::field::content_type, "application/json");
	res.keep_alive(true);

	js::json obj = js::json::object();
	obj["result_status"] = "failed"s;
	obj["result_message"] = err;


	auto ret = obj.dump();

	res_t::body_type::value_type value;
	value.resize(ret.size());
	std::copy(ret.begin(), ret.end(), value.begin());


	res.body() = value;
	res.prepare_payload();
	return res;
}

pof::base::net_manager::res_t pof::base::net_manager::auth_error(const std::string& err) const
{
	res_t res{ http::status::unauthorized, 11 };
	res.set(http::field::server, USER_AGENT_STRING);
	res.set(http::field::content_type, "application/json");
	res.keep_alive(true);

	js::json obj = js::json::object();
	obj["result_status"] = "failed"s;
	obj["result_message"] = err;


	auto ret = obj.dump();
	res_t::body_type::value_type value;
	value.resize(ret.size());
	std::copy(ret.begin(), ret.end(), value.begin());



	res.body() = value;
	res.prepare_payload();
	return res;
}

pof::base::net_manager::res_t pof::base::net_manager::unprocessiable(const std::string& err) const
{
	res_t res{ http::status::unprocessable_entity, 11 };
	res.set(http::field::server, USER_AGENT_STRING);
	res.set(http::field::content_type, "application/json");
	res.keep_alive(true);

	js::json obj = js::json::object();
	obj["result_status"] = "failed"s;
	obj["result_message"] = err;


	auto ret = obj.dump();
	res_t::body_type::value_type value;
	value.resize(ret.size());
	std::copy(ret.begin(), ret.end(), value.begin());


	res.body() = value;
	res.prepare_payload();
	return res;
}

pof::base::net_manager::res_t pof::base::net_manager::timeout_error() const
{
	res_t res{ http::status::request_timeout, 11 };
	res.set(http::field::server, USER_AGENT_STRING);
	res.set(http::field::content_type, "application/json");
	res.keep_alive(true);

	js::json obj = js::json::object();
	obj["result_status"] = "failed"s;
	obj["result_message"] = "operation timeout";


	auto ret = obj.dump();
	res_t::body_type::value_type value;
	value.resize(ret.size());
	std::copy(ret.begin(), ret.end(), value.begin());

	res.body() = value;
	res.prepare_payload();
	return res;
}

void pof::base::net_manager::run()
{
	boost::make_shared<listener>(*this, m_endpoint)->run();
	m_signals = std::make_shared<net::signal_set>(m_io, SIGINT, SIGTERM);
	m_signals->async_wait(
		[&](boost::system::error_code const&, int)
		{
			// Stop the io_context. This will cause run()
			// to return immediately, eventually destroying the
			// io_context and any remaining handlers in it.
			m_io.stop();
		});
}

void pof::base::net_manager::add_route(const std::string& target, callback&& cb)
{
	if (target.empty()) return;
	m_router.insert(target, std::forward<callback>(cb));
}

void pof::base::net_manager::listener::fail(beast::error_code ec, char const* what)
{
	// Don't report on canceled operations
	if (ec == net::error::operation_aborted)
		return;
	spdlog::error("Listner error :{}", what);
}

void pof::base::net_manager::listener::on_accept(beast::error_code ec, tcp::socket socket)
{
	if (ec) return fail(ec, "accept");

	//lunch a session
	spdlog::info("connected at {}", socket.remote_endpoint().address().to_string());
	boost::make_shared<httpsession>(manager,std::move(socket))->run();

	//schelde another accept
	 // The new connection gets its own strand
	acceptor_.async_accept(net::make_strand(manager.io()),
		beast::bind_front_handler(&listener::on_accept, shared_from_this()));
}

pof::base::net_manager::listener::listener(net_manager& man, tcp::endpoint endpoint)
: manager(man), acceptor_(man.io()){
	beast::error_code ec;

	// Open the acceptor
	acceptor_.open(endpoint.protocol(), ec);
	if (ec){
		fail(ec, "open");
		return;
	}
	// Allow address reuse
	acceptor_.set_option(net::socket_base::reuse_address(true), ec);
	if (ec){
		fail(ec, "set_option");
		return;
	}
	// Bind to the server address
	acceptor_.bind(endpoint, ec);
	if (ec) {
		fail(ec, "bind");
		return;
	}
	// Start listening for connections
	acceptor_.listen(net::socket_base::max_listen_connections, ec);
	if (ec){
		fail(ec, "listen");
		return;
	}
}

void pof::base::net_manager::listener::run()
{
	// The new connection gets its own strand
	acceptor_.async_accept( net::make_strand(manager.io()), 
		beast::bind_front_handler(&listener::on_accept,
			shared_from_this()));
}

void pof::base::net_manager::httpsession::fail(beast::error_code ec, char const* what)
{
	// Don't report on canceled operations
	if (ec == net::error::operation_aborted)
		return;
	spdlog::error("httpsession error :{} {}", what, ec.message());
}

void pof::base::net_manager::httpsession::do_read()
{
	parser.emplace();
	parser->body_limit(10000);
	boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(60));

	// Read a request
	http::async_read(
		stream_,
		buffer_,
		parser->get(),
		beast::bind_front_handler(
			&httpsession::on_read,
			shared_from_this()));
}

void pof::base::net_manager::httpsession::on_read(beast::error_code ec, std::size_t)
{
	// This means they closed the connection
	if (ec == http::error::end_of_stream){
		boost::beast::get_lowest_layer(stream_).socket().shutdown(tcp::socket::shutdown_both, ec);
		return;
	}
	// Handle the error, if any
	if (ec)
		return fail(ec, "read");

	auto& req = parser->get();
	const auto& target = req.target();
	
	auto rpath = boost::urls::parse_path(target);
	if (!rpath) {
		auto res = manager.bad_request("illegal request taget");
		auto self = shared_from_this();
		using response_type = typename std::decay<decltype(res)>::type;
		auto sp = boost::make_shared<response_type>(std::forward<decltype(res)>(res));
		http::async_write(stream_, *sp,
			[self, sp](
				beast::error_code ec, std::size_t bytes)
			{
				self->on_write(ec, bytes, sp->need_eof());
			});
		return;
	}
	boost::beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

	boost::urls::matches m;
	auto found = manager.m_router.find(rpath.value(), m);
	if (!found) {
		http::response<http::string_body> res{ http::status::not_found, 11 };

		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/json");
		res.keep_alive(true);

		js::json obj = js::json::object();
		obj["result_status"] = "failed"s;
		obj["result_message"] = "The resource '" + std::string(target) + "' was not found.";

		res.body() = obj.dump();
		res.prepare_payload();

		using response_type = typename std::decay<decltype(res)>::type;
		auto sp = boost::make_shared<response_type>(std::forward<decltype(res)>(res));
		auto self = shared_from_this();
		http::async_write(stream_, *sp,
			[self, sp](
				beast::error_code ec, std::size_t bytes)
			{
				self->on_write(ec, bytes, sp->need_eof());
			});
	}
	else {
		boost::asio::co_spawn(stream_.get_executor(), (*found)(std::move(parser->get()), std::move(m)), [self = shared_from_this()](std::exception_ptr ptr, pof::base::net_manager::res_t res) {
			using response_type = typename std::decay<decltype(res)>::type;
		auto sp = boost::make_shared<response_type>(std::forward<decltype(res)>(res));
		http::async_write(self->stream_, *sp,
			[self = self->shared_from_this(), sp](
				beast::error_code ec, std::size_t bytes)
			{
				self->on_write(ec, bytes, sp->need_eof());
			});
		});
	}
}

void pof::base::net_manager::httpsession::on_write(beast::error_code ec, std::size_t, bool close)
{
	// Handle the error, if any
	if (ec)
		return fail(ec, "write");

	if (close)
	{
		// This means we should close the connection, usually because
		// the response indicated the "Connection: close" semantic.
		boost::beast::get_lowest_layer(stream_).socket().shutdown(tcp::socket::shutdown_send, ec);
		return;
	}

	// Read another request
	do_read();
}

pof::base::net_manager::httpsession::httpsession(net_manager& man, tcp::socket&& socket)
	: manager(man), stream_(std::move(socket), man.ssl()) {

}

void pof::base::net_manager::httpsession::on_handshake(beast::error_code ec)
{
	if (ec) {
		fail(ec, "handshake");
		return;
	}
	do_read(); //begin the reading
}

void pof::base::net_manager::httpsession::run()
{
	// Set the timeout.
	beast::get_lowest_layer(stream_).expires_after(
		std::chrono::seconds(30));


	// Perform the SSL handshake
	stream_.async_handshake(
		boost::asio::ssl::stream_base::server,
		beast::bind_front_handler(
			&httpsession::on_handshake,
			this->shared_from_this()));
}
