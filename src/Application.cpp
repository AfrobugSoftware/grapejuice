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

bool grape::Application::Init()
{
	mDatabase->set_params(
		"localhost"s,
		"3306"s,
		"root"s,
		"Topdollar123"s
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
	mNetManager.bind_addr(tcp::endpoint(tcp::v4(), 8080));
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
}

void grape::Application::CreateTable()
{
	mAccountManager.CreateAccountTable();
	mPharmacyManager.CreateTables();
	mProductManager.CreateTables();
	mProductManager.Procedures();
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

boost::asio::awaitable<void> grape::Application::RunUpdateTimer()
{
	mUpdateTimer = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
	for (;;){
		mUpdateTimer->expires_after(std::chrono::milliseconds(30));
		auto&& [ec] = co_await mUpdateTimer->async_wait();
		if (ec == boost::asio::error::operation_aborted) break;

		for (const auto& v : mUpdateAsyncFuncs) {
			co_await v();
		}
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
