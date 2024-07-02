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

void grape::AccountManager::SetRoutes()
{
	auto app = grape::GetApp();
	app->route("/account/signup", std::bind_front(&grape::AccountManager::OnSignUp, this));
	app->route("/account/signin", std::bind_front(&grape::AccountManager::OnSignIn, this));
	app->route("/account/signout", std::bind_front(&grape::AccountManager::OnSignOut, this));
	app->route("/account/updateaccount", std::bind_front(&grape::AccountManager::UpdateUserAccount, this));
}


boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::AccountManager::OnSignUp(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	
	try {
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
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
			co_return app->mNetManager.bad_request("Invalid email");
		}

		if (!grape::VerifyPhonenumber(static_cast<std::string>(jsonData["phonenumber"]))) {
			co_return app->mNetManager.bad_request("Invalid email");
		}

		if (bool b = co_await CheckUsername(static_cast<std::string>(jsonData["username"]))) {
			co_return app->mNetManager.bad_request("Username already exisits");
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

		try {
			query->m_arguments.push_back(std::move(args));
			query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
			query->m_waittime->expires_after(std::chrono::seconds(60));

			auto fut = query->get_future();
			app->mDatabase->push(query);

			auto&& [ec] = co_await query->m_waittime->async_wait();
			if (ec == boost::asio::error::operation_aborted){
				//was cancelled as a result of completion
				auto dp = fut.get(); //block until we finish or throw an exception
			}
			else {
				//any other error including a timeout
				co_return app->mNetManager.timeout_error();
			}
		}
		catch (boost::mysql::error_with_diagnostics& err) {
			spdlog::error(err.what());
			co_return app->mNetManager.server_error(err.what());
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
		co_return res;
	}
	catch (const js::json::exception& err) {
		co_return app->mNetManager.bad_request(err.what());
	}
	catch (std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::AccountManager::OnSignIn(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try{
		if (req.method() != http::verb::post) {
			co_return app->mNetManager.bad_request("Method should be post method"s);
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
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
		std::string password = jsonData["password"]; //in plain text sent over ssl 
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
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));


		auto fut = query->get_future();
		app->mDatabase->push(query);

		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			//was cancelled as a result of completion of timer, or any other error occurs
			co_return app->mNetManager.timeout_error();
		}

		auto accountDetails = fut.get();
		if (!accountDetails || accountDetails->empty()) {
			co_return app->mNetManager.bad_request(fmt::format("No account with username or email, \'{}\'", username));
		}
		auto& acc = *accountDetails->begin();
		//verify password
		const std::string passHash = boost::variant2::get<std::string>(acc.first[PASSHASH]);
		if (!bcrypt::validatePassword(password, passHash)) {
			co_return app->mNetManager.auth_error("Invalid password");
		}

		//start a session for the user
		auto sessionquery = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			UPDATE accounts SET session_id = ?, session_start = ? WHERE account_username = ? AND account_id = ?;
		)"s);
		auto& pid = boost::variant2::get<boost::uuids::uuid>(acc.first[PHARMACY_ID]);
		auto& aid = boost::variant2::get<boost::uuids::uuid>(acc.first[ACCOUNT_ID]);
		auto& aun = boost::variant2::get<std::string>(acc.first[USERNAME]);

		auto sid = boost::uuids::random_generator_mt19937{}();
		auto tt = std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(pof::base::data::clock_t::now());
		sessionquery->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(sid.begin(), sid.end())),
					boost::mysql::field(boost::mysql::datetime(tt)),
					boost::mysql::field(boost::mysql::blob(pid.begin(), pid.end())),
					boost::mysql::field(boost::mysql::blob(aid.begin(), aid.end()))
		} };
		sessionquery->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		sessionquery->m_waittime->expires_after(std::chrono::seconds(60));
		fut = std::move(sessionquery->get_future());
		app->mDatabase->push(sessionquery);

		auto&& [ec2] = co_await query->m_waittime->async_wait();
		if (ec2 != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		auto d = fut.get();

		//set session into active sessions
		mActiveSessions.emplace(sid, acc);

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
		co_return res;
	}
	catch (std::exception& err) {
		co_return app->mNetManager.server_error(err.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::AccountManager::OnSignOut(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (!AuthuriseRequest(req)) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		if (req.method() != http::verb::post) {
			co_return app->mNetManager.bad_request("Method should be post method"s);
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}
		size_t len = boost::lexical_cast<size_t>(req.at(boost::beast::http::field::content_length));
		auto& req_body = req.body();
		std::string data;
		data.resize(len);

		auto buffer = req_body.data();
		boost::asio::buffer_copy(boost::asio::buffer(data), buffer);
		req_body.consume(len);

		js::json jsonData = js::json::parse(data);
		boost::uuids::uuid accountId = boost::lexical_cast<boost::uuids::uuid>(req["accountID"]);
		boost::uuids::uuid sessionId = boost::uuids::nil_uuid();
		boost::uuids::uuid pharmacyId = boost::lexical_cast<boost::uuids::uuid>(static_cast<std::string>(jsonData["pharmacyID"]));

		auto signOutTime = std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(std::chrono::system_clock::now());

		//do the sign out
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE accounts SET signout = ?, SET session_id = ? WHERE account_id = ?;)");
		query->m_arguments = { {
				boost::mysql::field(boost::mysql::datetime(signOutTime)),
				boost::mysql::field(boost::mysql::blob(sessionId.begin(), sessionId.end())),
				boost::mysql::field(boost::mysql::blob(accountId.begin(), accountId.end()))
		}};
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		auto d = fut.get();


		//remove from active accounts
		mActiveSessions.erase(sessionId);

		http::response<http::dynamic_body> res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/json");
		res.keep_alive(req.keep_alive());

		js::json jobj = js::json::object();
		jobj["result_status"] = "Successful"s;
		jobj["result_message"] = "Branch created sucessfully"s;

		auto sendData = jobj.dump();
		boost::beast::http::dynamic_body::value_type value{};
		auto buf = value.prepare(sendData.size());
		boost::asio::buffer_copy(buf, boost::asio::buffer(sendData));
		value.commit(sendData.size());

		res.body() = std::move(value);
		res.prepare_payload();
		co_return res;
	}
	catch (const js::json::exception& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (std::exception& ptr) {
		co_return app->mNetManager.server_error(ptr.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::AccountManager::OnSignInFromSession(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (!AuthuriseRequest(req)) {
			co_return app->mNetManager.auth_error("Account not signed in");
		}
		if (req.method() != http::verb::put) {
			co_return app->mNetManager.bad_request("GET method expected");
		}
		if(!req.has_content_length()){
			co_return app->mNetManager.bad_request("Body is required");
		}
			
		//increase timeout ?
		auto session_time = pof::base::data::clock_t::now();
		boost::uuids::uuid sess_id = boost::lexical_cast<boost::uuids::uuid>(req["Session-ID"]);
		boost::uuids::uuid acc_id = boost::lexical_cast<boost::uuids::uuid>(req["Account-ID"]);
		
		//update cache
		mActiveSessions.visit(sess_id, [&](auto& v) {
			boost::variant2::get<pof::base::data::datetime_t>(v.second.first[SESSION_START_TIME])
				= session_time;
		});

		//update database
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE accounts SET session_start_time = ? WHERE account_id = ? AND session_id = ?;)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(session_time))),
			boost::mysql::field(boost::mysql::blob(acc_id.begin(), acc_id.end())),
			boost::mysql::field(boost::mysql::blob(sess_id.begin(), sess_id.end()))
		}};
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		//check if we have an error
		(void)fut.get();

		http::response<http::dynamic_body> res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/json");
		res.keep_alive(req.keep_alive());

		js::json jobj = js::json::object();
		jobj["result_status"] = "Successful"s;
		jobj["result_message"] = "Sucessfully signed in"s;

		auto sendData = jobj.dump();
		boost::beast::http::dynamic_body::value_type value{};
		auto buf = value.prepare(sendData.size());
		boost::asio::buffer_copy(buf, boost::asio::buffer(sendData));
		value.commit(sendData.size());

		res.body() = std::move(value);
		res.prepare_payload();
		co_return res;
	}
	catch (const std::exception& err) {
		co_return app->mNetManager.server_error(err.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::AccountManager::GetActiveSession(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	co_return pof::base::net_manager::res_t();
}

boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::AccountManager::UpdateUserAccount(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (!AuthuriseRequest(req)) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		auto jsonData = js::json::parse(app->ExtractString(req));


	}
	catch (std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}

}

bool grape::AccountManager::VerifySession(const boost::uuids::uuid& aid, const boost::uuids::uuid& sid)
{
	try {
		boost::optional<pof::base::data::row_t> user = boost::none;
		//this might block
		bool found = mActiveSessions.visit(aid,
			[&](const auto& v) {
				user = v.second;
			});
		if (!found && !user.has_value()) return false;
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

boost::asio::awaitable<bool>
	grape::AccountManager::CheckUsername(const std::string& username)
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			SELECT 1 FROM accounts WHERE account_username = ?;
		)");
		query->m_arguments = { {boost::mysql::field(username)} };
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));

		auto fut = query->get_future();
		app->mDatabase->push(query);

		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			throw std::system_error(std::make_error_code(std::errc::timed_out));
		}

		auto accountDetails = fut.get();
		co_return (!accountDetails || accountDetails->empty()) ? false : true;
	}
	catch (std::exception& exp) {
		spdlog::error(exp.what());
	}
	co_return false;
}

