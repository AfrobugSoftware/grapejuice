#pragma once
#include <crow/app.h>
#include <boost/noncopyable.hpp>
#include <memory>


#include "netmanager.h"
#include "databasemysql.h"
namespace grape {
	class Manager : public std::enable_shared_from_this<Manager> {
	public:
		Manager();
		virtual ~Manager() {};
	};

	class Application : public boost::noncopyable, 
		public std::enable_shared_from_this<Application>
	{
	public:
		Application(const std::string& servername);
		virtual ~Application();

		virtual bool Init();
		virtual bool Run();
		virtual bool Exit();

		void route(const std::string& target,
			pof::base::net_manager::callback&& endpoint);
		inline crow::SimpleApp& GetCrow() { return *mRouteApp; }
		std::shared_ptr<crow::SimpleApp> mRouteApp;
		std::shared_ptr<pof::base::databasemysql> mDatabase;

		std::uint16_t mPort = 8080;
		pof::base::net_manager mNetManager;
		std::unordered_map<size_t, std::shared_ptr<Manager>> mManagerMap;
	};

	extern std::shared_ptr<Application> GetApp();
};
