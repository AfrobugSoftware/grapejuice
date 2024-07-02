#include "PharmacyManager.h"
#include "Application.h"

grape::PharmacyManager::PharmacyManager()
{
}

grape::PharmacyManager::~PharmacyManager()
{
}

void grape::PharmacyManager::CreatePharmacyTable()
{
	auto app = grape::GetApp();
	//pharmacy info is a json sting
	auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
		R"(
			CREATE TABLE IF NOT EXISTS pharmacy (
			pharmacy_id blob,
			pharmacy_name text,
			pharamcy_address blob,
			pharmacy_info text
		);)");
	auto fut = query->get_future();
	app->mDatabase->push(query);
	try {
		auto data = fut.get(); //block
	}
	catch (boost::mysql::error_with_diagnostics& err) {
		spdlog::error(err.what());
	}
}

void grape::PharmacyManager::CreateBranchTable()
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(
			CREATE TABLE IF NOT EXSITS branches (
			branch_id blob,
			pharmacy_id blob,
			address_id blob,
			branch_name text,
			branch_state integer,
			branch_info text
		);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto d = fut.get();
	}
	catch (boost::mysql::error_with_diagnostics& err) {
		//need to log this 
	}
}

void grape::PharmacyManager::CreateAddressTable()
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS address (
			address_id blob,
			country text,
			state text,
			lga text,
			street text,
			num text,
			add_description text);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto d = fut.get();
	}
	catch (boost::mysql::error_with_diagnostics& err) {
		//need to log this
	}
}

void grape::PharmacyManager::SetRoutes()
{
	auto app = grape::GetApp();
	app->route("/pharmacy/create", std::bind_front(&grape::PharmacyManager::OnCreatePharmacy, this));
	app->route("/pharmacy/openbranch", std::bind_front(&grape::PharmacyManager::OnOpenPharmacyBranch, this));
	app->route("/pharmacy/getbranches", std::bind_front(&grape::PharmacyManager::OnGetPharmacyBranches, this));
	app->route("/pharmacy/setbranchstate/{state}", std::bind_front(&grape::PharmacyManager::OnSetBranchState, this));

}

