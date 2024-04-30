#include "Application.h"

grape::Application::Application(const std::string& servername)
{
	mRouteApp = std::make_shared<crow::SimpleApp>();
	mRouteApp->server_name(servername);
	mDatabase = std::make_shared<pof::base::databasemysql>(mNetManager.io(), mNetManager.ssl());
}

grape::Application::~Application()
{
	mNetManager.stop();
}

bool grape::Application::Init()
{
	return false;
}

bool grape::Application::Run()
{
	return false;
}

bool grape::Application::Exit()
{
	return false;
}

std::shared_ptr<grape::Application> grape::GetApp()
{
	static std::shared_ptr<Application> app = std::make_shared<Application>(std::string("grapejuice"));
	return app;
}
