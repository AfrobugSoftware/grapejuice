#include "Application.h"
static std::once_flag g_flag;
static std::shared_ptr<grape::Application> app;

grape::Application::Application(const std::string& servername)
: mServerName(servername){
	mDatabase = std::make_shared<pof::base::databasemysql>(mNetManager.io(), mNetManager.ssl());
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
	mDatabase->connect();

	mDatabase->create_database("grapejuice");
	mDatabase->use_database("grapejuice"s);

	//
	CreateTable(); //creates the tables
	CreateRoutes();

	//connect update signal
	mUpdateSignal.connect(std::bind_front(&grape::AccountManager::UpdateSessions,
		&mAccountManager));
	//read server configuration

	mNetManager.bind_addr(tcp::endpoint(tcp::v4(), 8080));
	mUpdateTimer = boost::asio::steady_timer(mNetManager.io().get_executor(), 30s);
	return false;
}

bool grape::Application::Run()
{
	mNetManager.run();
	return true;
}

bool grape::Application::Exit()
{
	mUpdateTimer->cancel();
	mDatabase->disconnect();
	mNetManager.stop();
	return false;
}

void grape::Application::CreateRoutes()
{
	mAccountManager.SetRoutes();
	mPharmacyManager.SetRoutes();
}

void grape::Application::CreateTable()
{
	mAccountManager.CreateAccountTable();
	mPharmacyManager.CreatePharmacyTable();
	mPharmacyManager.CreateBranchTable();
	mPharmacyManager.CreateAddressTable();

}

void grape::Application::route(const std::string& target, pof::base::net_manager::callback&& endpoint)
{
	mNetManager.add_route(target, 
		std::forward<pof::base::net_manager::callback>(endpoint));
}

void grape::Application::OnTimeout(boost::system::error_code ec)
{
	mUpdateSignal();
	if (mUpdateTimer){
		mUpdateTimer->expires_from_now(30s);
		mUpdateTimer->async_wait(std::bind_front(&grape::Application::OnTimeout, this));
	}
}

std::string grape::Application::ExtractString(pof::base::net_manager::req_t& req)
{
	const size_t len = boost::lexical_cast<size_t>(req.at(boost::beast::http::field::content_length));
	auto& req_body = req.body();
	std::string data;
	data.resize(len);

	auto buffer = req_body.data();
	boost::asio::buffer_copy(boost::asio::buffer(data), buffer);
	req_body.consume(len);

	return data;
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
