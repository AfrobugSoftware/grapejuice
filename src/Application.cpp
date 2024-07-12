#include "Application.h"
static std::once_flag g_flag;
static std::shared_ptr<grape::Application> app;

grape::Application::Application(const std::string& servername)
: mServerName(servername){
	mDatabase = std::make_shared<pof::base::databasemysql>(mNetManager.io());
}

grape::Application::~Application()
{
}

bool grape::Application::Init(int argc, char** argv)
{
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "display help message")
		("sqlhostname,shn", po::value<std::string>()->default_value("root"), "set sql host name")
		("sqlpass,sp", po::value<std::string>()->default_value("Topdollar123"), "set sql pass")
		("sqladdress,sa", po::value<std::string>()->default_value("localhost"), "set sql address")
		("sqlport,sport", po::value<std::string>()->default_value("3306"), "set sql port")
		("serverport,port", po::value<std::uint16_t>()->default_value(8080), "set server port");
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	mDatabase->set_params(
		vm["sqladdress"].as<std::string>(),
		vm["sqlport"].as<std::string>(),
		vm["sqlhostname"].as<std::string>(),
		vm["sqlpass"].as<std::string>()
	);

	mDatabase->create_pool();
	bool isConnected = mDatabase->connect();
	if (isConnected) {
		mDatabase->create_database("grapejuice");
		mDatabase->use_database("grapejuice"s);

		//
		CreateTable(); //creates the tables
	}

	//read server configuration
	mNetManager.bind_addr(tcp::endpoint(tcp::v4(), vm["serverport"].as<std::uint16_t>()));
	CreateRoutes();


	//setup update
	mUpdateAsyncFuncs.emplace_back(std::bind_front(&grape::AccountManager::UpdateSessions, &mAccountManager));

	boost::asio::co_spawn(mNetManager.io().get_executor(),
		RunUpdateTimer(), boost::asio::detached);
	return true;
}

bool grape::Application::Run()
{
	mNetManager.run();
	return true;
}

bool grape::Application::Exit()
{
	if(mUpdateTimer.has_value()) mUpdateTimer->cancel();
	mDatabase->disconnect();
	mNetManager.stop();
	return false;
}

void grape::Application::CreateRoutes()
{
	mAccountManager.SetRoutes();
	mPharmacyManager.SetRoutes();
	mProductManager.SetRoutes();
	SetRoutes();
}

void grape::Application::CreateTable()
{
	mAccountManager.CreateAccountTable();
	mPharmacyManager.CreateTables();
	mProductManager.CreateTables();
	mProductManager.Procedures();
	CreateAppDetailsTable();
}

void grape::Application::CreateAppDetailsTable()
{
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(mDatabase,
			R"(CREATE TABLE IF NOT EXISTS app_details (
				id binary(16),
				app_address binary(16),
				name text,
				version text,
				os text,
				locale text,
				installed_date datetime,
				last_update datetime,
				last_ping datetime
			);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);

		(void)fut.get();
	}
	catch (boost::mysql::error_with_diagnostics& err) {
		spdlog::error(err.what());
	}
}

void grape::Application::route(const std::string& target, pof::base::net_manager::callback&& endpoint)
{
	mNetManager.add_route(target, 
		std::forward<pof::base::net_manager::callback>(endpoint));
}

std::string grape::Application::ExtractString(pof::base::net_manager::req_t& req)
{
	auto& body = req.body();
	std::string s(reinterpret_cast<const char*>(body.data()), body.size());
	return s;
}

grape::response grape::Application::OkResult(const std::string& message, 
		const std::string& status, bool keep_alive)
{
	grape::response res{ http::status::ok, 11 };
	res.set(http::field::server, USER_AGENT_STRING);
	res.set(http::field::content_type, "application/octlet-stream");
	res.keep_alive(keep_alive);

	grape::result result;
	result.message = message;
	result.status = status;

	grape::response::body_type::value_type value(grape::serial::get_size(result), 0x00);
	grape::serial::write(boost::asio::buffer(value), result);

	res.body() = std::move(value);
	res.prepare_payload();
	return res;
}

