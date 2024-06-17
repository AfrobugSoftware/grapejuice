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
		signout_time datetime,
		session_id blob,
		session_start datetime
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
		
		//verify data
		if (!grape::VerifyEmail(static_cast<std::string>(jsonData["email"]))) {
			return app->mNetManager.bad_request("Invalid email");
		}

		if (!grape::VerifyPhonenumber(static_cast<std::string>(jsonData["phonenumber"]))) {
			return app->mNetManager.bad_request("Invalid email");
		}

		if (CheckUsername(static_cast<std::string>(jsonData["username"]))) {
			return app->mNetManager.bad_request("Username already exisits");
		}


		std::array<size_t, 3> dates = {};
		int i = 0;
		for (const auto s : std::views::split(static_cast<std::string>(jsonData["account_date_of_birth"]), '-')) {
			dates[i] = boost::lexical_cast<size_t>(std::string(s.begin(), s.end()));
			i++;
		}


		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			 R"(INSERT INTO account VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?, ?, ?);)"s);
		std::vector<boost::mysql::field> args;
		pof::base::data::duuid_t pid = boost::uuids::random_generator_mt19937{}();
		pof::base::data::duuid_t uid = boost::uuids::random_generator_mt19937{}();
		const auto niluuid = boost::uuids::nil_uuid();

		args.reserve(14);
		args = {
				boost::mysql::field(boost::mysql::blob(pid.begin(), pid.end())),
				boost::mysql::field(boost::mysql::blob(uid.begin(), uid.end())),
				boost::mysql::field(static_cast<int>(jsonData["privilage"])),
				boost::mysql::field(static_cast<std::string>(jsonData["account_first_name"])),
				boost::mysql::field(static_cast<std::string>(jsonData["account_last_name"])),
				boost::mysql::field(boost::mysql::date(dates[0], dates[1], dates[2])),
				boost::mysql::field(static_cast<std::string>(jsonData["phonenumber"])),
				boost::mysql::field(static_cast<std::string>(jsonData["email"])),
				boost::mysql::field(static_cast<std::string>(jsonData["account_username"])),
				boost::mysql::field(static_cast<std::string>(jsonData["account_passhash"])),
				boost::mysql::field(static_cast<std::string>(jsonData["sec_question"])),
				boost::mysql::field(static_cast<std::string>(jsonData["sec_ans_hash"])),
				boost::mysql::field(boost::mysql::datetime()),
				boost::mysql::field(boost::mysql::datetime()),
				boost::mysql::field(boost::mysql::blob(niluuid.begin(), niluuid.end())),
				boost::mysql::field(boost::mysql::datetime())
		};

		query->m_arguments.push_back(std::move(args));
		auto fut = query->get_future();
		app->mDatabase->push(query);

		try {
			auto dp = fut.get(); //block until we finish or throw an exception
		}
		catch (boost::mysql::error_with_diagnostics& err) {
			spdlog::error(err.what());
			return app->mNetManager.server_error(err.what());
		}


		//send response
		http::response<http::dynamic_body> res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/json");
		res.keep_alive(req.keep_alive());

		http::dynamic_body::value_type value;
		js::json jres = js::json::object();
		jres["userid"] = boost::lexical_cast<std::string>(uid);
		jres["status"]  = "successfull"s;

		const std::string sendd = jres.dump();
		buffer = value.prepare(sendd.size());
		boost::asio::buffer_copy(buffer, boost::asio::buffer(sendd));
		value.commit(sendd.size());


		res.body() = std::move(value);
		res.prepare_payload();
		return res;
	}
	catch (const js::json::exception& err) {
		spdlog::error(err.what());
		return app->mNetManager.bad_request("In properly formarted json");
	}
	catch (std::exception& exp) {
		return app->mNetManager.server_error(exp.what());
	}
}

