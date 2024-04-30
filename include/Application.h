#pragma once
#include <crow/app.h>
#include <boost/noncopyable.hpp>
#include <memory>


#include "netmanager.h"
#include "databasemysql.h"
namespace grape {
	class Application : public boost::noncopyable, 
		public std::enable_shared_from_this<Application>
	{
	public:
		Application(const std::string& servername);
		virtual ~Application();

		virtual bool Init();
		virtual bool Run();
		virtual bool Exit();


		std::shared_ptr<crow::SimpleApp> mRouteApp;
		std::shared_ptr<pof::base::databasemysql> mDatabase;


		pof::base::net_manager mNetManager;
	};

	extern std::shared_ptr<Application> GetApp();
};