void grape::Application::SetRoutes()
{
	route("/app/ping", std::bind_front(&grape::Application::onAppPing, this));
	route("/app/checkupdate", std::bind_front(&grape::Application::onAppCheckUpdate, this));
	route("/app/update", std::bind_front(&grape::Application::onAppUpdate, this));
	route("/app/getoffice", std::bind_front(&grape::Application::onGetOffice, this));
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::Application::onAppPing(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	try {
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}
		auto& body = req.body();
		auto&& [app_details, buf] = grape::serial::read<grape::app_details>(boost::asio::buffer(body));
		if (app_details.app_id == boost::uuids::nil_uuid()) {
			//first time ping
			auto&& [address, buf2] = grape::serial::read<grape::address>(buf);

			//should be the same address created for the pharmacy from setup
			app_details.app_id = boost::uuids::random_generator_mt19937{}();
			app_details.app_install_location_id = address.id;
			app_details.app_last_update = std::chrono::system_clock::now();
			app_details.app_installed_date = app_details.app_last_update;
		}
		app_details.app_last_ping = std::chrono::system_clock::now();
		
		auto query = std::make_shared<pof::base::datastmtquery>(mDatabase, 
			R"(INSERT INTO app_details VALUES (?,?,?,?,?,?,?,?,?);)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(60s);
		query->m_arguments = { {
				boost::mysql::field(boost::mysql::blob(app_details.app_id.begin(), app_details.app_id.end())),
				boost::mysql::field(boost::mysql::blob(app_details.app_install_location_id.begin(), app_details.app_install_location_id.end())),
				boost::mysql::field(app_details.app_name),
				boost::mysql::field(app_details.app_version),
				boost::mysql::field(app_details.os),
				boost::mysql::field(app_details.locale),
				boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(app_details.app_installed_date))),
				boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(app_details.app_last_update))),
				boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(app_details.app_last_ping)))
		}};

		auto fut = query->get_future();
		bool tried = app->mDatabase->push(query);
		if (!tried) {
			tried = co_await app->mDatabase->retry(query); //try to push into the queue multiple times
			if (!tried) {
				co_return app->mNetManager.server_error("Error in query");
			}
		}
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			//the operation my have completed, but we timed out in waiting, how do we resolve this?
			co_return app->mNetManager.timeout_error();
		}

		(void)fut.get();

		co_return OkResult(app_details, req.keep_alive());
	}
	catch (std::exception& exp) {
		co_return mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::Application::onAppCheckUpdate(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	try {
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}
		auto& body = req.body();
		auto&& [app_details, buf] = grape::serial::read<grape::app_details>(boost::asio::buffer(body));
		
		//file that descripes the update
		auto distropath = fs::current_path() / "distro" / "update.json";
		if (!fs::exists(distropath)) {
			co_return OkResult("No app to update");
		}
		std::fstream file(distropath, std::ios::in);
		if (!file.is_open()) {
			co_return mNetManager.server_error("Error in reading update file");
		}
		std::ostringstream os;
		os << file.rdbuf();

		js::json appObj = js::json::parse(os.str());
		std::string ver = static_cast<std::string>(appObj["version"]);
		std::string_view version = ver;
		std::string_view app_version = app_details.app_version;
		
		size_t pos = 0;
		size_t pos_prev = 0;
		size_t pos2 = 0;
		size_t pos2_prev = 0;
		std::string_view token;
		std::string_view token2;

		//major and minor
		while ((pos = version.find(".", pos)) != std::string::npos && 
			 (pos2 = app_version.find(".", pos2)) != std::string::npos) {
			token = version.substr(pos_prev, pos - pos_prev);
			token2 = app_version.substr(pos_prev, pos2 - pos2_prev);
			if (boost::lexical_cast<size_t>(token) >
				boost::lexical_cast<size_t>(token2)){
				co_return OkResult("UpdateReady");
			}
			pos_prev = ++pos;
			pos2_prev = ++pos2;
		}
		//patch
		if (boost::lexical_cast<size_t>(version.substr(pos_prev, version.size() - pos_prev)) >
			boost::lexical_cast<size_t>(app_version.substr(pos2_prev, app_version.size() - pos2_prev))) {
			co_return OkResult("UpdateReady");
		}
		co_return OkResult("NoUpdate");
	}
	catch (std::exception& exp) {
		co_return mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::Application::onAppUpdate(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	try {
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}
		auto& body = req.body();
		auto&& [app_details, buf] = grape::serial::read<grape::app_details>(boost::asio::buffer(body));
		if (app_details.os == "Windows") {
			//file that descripes the update
			auto distropath = fs::current_path() / "distro" / "update.json";
			if (!fs::exists(distropath)) {
				co_return OkResult("No app to update");
			}
			std::fstream file(distropath, std::ios::in);
			if (!file.is_open()) {
				co_return mNetManager.server_error("Error in reading update file");
			}
			std::ostringstream os;
			os << file.rdbuf();
			file.close();

			js::json appObj = js::json::parse(os.str());
			app_details.app_version = static_cast<std::string>(appObj["version"]);
			app_details.app_last_update = std::chrono::system_clock::now();


			auto pofile = fs::current_path() / "distro" / static_cast<std::string>(appObj["app_name"]);
			std::string name = pofile.filename().string();
			if (!fs::exists(pofile)) {
				co_return OkResult("No app to update");
			}
			file.open(pofile, std::ios::in);
			if (!file.is_open()) {
				co_return mNetManager.server_error("Error in processing update file");
			}
			file.seekg(0, std::ios::end);
			size_t size = file.tellg();
			file.seekg(0, std::ios::beg);

			grape::response::body_type::value_type value(grape::serial::get_size(app_details) + sizeof(std::uint32_t) * 2 + size
				+ name.size(), 0x00);
			auto mbuf = grape::serial::write(boost::asio::buffer(value), app_details);

			*boost::asio::buffer_cast<std::uint32_t*>(mbuf) = name.size();
			mbuf += sizeof(std::uint32_t);

			std::copy(name.begin(), name.end(), mbuf.data());
			mbuf += name.size();

			*boost::asio::buffer_cast<std::uint32_t*>(mbuf) = size;
			mbuf += sizeof(std::uint32_t);

			//can only work with small files
			file.read(reinterpret_cast<char*>(mbuf.data()), size);
			mbuf += size;
			file.close();

			grape::response res{ http::status::ok, 11 };
			res.set(http::field::server, USER_AGENT_STRING);
			res.set(http::field::content_type, "application/octlet-stream");
			res.keep_alive(req.keep_alive());

			res.body() = std::move(value);
			res.prepare_payload();
			co_return res;

		}
		else {
			//only windows is support
			co_return mNetManager.bad_request("Can only support windows");
		}
	}
	catch (const std::exception& exp) {
		co_return mNetManager.server_error(exp.what());
	}


}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::Application::onGetOffice(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	try {
		auto distropath = fs::current_path() / "distro" / "update.json";
		if (!fs::exists(distropath)) {
			co_return OkResult("No app to update");
		}
		std::fstream file(distropath, std::ios::in);
		if (!file.is_open()) {
			co_return mNetManager.server_error("Error in reading update file");
		}
		std::ostringstream os;
		os << file.rdbuf();
		file.close();

		js::json appObj = js::json::parse(os.str());

		auto pofile = fs::current_path() / "distro" / static_cast<std::string>(appObj["app_name"]);
		std::string name = pofile.filename().string();
		if (!fs::exists(pofile)) {
			co_return OkResult("No app to update");
		}
		file.open(pofile, std::ios::in);
		if (!file.is_open()) {
			co_return mNetManager.server_error("Error in processing update file");
		}
		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		file.seekg(0, std::ios::beg);
		grape::response::body_type::value_type value(size, 0x00);

		file.read(reinterpret_cast<char*>(value.data()), size);
		file.close();

		grape::response res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/x-msdownload");
		res.set("Content-Disposition", boost::beast::string_view(fmt::format("attachment; filename={}", name)));
		res.keep_alive(req.keep_alive());

		res.body() = std::move(value);
		res.prepare_payload();
		co_return res;
	}
	catch (const std::exception& exp){
		co_return mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<void> grape::Application::RunUpdateTimer()
{
	try {
		mUpdateTimer = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		for (;;) {
			mUpdateTimer->expires_after(std::chrono::milliseconds(30));
			auto&& [ec] = co_await mUpdateTimer->async_wait();
			if (ec == boost::asio::error::operation_aborted) break;

			for (const auto& v : mUpdateAsyncFuncs) {
				co_await v();
			}
		}
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
	}
}

bool grape::VerifyEmail(const std::string& email)
{
	const std::regex rex(R"(^[\w-\.]+@([\w-]+\.)+[\w-]{2,4}$)");
	return (std::regex_match(email, rex));
}

bool grape::VerifyPhonenumber(const std::string& phone)
{
	const std::regex pattern("(0|91)?[6-9][0-9]{9}");
	return (std::regex_match(phone, pattern));
}

std::shared_ptr<grape::Application> grape::GetApp()
{
	std::call_once(g_flag, [&]() {
		if (!app) {
		app = std::make_shared<grape::Application>("grapejuice"s);
		}
	});
	return app;
}
