#include "AccountManager.h"
#include "Application.h"

grape::AccountManager::AccountManager()
{
}

grape::AccountManager::~AccountManager()
{
}

void grape::AccountManager::CreateAccountTable()
{
	auto app = grape::GetApp();
	auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase);
	query->m_sql = R"(CREATE TABLE IF NOT EXISTS account (
		pharmacy_id blob,
		account_id blob,
		privilage integer,
		account_first_name text,
		account_last_name text,
		account_date_of_birth date,
		phonenumber text,
		email text,
		account_username text,
		account_passhash blob,
		sec_question text,
		sec_ans_hash text, 
		signin_time datetime, 
		signout_time datetime
	);)"s;
	auto fut = query->get_future();
	app->mDatabase->push(query);
	auto status = fut.wait_for(10ms);
	if (status == std::future_status::ready) {
		try {
			auto data = fut.get();
			if (!data) {
				spdlog::error("Canont create account table");
			}
		}
		catch (boost::mysql::error_with_diagnostics& exp) {
			spdlog::error("{}: {}", exp.what(),
					exp.get_diagnostics().client_message().data());
		}
	}
}

void grape::AccountManager::CreateSessionTable()
{
	auto app = grape::GetApp();
	auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase);
	query->m_sql = R"(CREATE TABLE IF NOT EXISTS session (
		pharmacy_id blob,
		account_id blob,
		session_id blob,
		session_start timestamp
	);)"s;
	auto fut = query->get_future();
	app->mDatabase->push(query);
	auto status = fut.wait_for(10ms);
	if (status == std::future_status::ready) {
		try {
			auto data = fut.get();
			if (!data) {
				spdlog::error("Canont create account table");
			}
		}
		catch (boost::mysql::error_with_diagnostics& exp) {
			spdlog::error("{}: {}", exp.what(),
				exp.get_diagnostics().client_message().data());
		}
	}
}
