#pragma once
#include <boost/noncopyable.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <boost/signals2/signal.hpp>
#include <boost/fusion/include/define_struct.hpp>

#include <regex>


#include "netmanager.h"
#include "databasemysql.h"
#include "packages.h"

#include "AccountManager.h"
#include "PharmacyManager.h"
#include "ProductManager.h"

//protocol serialiser
#include "serialiser.h"

//operation result message
BOOST_FUSION_DEFINE_STRUCT(
	(grape), result,
	(std::string, status)
	(std::string, message)
)

//pharmacy credentials
BOOST_FUSION_DEFINE_STRUCT(
	(grape), credentials,
	(boost::uuids::uuid, account_id)
	(boost::uuids::uuid, session_id)
	(boost::uuids::uuid, pharm_id)
	(boost::uuids::uuid, branch_id)
)


namespace grape {
	using request = pof::base::net_manager::req_t;
	using response = pof::base::net_manager::res_t;

	namespace js = nlohmann;
	namespace fs = std::filesystem;

	//utilities
	extern bool VerifyEmail(const std::string& email);
	extern bool VerifyPhonenumber(const std::string& phone);

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

		
		std::string ExtractString(pof::base::net_manager::req_t& req);

		grape::response OkResult(const std::string& message, const std::string& status = "Successful"s,
			 bool keep_alive = true);

		template<typename T>
			requires grape::FusionStruct<T>
		grape::response OkResult(const T& data, bool keep_alive = true) {
			grape::response res{ http::status::ok, 11 };
			res.set(http::field::server, USER_AGENT_STRING);
			res.set(http::field::content_type, "application/octlet-stream");
			res.keep_alive(keep_alive);

			grape::response::body_type::value_type value(grape::serial::get_size(data), 0x00);
			grape::serial::write(boost::asio::buffer(value), data);

			res.body() = std::move(value);
			res.prepare_payload();
			return res;
		}

		std::string mServerName; 
		std::shared_ptr<pof::base::databasemysql> mDatabase;
		std::uint16_t mPort = 8080;
		pof::base::net_manager mNetManager;
		grape::AccountManager mAccountManager;
		grape::PharmacyManager mPharmacyManager;
		grape::ProductManager mProductManager;

		boost::asio::awaitable<void> RunUpdateTimer();
		boost::optional<pof::base::dataquerybase::timer_t> mUpdateTimer = boost::none;
		std::vector<std::function<boost::asio::awaitable<void>(void)>> mUpdateAsyncFuncs;
	};

	extern std::shared_ptr<Application> GetApp();
};