pof::base::net_manager::res_t grape::AccountManager::OnSignIn(pof::base::net_manager::req_t& req, boost::urls::matches& match)
{
	auto app = grape::GetApp();
	try{
		if (req.method() != http::verb::post) {
			return app->mNetManager.bad_request("Method should be post method"s);
		}
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
		std::string username = jsonData["username"];
		std::string password = jsonData["password"];
		std::uint64_t lastSession = static_cast<std::uint64_t>(jsonData["last_session_time"]);

		std::string sql;
		if (grape::VerifyEmail(username)) {
			sql = R"(SELECT * FROM accounts WHERE email = ?;)"s;
		}
		else {
			sql = R"(SELECT * FROM accounts WHERE account_username = ?;)"s;
		}

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, std::move(sql));
		query->m_arguments = { {boost::mysql::field(username)} };
		auto fut = query->get_future();
		app->mDatabase->push(query);

		auto accountDetails = fut.get();
		if (!accountDetails || accountDetails->empty()) {
			return app->mNetManager.bad_request(fmt::format("No account with username or email, \'{}\'", username));
		}

		//verify password



		//start a session for it
		auto sessionquery = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			UPDATE accounts SET session_id = ?, session_start = ? WHERE account_username = ? AND account_id = ?;
		)"s);
		auto& pid = boost::variant2::get<boost::uuids::uuid>(accountDetails->at(0).first[PHARMACY_ID]);
		auto& aid = boost::variant2::get<boost::uuids::uuid>(accountDetails->at(0).first[ACCOUNT_ID]);
		auto& aun = boost::variant2::get<std::string>(accountDetails->at(0).first[USERNAME]);

		auto sid = boost::uuids::random_generator_mt19937{}();
		auto tt = std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration,
			pof::base::data::clock_t, std::chrono::system_clock::duration>(pof::base::data::clock_t::now());
		sessionquery->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(sid.begin(), sid.end())),
					boost::mysql::field(boost::mysql::datetime(tt)),
					boost::mysql::field(boost::mysql::blob(pid.begin(), pid.end())),
					boost::mysql::field(boost::mysql::blob(aid.begin(), aid.end()))
		} };
		fut = std::move(sessionquery->get_future());
		auto d = fut.get();

		//response //enahnce the packer to pack multiple tables
		auto package = pof::base::packer{ *accountDetails }();
		http::response<http::dynamic_body> res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/octet-stream");
		res.keep_alive(req.keep_alive());

		http::dynamic_body::value_type value;
		auto buf = value.prepare(package.size());
		boost::asio::buffer_copy(buf, boost::asio::buffer(package));
		value.commit(package.size());

		res.prepare_payload();
		return res;
	}
	catch (std::exception& err) {
		return app->mNetManager.server_error(err.what());
	}
}

pof::base::net_manager::res_t grape::AccountManager::OnSignOut(pof::base::net_manager::req_t& req, boost::urls::matches& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::post) {
			return app->mNetManager.bad_request("Method should be post method"s);
		}
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
		std::string username = jsonData["username"];
		std::string accountId = jsonData["accountID"];
		std::string sessionId = jsonData["sessionID"];
		std::string pharmacyId = jsonData["pharmacy"];

		auto signOutTime = pof::base::data::clock_t::now();

	}
	catch (std::exception& ptr) {
		
	}
}

pof::base::net_manager::res_t grape::AccountManager::OnSignInFromSession(pof::base::net_manager::req_t& req, boost::urls::matches& match)
{
	return pof::base::net_manager::res_t();
}

pof::base::net_manager::res_t grape::AccountManager::GetActiveSession(pof::base::net_manager::req_t& req, boost::urls::matches& match)
{
	return pof::base::net_manager::res_t();
}

bool grape::AccountManager::VerifySession(const boost::uuids::uuid& aid, const boost::uuids::uuid& sid)
{
	auto app = grape::GetApp();
	try {
		boost::optional<pof::base::data::row_t> user = boost::none;
		//this might block
		bool found = mActiveSessions.visit(aid,
			[&](const auto& v) {
				user = v;
			});
		if (!found) return false;
		auto& v = user->first;
		auto& usid = boost::variant2::get<boost::uuids::uuid>(v[SESSION_ID]);
		auto& ustime = boost::variant2::get<pof::base::data::datetime_t>(v[SESSION_START_TIME]);
		if (usid != sid) return false; //uid and sid mismatch
		else if (ustime + mSessionDuration < pof::base::data::clock_t::now()) return false;
		else return true;
	}
	catch (std::exception& exp) {
		spdlog::error(exp.what());
	}
	return false;
}

bool grape::AccountManager::CheckUsername(const std::string& username)
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			SELECT 1 FROM accounts WHERE account_username = ?;
		)");
		query->m_arguments = { {boost::mysql::field(username)} };
		auto fut = query->get_future();
		app->mDatabase->push(query);

		auto accountDetails = fut.get();
		return (!accountDetails || accountDetails->empty()) ? false : true;
	}
	catch (std::exception& exp) {
		spdlog::error(exp.what());
	}
	return false;
}

void grape::AccountManager::UpdateSessions()
{
	auto app = grape::GetApp();
	if (mActiveSessions.empty()){
		//the server is starting up, check dataabase for active sessions
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			SELECT * FROM accounts WHERE session_id IS NOT ?;
		)");
		auto nullid = boost::uuids::nil_uuid();
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(nullid.begin(), nullid.end()))}};
		auto fut = query->get_future();
		app->mDatabase->push(query);

		auto accountDetails = fut.get();
		if(!accountDetails || accountDetails->empty()) return;

		for (auto& ac : *accountDetails) {
			auto& v = ac.first;
			auto& uid = boost::variant2::get<pof::base::data::duuid_t>(v[ACCOUNT_ID]);
			auto& ustime = boost::variant2::get<pof::base::data::datetime_t>(v[SESSION_START_TIME]);
			if (ustime + mSessionDuration < pof::base::data::clock_t::now()) continue;

			mActiveSessions.emplace(uid, ac);
		}
	}
	else {
		//remove expired sessions
		mActiveSessions.erase_if([&](const auto& ac) -> bool {
			const auto& v = ac.first;
			const auto& ustime = boost::variant2::get<pof::base::data::datetime_t>(v[SESSION_START_TIME]);
			return (ustime + mSessionDuration < pof::base::data::clock_t::now());
		});
		//how to use mysql functions ? so that it can do the resetting on its own
	}
}
