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
	query->m_sql = R"(CREATE TABLE IF NOT EXISTS accounts (
		pharmacy_id binary(16),
		account_id binary(16) NOT NULL,
		account_type integer,
		privilage integer,
		account_first_name text,
		account_last_name text,
		account_date_of_birth date,
		phonenumber text,
		email text,
		regnumber text,
		account_username text,
		account_passhash text,
		sec_question text,
		sec_ans_hash text, 
		session_id binary(16),
		session_start datetime,
		PRIMARY KEY (account_id)
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
	app->route("/account/signinfromsession", std::bind_front(&grape::AccountManager::OnSignInFromSession, this));
	app->route("/account/updateaccount", std::bind_front(&grape::AccountManager::UpdateUserAccount, this));
	app->route("/account/getpharmacyusers", std::bind_front(&grape::AccountManager::GetUsersForPharmacy, this));
	app->route("/account/checkname/{name}", std::bind_front(&grape::AccountManager::OnCheckUsernameExists, this));
	app->route("/account/verifyuser", std::bind_front(&grape::AccountManager::OnVerifyUser, this));

}


boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::AccountManager::OnSignUp(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	
	try {
		if (req.method() != http::verb::post) {
			co_return app->mNetManager.bad_request("expected a post request");
		}

		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("expected a body");
		auto&& [account, buf] = grape::serial::read<grape::account>(boost::asio::buffer(body));

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			 R"(INSERT INTO accounts 
				(pharmacy_id,
				account_id,
				account_type,
				privilage,
				account_first_name,
				account_last_name,
				account_date_of_birth,
				phonenumber,
				email,
				regnumber,
				account_username,
				account_passhash,
				sec_question,
				sec_ans_hash) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?);)"s);
		std::vector<boost::mysql::field> args;

		std::string_view sq, sa;
		if(account.sec_que.has_value()){
			sq = account.sec_que.value();
		}
		if (account.sec_ans.has_value()) {
			sa = account.sec_ans.value();
		}
		account.account_id = boost::uuids::random_generator_mt19937{}();
		const auto niluuid = boost::uuids::nil_uuid();
		args = {
				boost::mysql::field(boost::mysql::blob(account.pharmacy_id.begin(), account.pharmacy_id.end())),
				boost::mysql::field(boost::mysql::blob(account.account_id.begin(), account.account_id.end())),
				boost::mysql::field(static_cast<std::underlying_type_t<grape::account_type>>(account.type)),
				boost::mysql::field(account.privilage.to_ullong()),
				boost::mysql::field(account.first_name),
				boost::mysql::field(account.last_name),
				boost::mysql::field(boost::mysql::date((std::int32_t)account.dob.year(),
					(std::uint32_t)account.dob.month(), (std::uint32_t)account.dob.day())),
				boost::mysql::field(account.phonenumber),
				boost::mysql::field(account.email),
				boost::mysql::field(account.regnumber),
				boost::mysql::field(account.username),
				boost::mysql::field(account.passhash.value()),
				boost::mysql::field(sq),
				boost::mysql::field(bcrypt::generateHash(sa.data()))
		};

		try {
			query->m_arguments.push_back(std::move(args));
			query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
			query->m_waittime->expires_after(std::chrono::seconds(60));

			auto fut = query->get_future();
			bool tried = app->mDatabase->push(query);
			if (!tried) {
				tried = co_await app->mDatabase->retry(query); //try to push into the queue multiple times
				if (!tried) {
					co_return app->mNetManager.server_error("Error in query");
				}
			}

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
		grape::uid_t id;
		boost::fusion::at_c<0>(id) = account.account_id;
		co_return app->OkResult(id, req.keep_alive());
	}
	catch (const std::logic_error& err) {
		spdlog::error(err.what());
		co_return app->mNetManager.bad_request(err.what());
	}
	catch (std::exception& exp) {
		spdlog::error(exp.what());
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
		auto& body = req.body();
		auto&& [account_cred, buf] = grape::serial::read<grape::account_cred>(boost::asio::buffer(body));
		


		std::string sql;
		if (grape::VerifyEmail(account_cred.username)) {
			sql = R"(SELECT 
				pharmacy_id,
				account_id,
				account_type,
				privilage,
				account_first_name,
				account_last_name,
				account_date_of_birth,
				phonenumber,
				email,
				regnumber,
				account_username,
				account_passhash,
				sec_question,
				sec_ans_hash
			 FROM accounts WHERE email = ? AND pharmacy_id = ?;)"s;
		}
		else {
			sql = R"(SELECT 
				pharmacy_id,
				account_id,
				account_type,
				privilage,
				account_first_name,
				account_last_name,
				account_date_of_birth,
				phonenumber,
				email,
				regnumber,
				account_username,
				account_passhash,
				sec_question,
				sec_ans_hash
			FROM accounts WHERE account_username = ? AND pharmacy_id = ?;)"s;
		}

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, std::move(sql));
		query->m_arguments = { {boost::mysql::field(account_cred.username), 
			  boost::mysql::field(boost::mysql::blob(account_cred.pharmacy_id.begin(),
				  account_cred.pharmacy_id.end()))}};
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		auto fut = query->get_future();
		bool tried = app->mDatabase->push(query);

		if (!tried) {
			tried = co_await app->mDatabase->retry(query); //try to push into the queue multiple times
			if (!tried) {
				co_return app->mNetManager.server_error("Error in query");
			}
		}

		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			//was cancelled as a result of completion of timer, or any other error occurs
			co_return app->mNetManager.timeout_error();
		}
		
		auto accountDetails = fut.get();
		if (!accountDetails || accountDetails->empty()) {
			co_return app->mNetManager.bad_request(fmt::format("No account with username or email, \'{}\'",
				account_cred.username));
		}

		auto& acc = *accountDetails->begin();
		auto&& account = grape::serial::build<grape::account>(acc.first);
		//verify password
		if (!bcrypt::validatePassword(account_cred.password, account.passhash.value())) {
			co_return app->mNetManager.auth_error("Invalid password");
		}

		//start a session for the user
		query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			UPDATE accounts SET session_id = ?, session_start = ? WHERE account_username = ? AND account_id = ?;
		)"s);

		account.session_id = boost::uuids::random_generator_mt19937{}();
		account.session_start_time = pof::base::data::clock_t::now();
		auto tt = std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(account.session_start_time.value());
		query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(account.session_id.value().begin(), account.session_id.value().end())),
					boost::mysql::field(boost::mysql::datetime(tt)),
					boost::mysql::field(account.username),
					boost::mysql::field(boost::mysql::blob(account.account_id.begin(), account.account_id.end()))
		} };

		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		tried = app->mDatabase->push(query);
		if (!tried) {
			tried = co_await app->mDatabase->retry(query); //try to push into the queue multiple times
			if (!tried) {
				co_return app->mNetManager.server_error("Error in query");
			}
		}
		std::tie(ec) = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		fut = query->get_future();
		auto d = fut.get();

		//set session into active sessions
		mActiveSessions.emplace(std::make_pair(account.session_id.value(), account));

		co_return app->OkResult(account, req.keep_alive());
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
		if (req.method() != http::verb::post) {
			co_return app->mNetManager.bad_request("Method should be post method"s);
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}
		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));

		if (!(VerifySession(cred.account_id, cred.session_id) && IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		auto signOutTime = std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(std::chrono::system_clock::now());

		//do the sign out
		const auto sid = boost::uuids::nil_uuid();
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE accounts SET signout = ?, SET session_id = ? WHERE account_id = ?;)");
		query->m_arguments = { {
				boost::mysql::field(boost::mysql::datetime(signOutTime)),
				boost::mysql::field(boost::mysql::blob(sid.begin(), sid.end())),
				boost::mysql::field(boost::mysql::blob(cred.account_id.begin(), cred.account_id.end()))
		}};
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		auto fut = query->get_future();
		bool tried = app->mDatabase->push(query);
		if (!tried) {
			tried = co_await app->mDatabase->retry(query); //try to push into the queue multiple times
			if (!tried) {
				co_return app->mNetManager.server_error("Error in query");
			}
		}
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		auto d = fut.get();

		//remove from active accounts
		mActiveSessions.erase(cred.session_id);
		co_return app->OkResult("Account signed out");
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
		if (req.method() != http::verb::post) {
			co_return app->mNetManager.bad_request("get method expected");
		}
		if(!req.has_content_length()){
			co_return app->mNetManager.bad_request("body is required");
		}

		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::session_cred>(boost::asio::buffer(body));
		if (cred.session_start_time + mSessionDuration <= std::chrono::system_clock::now()) {
			co_return app->mNetManager.auth_error("Session time out");
		}


		//increase timeout ?
		auto session_time = pof::base::data::clock_t::now();		
		//get account for the session
		grape::account account;
		auto found = mActiveSessions.visit(cred.session_id, [&](auto& v) {
			account = v.second;
		});
		if (!found) {
			co_return app->mNetManager.auth_error("Session time out");
		}

		//change the session_id
		account.session_id.value() = boost::uuids::random_generator_mt19937{}();
		account.session_start_time.value() = session_time;
		
		//update database
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE accounts SET session_start_time = ?, session_id = ? WHERE account_id = ?;)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(session_time))),
			boost::mysql::field(boost::mysql::blob(account.session_id.value().begin(), account.session_id.value().end())),
			boost::mysql::field(boost::mysql::blob(account.account_id.begin(), account.account_id.end()))
		}};
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		auto fut = query->get_future();
		bool tried = app->mDatabase->push(query);
		if (!tried) {
			tried = co_await app->mDatabase->retry(query); //try to push into the queue multiple times
			if (!tried) {
				co_return app->mNetManager.server_error("Error in query");
			}
		}
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		//check if we have an error
		(void)fut.get();

		//update cache
		mActiveSessions.erase(cred.session_id);
		mActiveSessions.emplace(std::make_pair(account.session_id.value(), account));

		co_return app->OkResult(account, req.keep_alive());
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
		

		co_return app->OkResult("Updated user account");
	}
	catch (std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}

}

