#pragma once
#include <boost/noncopyable.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <filesystem>


#include "netmanager.h"
#include "databasemysql.h"

#include "AccountManager.h"
namespace grape {
	namespace js = nlohmann;
	namespace fs = std::filesystem;
	class Application : public boost::noncopyable, 
		public std::enable_shared_from_this<Application>
	{
	public:
		Application(const std::string& servername);
		virtual ~Application();

		virtual bool Init();
		virtual bool Run();
		virtual bool Exit();

		void CreateRoutes();
		void CreateTable();
		void route(const std::string& target,
			pof::base::net_manager::callback&& endpoint);

		std::string mServerName; 
		std::shared_ptr<pof::base::databasemysql> mDatabase;
		std::uint16_t mPort = 8080;
		pof::base::net_manager mNetManager;
		grape::AccountManager mAccountManager;
	};

	extern std::shared_ptr<Application> GetApp();
};
