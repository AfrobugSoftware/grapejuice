#pragma once

#include "netmanager.h"
#include "databasemysql.h"
#include <boost/lexical_cast.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/chrono.hpp>

namespace grape
{
	class AccountManager {
	public:
		enum : std::uint8_t {
			PHARMACY_ID = 0,
			ACCOUNT_ID,
			PRIVILAGE,
			FIRST_NAME,
			LAST_NAME,
			DOB,
			PHONENUMBER,
			EMAIL,
			USERNAME,
			PASSHASH,
			SEC_QUE,
			SEC_ANSHASH,
			SIGNIN_TIME,
			SIGNOUT_TIME,
			SESSION_ID,
			SESSION_START_TIME,

			ACCOUNT_COL_COUNT,
		};
		AccountManager();
		~AccountManager();

		void CreateAccountTable();
		void SetRoutes();


		pof::base::net_manager::res_t OnSignUp(pof::base::net_manager::req_t& req,
				boost::urls::matches& match);

		pof::base::net_manager::res_t OnSignIn(pof::base::net_manager::req_t& req,
			boost::urls::matches& match);

		pof::base::net_manager::res_t OnSignOut(pof::base::net_manager::req_t& req,
			boost::urls::matches& match);

		pof::base::net_manager::res_t OnSignInFromSession(pof::base::net_manager::req_t& req,
			boost::urls::matches& match);

		pof::base::net_manager::res_t GetActiveSession(pof::base::net_manager::req_t& req,
			boost::urls::matches& match);

		pof::base::net_manager::res_t UpdateUserAccount(pof::base::net_manager::req_t& req,
			boost::urls::matches& match);

		bool VerifySession(const boost::uuids::uuid& aid, 
			const boost::uuids::uuid& sid);
		bool CheckUsername(const std::string& username);

		void UpdateSessions();

		std::chrono::system_clock::duration mSessionDuration = std::chrono::days(5);
		//thread safe
		boost::concurrent_flat_map<boost::uuids::uuid,
			pof::base::data::row_t> mActiveSessions;
	};
};