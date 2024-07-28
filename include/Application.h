#pragma once
#include <boost/noncopyable.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <boost/signals2/signal.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include <boost/program_options.hpp>
#include <filesystem>

#include <regex>
#include <fstream>

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

//app details
BOOST_FUSION_DEFINE_STRUCT(
	(grape), app_details,
	(boost::uuids::uuid, app_id)
	(boost::uuids::uuid, app_install_location_id)
	(std::string, app_name)
	(std::string, app_version)
	(std::string, os)
	(std::string, locale)
	(std::chrono::system_clock::time_point, app_installed_date)
	(std::chrono::system_clock::time_point, app_last_update)
	(std::chrono::system_clock::time_point, app_last_ping)
)

//file buffer
BOOST_FUSION_DEFINE_STRUCT(
	(grape), file,
	(std::string, name)
	(std::vector<std::uint8_t>, content)
)

//address
BOOST_FUSION_DEFINE_STRUCT(
	(grape), address,
	(boost::uuids::uuid, id)
	(std::string, country)
	(std::string, state)
	(std::string, lga)
	(std::string, street)
	(std::string, num)
	(std::string, add_info)
)


BOOST_FUSION_DEFINE_STRUCT(
	(grape), page,
	(std::uint32_t, begin)
	(std::uint32_t, limit)
)


namespace po = boost::program_options;
namespace fs = std::filesystem;
namespace grape {
	template<typename T>
		requires grape::FusionStruct<T>
	using collection_type = boost::fusion::vector<std::vector<T>>;
	using date_query_t = boost::fusion::vector<opt_fields, optional_field<std::chrono::year_month_day, 0>>;
	using pid = grape::collection_type<boost::fusion::vector<boost::uuids::uuid>>;
	using optional_list_t = boost::fusion::vector<opt_fields, optional_field<std::vector<boost::uuids::uuid>, 0>>;
	using string_t = boost::fusion::vector<std::string>;

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

		virtual bool Init(int argc, char** argv);
		virtual bool Run();
		virtual bool Exit();

		void CreateRoutes();
		void CreateTable();
		void CreateAppDetailsTable();
		void route(const std::string& target,
			pof::base::net_manager::callback&& endpoint);

		
		std::string ExtractString(pof::base::net_manager::req_t& req);

		grape::response OkResult(const std::string& message, const std::string& status = "Successful"s,
			 bool keep_alive = true, http::status stat = http::status::ok);

		template<typename T>
			requires grape::FusionStruct<T>
		grape::response OkResult(const T& data, bool keep_alive = true, http::status stat = http::status::ok) {
			grape::response res{ stat, 11 };
			res.set(http::field::server, USER_AGENT_STRING);
			res.set(http::field::content_type, "application/octlet-stream");
			res.keep_alive(keep_alive);

			grape::response::body_type::value_type value(grape::serial::get_size(data), 0x00);
			grape::serial::write(boost::asio::buffer(value), data);

			res.body() = std::move(value);
			res.prepare_payload();
			return res;
		}

		//application specific routes
		void SetRoutes();
		boost::asio::awaitable<pof::base::net_manager::res_t> onAppPing(pof::base::net_manager::req_t&& req,
			boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> onAppCheckUpdate(pof::base::net_manager::req_t&& req,
			boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> onAppUpdate(pof::base::net_manager::req_t&& req,
			boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> onGetOffice(pof::base::net_manager::req_t&& req,
			boost::urls::matches&& match);


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
