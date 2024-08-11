#pragma once

#include "netmanager.h"
#include "databasemysql.h"
#include <../base/bcrypt/include/bcrypt.h>

#include <boost/lexical_cast.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/chrono.hpp>
#include <boost/noncopyable.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include "serialiser.h"

namespace grape {
	using opt_hash = grape::optional_field<std::string, 0>;
	using opt_secque = grape::optional_field<std::string, 1>;
	using opt_secans = grape::optional_field<std::string, 2>;
	using opt_sessionid = grape::optional_field<boost::uuids::uuid, 3>;
	using opt_session_start_time = grape::optional_field<std::chrono::system_clock::time_point, 4>;


	enum class account_type : std::uint32_t {
		pharmacist,
		loccum_pharmacist,
		intern_pharmacist,
		pharmacy_tech,
		student_pharmacist,
		dispenser,
		sale_assistant
	};
};

//accounts
BOOST_FUSION_DEFINE_STRUCT(
	(grape), account,
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, account_id)
	(grape::account_type, type)
	(std::bitset<5>, privilage)
	(std::string, first_name)
	(std::string, last_name)
	(std::chrono::year_month_day , dob)
	(std::string, phonenumber)
	(std::string, email)
	(std::string, username)
	(grape::opt_fields, fields)
	(grape::opt_hash, passhash)
	(grape::opt_secque, sec_que)
	(grape::opt_secans, sec_ans)
	(grape::opt_sessionid, session_id)
	(grape::opt_session_start_time, session_start_time)
)

//accounts collection
BOOST_FUSION_DEFINE_STRUCT(
	(grape)(collection), accounts,
	(std::vector<grape::account>, group)
)

//account cred
BOOST_FUSION_DEFINE_STRUCT(
	(grape), account_cred,
	(boost::uuids::uuid, pharmacy_id)
	(std::string, username)
	(std::string, password)
	(std::chrono::system_clock::time_point, last_session_time)
)

//account sign in reponse, a session cred
BOOST_FUSION_DEFINE_STRUCT(
	(grape), session_cred,
	(std::chrono::system_clock::time_point, session_start_time)
	(boost::uuids::uuid, session_id)
)

namespace grape
{
	class AccountManager : public boost::noncopyable {
	public:
		//account type
		enum : std::uint8_t {
			ACCOUNT_PHARMACY = 0x01,
			ACCOUNT_INSTITUTION = 0x02,
		};

		AccountManager();
		~AccountManager();

		void CreateAccountTable();
		void SetRoutes();


		boost::asio::awaitable<pof::base::net_manager::res_t> OnSignUp(pof::base::net_manager::req_t&& req,boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnSignIn(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

		boost::asio::awaitable<pof::base::net_manager::res_t> OnSignOut(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

		boost::asio::awaitable<pof::base::net_manager::res_t> OnSignInFromSession(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

		boost::asio::awaitable<pof::base::net_manager::res_t> GetActiveSession(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

		boost::asio::awaitable<pof::base::net_manager::res_t> UpdateUserAccount(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> GetUsersForPharmacy(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

		bool VerifySession(const boost::uuids::uuid& aid, const boost::uuids::uuid& sid);
		boost::asio::awaitable<bool> CheckUsername(const std::string& username);
		boost::asio::awaitable<void> UpdateSessions();

		bool CheckUserPrivilage(const boost::uuids::uuid& user, grape::account_type atype) const;
		bool AuthuriseRequest(pof::base::net_manager::req_t& req);
		bool IsUser(const boost::uuids::uuid& accountID, const boost::uuids::uuid& id);
		bool RemoveAllAccountsInPharmacy(const boost::uuids::uuid& pharmacyId);


		std::chrono::system_clock::duration mSessionDuration = std::chrono::days(5);
		//thread safe
		boost::concurrent_flat_map<boost::uuids::uuid,
		 grape::account> mActiveSessions;
	};
};