boost::asio::awaitable<pof::base::net_manager::res_t> 
	grape::PharmacyManager::OnCreatePharmacy(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	thread_local static boost::uuids::random_generator_mt19937 uuidGen;
	try {
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
		boost::uuids::uuid pharmacy_id = uuidGen();
		std::string pharmacy_name = jsonData["pharmacy_name"];
		std::string pharmacy_info = jsonData["pharmacy_info"];
		boost::trim(pharmacy_name);
		std::transform(pharmacy_name.begin(), pharmacy_name.end(), pharmacy_name.begin(),
			[](char& c) -> char {return std::tolower(c); });
		bool b = false;
		if (!(b = co_await CheckIfPharmacyExists(pharmacy_name))) {
			co_return app->mNetManager.bad_request("Pharmacy already exists");
		}

		js::json addressObj = jsonData["address"];
		boost::uuids::uuid add_id = uuidGen();
		const std::string country = addressObj["country"];
		const std::string state = addressObj["state"];
		const std::string lga = addressObj["lga"];
		const std::string street = addressObj["street"];
		const std::string num = addressObj["num"];
		const std::string addInfo = addressObj["add_description"];

		//write the address
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO address (country, state, lag, street, num, add_description) VALUES (?,?,?,?,?,?);)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
				boost::mysql::field(country),
				boost::mysql::field(state),
				boost::mysql::field(lga),
				boost::mysql::field(street),
				boost::mysql::field(num),
				boost::mysql::field(addInfo),
		} };
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}

		auto d = fut.get();

		//write pharmacy
		query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO pharmacy VALUES (?,?,?,?);)");
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(pharmacy_id.begin(), pharmacy_id.end())),
			boost::mysql::field(pharmacy_name),
			boost::mysql::field(boost::mysql::blob(add_id.begin(), add_id.end())),
			boost::mysql::field(pharmacy_info)
		}};
		fut = std::move(query->get_future());
		app->mDatabase->push(query);
		auto&& [ec2] = co_await query->m_waittime->async_wait();
		if (ec2 != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		d = fut.get();

		http::response<http::dynamic_body> res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/json");
		res.keep_alive(req.keep_alive());

		js::json jobj = js::json::object();
		jobj["result_status"] = "Successful"s;
		jobj["result_message"] = "Branch created sucessfully"s;
		jobj["pharmacy_id"] = boost::lexical_cast<std::string>(pharmacy_id);

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
	catch (std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
	grape::PharmacyManager::OnPharmacyInfoUpdate(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get) {
			co_return app->mNetManager.bad_request("GET method expected");
		}


	}
	catch (const js::json::exception& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::PharmacyManager::OnOpenPharmacyBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	thread_local static boost::uuids::random_generator_mt19937 uuidGen;
	try {
		if (!app->mAccountManager.AuthuriseRequest(req)) {
			co_return app->mNetManager.auth_error("Account not authorizesd");
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
		
		//extract data from the object
		js::json obj = js::json::parse(data);
		const boost::uuids::uuid pharmacyId = boost::lexical_cast<boost::uuids::uuid>(static_cast<std::string>(obj["pharamcy_id"]));
		std::string branch_name = obj["branch_name"];
		const std::string branch_info = obj["branch_info"];
		boost::uuids::uuid branch_id = uuidGen();

		boost::trim(branch_name);
		std::transform(branch_name.begin(), branch_name.end(), branch_name.begin(),
			[](char& c) -> char {return std::tolower(c); });
		bool b = false;
		if ((b = co_await  CheckIfBranchExists(branch_name, pharmacyId))) {
			co_return app->mNetManager.not_found("Branch exists");
		}

		const std::uint32_t branch_state = OPEN;
		//branch address
		js::json addressObj = obj["address"];
		boost::uuids::uuid add_id = uuidGen();
		const std::string country = addressObj["country"];
		const std::string state = addressObj["state"];
		const std::string lga = addressObj["lga"];
		const std::string street = addressObj["street"];
		const std::string num = addressObj["num"];
		const std::string addInfo = addressObj["add_description"];

		//write the address
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO address (address_id, country, state, lag, street, num, add_description) VALUES (?,?,?,?,?,?,?);)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
				boost::mysql::field(boost::mysql::blob(add_id.begin(), add_id.end())),
				boost::mysql::field(country),
				boost::mysql::field(state),
				boost::mysql::field(lga),
				boost::mysql::field(street),
				boost::mysql::field(num),
				boost::mysql::field(addInfo),
		} };
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		auto d = fut.get();
	

		//write the branches
		query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			INSERT INTO branches VALUES (?,?,?,?,?,?);
		)");
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(pharmacyId.begin(), pharmacyId.end())),
					boost::mysql::field(boost::mysql::blob(branch_id.begin(), branch_id.end())),
					boost::mysql::field(boost::mysql::blob(add_id.begin(), add_id.end())),
					boost::mysql::field(branch_name),
					boost::mysql::field(branch_state),
					boost::mysql::field(branch_info)
		} };
		fut = std::move(query->get_future());
		app->mDatabase->push(query);
		auto&& [ec2] = co_await query->m_waittime->async_wait();
		if (ec2 != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}

		d = fut.get();



		http::response<http::dynamic_body> res{ http::status::created, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/json");
		res.keep_alive(req.keep_alive());

		js::json jobj = js::json::object();
		jobj["result_status"] = "Successful"s;
		jobj["result_message"] = "Branch created sucessfully"s;
		jobj["branch_name"] = branch_name;
		jobj["branch_id"] = boost::lexical_cast<std::string>(branch_id);

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
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::PharmacyManager::OnSetBranchState(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::post) {
			co_return app->mNetManager.bad_request("Method should be post method"s);
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}

		auto mt = match.find("state");
		if (mt == match.end()) throw std::logic_error("improper varaible");
		std::uint32_t state = NONE_STATE;
		if (boost::iequals(*mt, "closed")) {
			state = CLOSED;
		}
		else if (boost::iequals(*mt, "open")) {
			state = OPEN;
		}
		else {
			throw std::logic_error("Invalid state specified");
		}

		js::json jsonData = js::json::parse(app->ExtractString(req));
		boost::uuids::uuid accountId = boost::lexical_cast<boost::uuids::uuid>(static_cast<std::string>(jsonData["accountID"]));
		boost::uuids::uuid sessionId = boost::lexical_cast<boost::uuids::uuid>(static_cast<std::string>(jsonData["sessionID"]));
		boost::uuids::uuid branchId =  boost::lexical_cast<boost::uuids::uuid>(static_cast<std::string>(jsonData["branchID"]));
		std::string pharmacyId = jsonData["pharmacyID"];
		if (!app->mAccountManager.VerifySession(accountId, sessionId)) {
			co_return app->mNetManager.auth_error("ACCOUNT NOT AUTHORISED");
		}
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE branches set branch_state = ? WHERE branch_id = ?;)");
		query->m_arguments = {
			{boost::mysql::field(state), 
			 boost::mysql::field(boost::mysql::blob(branchId.begin(), branchId.end()))} };
		auto fut = query->get_future();
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		app->mDatabase->push(query);
		auto d = fut.get();
		
		if ( state == CLOSED ){
			mActivePharamcyBranches.erase(branchId);
		}
		else if( state == OPEN ) {
			//put the brach into our list of open branches
			query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
				R"(SELECT * FROM branches WHERE branch_id = ?;)");
			query->m_waittime->expires_after(60s);
			query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(branchId.begin(), branchId.end()))
			}};
			fut = std::move(query->get_future());
			app->mDatabase->push(query);
			auto&& [ec2] = co_await query->m_waittime->async_wait();
			if (ec2 != boost::asio::error::operation_aborted) {
				co_return app->mNetManager.timeout_error();
			}
			d = fut.get();
			if (d == nullptr || d->empty()) {
				co_return app->mNetManager.server_error("Cannot set branch to open");
			}
			auto& branchId = boost::variant2::get<boost::uuids::uuid>((d->begin())->first[BRANCH_ID]);
			mActivePharamcyBranches.emplace(branchId, *d->begin());
		}

		http::response<http::dynamic_body> res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/json");
		res.keep_alive(req.keep_alive());

		js::json jobj = js::json::object();
		jobj["result_status"] = "Successful"s;
		jobj["result_message"] = "Branch state has been set sucessfully"s;
		 
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
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
	grape::PharmacyManager::OnDestroyPharmacy(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try{
		

		co_return pof::base::net_manager::res_t{};
	}
	catch (const js::json::exception& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}

}

boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::PharmacyManager::OnGetPharmacyBranches(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (!app->mAccountManager.AuthuriseRequest(req)) {
			co_return app->mNetManager.auth_error("Account not authorized");
		}
		js::json obj = js::json::parse(app->ExtractString(req));
		boost::uuids::uuid pharmacyId = boost::lexical_cast<boost::uuids::uuid>(obj["pharmacy_id"]);

		//create the query
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT * FROM branches WHERE pharmacy_id = ?;)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(pharmacyId.begin(), pharmacyId.end()))
		}};
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		auto d = fut.get();

		if (d == nullptr && d->empty()) {
			co_return app->mNetManager.not_found("No branches for pharmacy");
		}


	}
	catch (const js::json::exception& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (const std::exception& err) {
		co_return app->mNetManager.server_error(err.what());
	}
}

boost::asio::awaitable<bool>
	grape::PharmacyManager::CheckIfPharmacyExists(const std::string& name)
{
	if (name.empty()) co_return false;
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT 1 FROM pharamcy WHERE pharmacy_name = ?;)");
		query->m_arguments = { {
				boost::mysql::field(name)
			} };
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(60s);
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			throw std::system_error(std::make_error_code(std::errc::timed_out));
		}
		
		auto d = fut.get(); //I need to suspend and not block

		co_return (d != nullptr && !d->empty());
	}
	catch (boost::mysql::error_with_diagnostics& err){
		co_return false;
	}
}

boost::asio::awaitable<bool>
	grape::PharmacyManager::CheckIfBranchExists(const std::string& bn, const boost::uuids::uuid& pid)
{
	try {
		auto app = GetApp();
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT 1 FROM branches WHERE branch_name = ? AND pharmacy_id = ?;)");
		query->m_arguments = { {
			boost::mysql::field(bn),
			boost::mysql::field(boost::mysql::blob(pid.begin(), pid.end()))
		} };
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(60s);

		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			throw std::system_error(std::make_error_code(std::errc::timed_out));

		}
		
		auto d = fut.get(); //I need to suspend and not block

		co_return (d != nullptr && !d->empty());
	}
	catch (const boost::mysql::error_with_diagnostics& err) {
		co_return false;
	}
}
