#include "Application.h"

grape::Application::Application(const std::string& servername)
{
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

	mNetManager.bind_addr(tcp::endpoint(tcp::v4(), 8080));
	return false;
}

bool grape::Application::Run()
{
	mNetManager.run();
	return true;
}

bool grape::Application::Exit()
{
	mDatabase->disconnect();
	mNetManager.stop();
	return false;
}

void grape::Application::route(const std::string& target, pof::base::net_manager::callback&& endpoint)
{
	mNetManager.add_route(target, 
		std::forward<pof::base::net_manager::callback>(endpoint));
}

std::shared_ptr<grape::Application> grape::GetApp()
{
	static std::shared_ptr<Application> app = std::make_shared<Application>(std::string("grapejuice"));
	return app;
}