boost::asio::awaitable<pof::base::net_manager::res_t> 
	grape::AccountManager::GetUsersForPharmacy(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));

		if (!(VerifySession(cred.account_id, cred.session_id) && IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT * FROM accounts WHERE pharmacy_id = ?;)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_arguments = { {
		boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()))
		} };
		query->m_waittime->expires_after(60s);
		auto fut = query->get_future();
		bool tried = app->mDatabase->push(query);
		if (!tried) {
			tried = co_await app->mDatabase->retry(query); //try to push into the queue multiple times
			if (!tried) {
				co_return app->mNetManager.server_error("Error in query");
			}
		}
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}

		auto data = fut.get();
		grape::collection::accounts accounts;
		accounts.group.reserve(data->size());
		for (const auto& d : *data) {
			accounts.group.emplace_back(grape::serial::build<grape::account>(d.first));
		}
		co_return app->OkResult(accounts);
	}
	catch (const std::logic_error& err) {
		co_return app->mNetManager.bad_request(err.what());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

bool grape::AccountManager::VerifySession(const boost::uuids::uuid& aid, const boost::uuids::uuid& sid)
{
	try {
		boost::optional<grape::account> user = boost::none;
		//this might block
		bool found = mActiveSessions.visit(aid,
			[&](const auto& v) {
				user = v.second;
			});
		if (!found && !user.has_value()) return false;
		if (user.value().session_id.value() != sid) return false; //uid and sid mismatch
		else if (user.value().session_start_time.value() + mSessionDuration < pof::base::data::clock_t::now()) return false;
		else return true;
	}
	catch (std::exception& exp) {
		spdlog::error(exp.what());
	}
	return false;
}

boost::asio::awaitable<bool>
	grape::AccountManager::CheckUsername(const std::string& username,const boost::uuids::uuid& pharmid)
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			SELECT 1 FROM accounts WHERE account_username = ? AND pharmacy_id = ?;
		)");
		query->m_arguments = { {boost::mysql::field(username),
			boost::mysql::field(boost::mysql::blob(pharmid.begin(), pharmid.end())) } };
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(1));

		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);

		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted || pushed == false) {
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
	try {
		if (mActiveSessions.empty()) {
			//the server is starting up, check dataabase for active sessions
			auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			SELECT * FROM accounts WHERE session_id != ?;
		)");
			auto nullid = boost::uuids::nil_uuid();
			query->m_arguments = { {boost::mysql::field(boost::mysql::blob(nullid.begin(), nullid.end()))} };
			query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
			query->m_waittime->expires_after(std::chrono::seconds(1));

			auto fut = query->get_future();
			bool pushed = app->mDatabase->push(query);
			auto&& [ec] = co_await query->m_waittime->async_wait();
			if (ec != boost::asio::error::operation_aborted || pushed == false) {
				throw std::system_error(std::make_error_code(std::errc::timed_out));
			}

			auto accountDetails = fut.get();
			if (!accountDetails || accountDetails->empty()) co_return;

			for (auto& ac : *accountDetails) {
				auto v = grape::serial::build<grape::account>(ac.first);
				if (v.session_start_time.value() + mSessionDuration < pof::base::data::clock_t::now()) continue;

				mActiveSessions.emplace(std::make_pair(v.session_id.value(), v));
			}
		}
		else {
			//remove expired sessions
			mActiveSessions.erase_if([&](const auto& ac) -> bool {
				return (ac.second.session_start_time.value() + mSessionDuration < pof::base::data::clock_t::now());
				});
			//how to use mysql functions ? so that it can do the resetting on its own
		}
	}
	catch (const std::exception& exp) {
		//spdlog::error(exp.what());
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

bool grape::AccountManager::IsUser(const boost::uuids::uuid& accountID, const boost::uuids::uuid& id)
{
	boost::optional<grape::account> user = boost::none;
	//this might block
	bool found = mActiveSessions.visit(accountID,
		[&](const auto& v) {
			user = v.second;
		});
	if (!found || !user.has_value()) return false;
	return (user->account_id == id);
}

bool grape::AccountManager::CheckUserPrivilage(const boost::uuids::uuid& userID, grape::account_type atype) const
{
	boost::optional<grape::account> user = boost::none;
	bool found = mActiveSessions.visit(userID,
		[&](const auto& v) {
			user = v.second;
		});
	if (!found || !user.has_value()) return false;
	return (user.value().type == atype);
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::AccountManager::OnCheckUsernameExists(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get)
			co_return app->mNetManager.bad_request("expected a get reqest");
		
		std::string name = match["name"];

		boost::trim(name);
		boost::to_lower(name);
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected an argument"s);

		auto&& [pid, buf] = grape::serial::read<grape::uid_t>(boost::asio::buffer(body));
		if (!(co_await CheckUsername(name, boost::fusion::at_c<0>(pid)))) {
			co_return app->mNetManager.not_found("username does not exisits");
		}

		co_return app->OkResult("username exists");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::AccountManager::OnVerifyUser(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		auto& body = req.body();
		if (body.empty()) throw std::logic_error("expected a body");
		grape::account_cred cred;
		auto&& [v, buf] = grape::serial::read<grape::account_cred>(boost::asio::buffer(body));

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT a.type, a.username, a.passhash FROM accounts a WHERE a.pharmacy_id = ? AND a.username = ?;)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(boost::fusion::at_c<0>(v).begin(),  boost::fusion::at_c<0>(v).end())),
			boost::mysql::field(boost::fusion::at_c<2>(v))
		} };
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(1));

		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted || pushed == false) {
			throw std::system_error(std::make_error_code(std::errc::timed_out));
		}

		auto data = fut.get();
		if (!data || data->empty())
			co_return app->mNetManager.not_found("Account not found");

		using r_type = boost::fusion::vector<
			account_type,
			std::string,
			std::string
		>;

		r_type s = serial::build<r_type>(data->begin()->first);
		if (boost::fusion::at_c<0>(s) > boost::fusion::at_c<1>(v) || 
			!bcrypt::validatePassword(boost::fusion::at_c<3>(v), boost::fusion::at_c<2>(s))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		co_return app->OkResult("verified", req.keep_alive());
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
		co_return app->mNetManager.server_error(exp.what());
	}
}

