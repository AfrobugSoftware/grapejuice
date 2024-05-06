#pragma once

#include "netmanager.h"
#include "databasemysql.h"
#include <boost/lexical_cast.hpp>

namespace grape
{
	class AccountManager {
	public:
		AccountManager();
		~AccountManager();

		void CreateAccountTable();
		void CreateSessionTable();
		void SetRoutes();


		pof::base::net_manager::res_t OnSignUp(pof::base::net_manager::req_t& req,
				boost::urls::matches& match);

		pof::base::net_manager::res_t OnSignIn(pof::base::net_manager::req_t& req,
			boost::urls::matches& match);

		pof::base::net_manager::res_t OnSignOut(pof::base::net_manager::req_t& req,
			boost::urls::matches& match);

	};
};