boost::asio::awaitable<void>
	grape::AccountManager::UpdateSessions()
{
	auto app = grape::GetApp();
	if (mActiveSessions.empty()){
		//the server is starting up, check dataabase for active sessions
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			SELECT * FROM accounts WHERE session_id IS NOT ?;
		)");
		auto nullid = boost::uuids::nil_uuid();
		query->m_arguments = { {boost::mysql::field(boost::mysql::blob(nullid.begin(), nullid.end()))}};
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));

		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			throw std::system_error(std::make_error_code(std::errc::timed_out));
		}

		auto accountDetails = fut.get();
		if(!accountDetails || accountDetails->empty()) co_return;

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
			const auto& ustime = boost::variant2::get<pof::base::data::datetime_t>(ac.second.first[SESSION_START_TIME]);
			return (ustime + mSessionDuration < pof::base::data::clock_t::now());
		});
		//how to use mysql functions ? so that it can do the resetting on its own
	}
}

bool grape::AccountManager::AuthuriseRequest(pof::base::net_manager::req_t& req)
{
	try {
		const auto sid = boost::lexical_cast<boost::uuids::uuid>(req["Session-ID"]);
		const auto aid = boost::lexical_cast<boost::uuids::uuid>(req["Account-ID"]);
		return (VerifySession(aid, sid));
	}
	catch (const std::exception& err) {
		return false;
	}
}
