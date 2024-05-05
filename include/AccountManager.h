#pragma once

#include "netmanager.h"
#include "databasemysql.h"

namespace grape
{
	class AccountManager {
	public:
		AccountManager();
		~AccountManager();

		void CreateAccountTable();
		void CreateSessionTable();
	};
};