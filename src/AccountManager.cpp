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
		account_passhash text,
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

void grape::AccountManager::SetRoutes()
{
	auto app = grape::GetApp();
	app->route("/account/signup", std::bind_front(&grape::AccountManager::OnSignUp, this));
	app->route("/account/signin", std::bind_front(&grape::AccountManager::OnSignIn, this));
	app->route("/account/signout", std::bind_front(&grape::AccountManager::OnSignIn, this));
}


pof::base::net_manager::res_t grape::AccountManager::OnSignUp(pof::base::net_manager::req_t& req, boost::urls::matches& match)
{
	auto app = grape::GetApp();
	
	try {
		if (!req.has_content_length()) {
			return app->mNetManager.bad_request("Expected a body");
		}
		size_t len = boost::lexical_cast<size_t>(req.at("content-length"));
		auto& req_body = req.body();
		std::string data;
		data.resize(len);

		auto buffer = req_body.data();
		boost::asio::buffer_copy(boost::asio::buffer(data), buffer);
		req_body.consume(len);

		js::json jsonData = js::json::parse(data);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			 R"(INSERT INTO account VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?);)"s);
		std::vector<boost::mysql::field> args;
		args.reserve(14);
		pof::base::data::duuid_t pid = boost::uuids::random_generator_mt19937{}();
		pof::base::data::duuid_t uid = boost::uuids::random_generator_mt19937{}();

		args.emplace_back(boost::mysql::field(boost::mysql::blob(pid.begin(), pid.end())));
		args.emplace_back(boost::mysql::field(boost::mysql::blob(uid.begin(), uid.end())));
		args.emplace_back(boost::mysql::field(static_cast<int>(jsonData["privilage"])));
		args.emplace_back(boost::mysql::field(static_cast<std::string>(jsonData["account_first_name"])));
		args.emplace_back(boost::mysql::field(static_cast<std::string>(jsonData["account_last_name"])));



		std::array<size_t, 3> dates = {};
		int i = 0;
		for (const auto s : std::views::split(static_cast<std::string>(jsonData["account_date_of_birth"]), '-')){
			dates[i] = boost::lexical_cast<size_t>(std::string(s.begin(), s.end()));
			i++;
		}
		args.emplace_back(boost::mysql::field(boost::mysql::date(dates[0], dates[1], dates[2])));
		args.emplace_back(boost::mysql::field(static_cast<std::string>(jsonData["phonenumber"])));
		args.emplace_back(boost::mysql::field(static_cast<std::string>(jsonData["email"])));
		args.emplace_back(boost::mysql::field(static_cast<std::string>(jsonData["account_username"])));
		args.emplace_back(boost::mysql::field(static_cast<std::string>(jsonData["account_passhash"])));
		args.emplace_back(boost::mysql::field(static_cast<std::string>(jsonData["sec_question"])));
		args.emplace_back(boost::mysql::field(static_cast<std::string>(jsonData["sec_ans_hash"])));
		auto tt = std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration,
			pof::base::data::clock_t, std::chrono::system_clock::duration>(pof::base::data::clock_t::now());
		args.emplace_back(boost::mysql::field(boost::mysql::datetime(tt)));
		args.emplace_back(boost::mysql::field(boost::mysql::datetime(tt)));

		query->m_arguments.push_back(args);
		auto fut = query->get_future();
		app->mDatabase->push(query);

		try {
			auto dp = fut.get(); //block until we finish or throw an exception
		}
		catch (boost::mysql::error_with_diagnostics& err) {
			spdlog::error(err.what());
			return app->mNetManager.server_error(err.what());
		}

		http::response<http::dynamic_body> res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/json");
		res.keep_alive(req.keep_alive());

		http::dynamic_body::value_type value;
		js::json jres = js::json::object();
		jres["userid"] = boost::lexical_cast<std::string>(uid);
		jres["status"]  = "successfull"s;

		const auto sendd = jres.dump();
		buffer = value.prepare(sendd.size());
		boost::asio::buffer_copy(buffer, boost::asio::buffer(sendd));
		value.commit(sendd.size());


		res.body() = std::move(value);
		res.prepare_payload();
		return res;
	}
	catch (std::exception& exp) {
		return app->mNetManager.server_error(exp.what());
	}
}

pof::base::net_manager::res_t grape::AccountManager::OnSignIn(pof::base::net_manager::req_t& req, boost::urls::matches& match)
{
	return pof::base::net_manager::res_t();
}

pof::base::net_manager::res_t grape::AccountManager::OnSignOut(pof::base::net_manager::req_t& req, boost::urls::matches& match)
{
	return pof::base::net_manager::res_t();
}
