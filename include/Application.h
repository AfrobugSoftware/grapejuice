#pragma once
#include <boost/noncopyable.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <boost/signals2/signal.hpp>
#include <regex>


#include "netmanager.h"
#include "databasemysql.h"
#include "packages.h"

#include "AccountManager.h"
#include "PharmacyManager.h"

namespace grape {
	namespace js = nlohmann;
	namespace fs = std::filesystem;

	//utilities
	extern bool VerifyEmail(const std::string& email);
	extern bool VerifyPhonenumber(const std::string& phone);

	class Application : public boost::noncopyable, 
		public std::enable_shared_from_this<Application>
	{
	public:
		boost::signals2::signal<void(void)> mUpdateSignal;
		Application(const std::string& servername);
		virtual ~Application();

		virtual bool Init();
		virtual bool Run();
		virtual bool Exit();

		void CreateRoutes();
		void CreateTable();
		void route(const std::string& target,
			pof::base::net_manager::callback&& endpoint);

		void OnTimeout(boost::system::error_code ec);
		
		std::string ExtractString(pof::base::net_manager::req_t& req);

		std::string mServerName; 
		std::shared_ptr<pof::base::databasemysql> mDatabase;
		std::uint16_t mPort = 8080;
		pof::base::net_manager mNetManager;
		grape::AccountManager mAccountManager;
		grape::PharmacyManager mPharmacyManager;

		boost::optional<boost::asio::steady_timer> mUpdateTimer = boost::none;
	};

	extern std::shared_ptr<Application> GetApp();
};
