#include "PharmacyManager.h"
#include "Application.h"

grape::PharmacyManager::PharmacyManager()
{
}

grape::PharmacyManager::~PharmacyManager()
{
}

void grape::PharmacyManager::CreateTables()
{
	CreatePharmacyTable();
	CreateBranchTable();
	CreateAddressTable();
	CreateInstitution();
}

void grape::PharmacyManager::CreatePharmacyTable()
{
	auto app = grape::GetApp();
	//pharmacy info is a json sting
	auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
		R"(
			CREATE TABLE IF NOT EXISTS pharmacy (
			pharmacy_id binary(16) not null,
			pharmacy_name text,
			pharamcy_address binary(16),
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

void grape::PharmacyManager::CreateInstitution()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS institutions (
				id binary(16) not null,
				name text,
				type tinyint,
				address_id binary(16),
				info text
			);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);

		(void)fut.get();
	}
	catch (const std::exception& err) {
		spdlog::error(err.what());
	}
}

void grape::PharmacyManager::CreateBranchTable()
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(
			CREATE TABLE IF NOT EXISTS branches (
			branch_id binary(16),
			pharmacy_id binary(16),
			address_id binary(16),
			branch_name text,
			branch_type integer,
			branch_state integer,
			branch_info text
		);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto d = fut.get();
	}
	catch (boost::mysql::error_with_diagnostics& err) {
		//need to log this 
		spdlog::error(err.what());
	}
}

void grape::PharmacyManager::CreateAddressTable()
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS address (
			address_id binary(16) not null,
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
		spdlog::error(err.what());
	}
}

void grape::PharmacyManager::SetRoutes()
{
	auto app = grape::GetApp();
	app->route("/pharmacy/create", std::bind_front(&grape::PharmacyManager::OnCreatePharmacy, this));
	app->route("/pharmacy/createbranch", std::bind_front(&grape::PharmacyManager::OnCreateBranch, this));
	app->route("/pharmacy/openbranch", std::bind_front(&grape::PharmacyManager::OnOpenPharmacyBranch, this));
	app->route("/pharmacy/getbranches", std::bind_front(&grape::PharmacyManager::OnGetBranches, this));
	app->route("/pharmacy/getbranchesid", std::bind_front(&grape::PharmacyManager::OnGetBranchesById, this));
	app->route("/pharmacy/setbranchstate/{state}", std::bind_front(&grape::PharmacyManager::OnSetBranchState, this));

	app->route("/pharmacy/getpharmacies", std::bind_front(&grape::PharmacyManager::OnGetPharmacies, this));
	app->route("/pharmacy/search", std::bind_front(&grape::PharmacyManager::OnSearchPharmacies, this));

}

boost::asio::awaitable<pof::base::net_manager::res_t> 
	grape::PharmacyManager::OnCreatePharmacy(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	std::unique_ptr<boost::uuids::random_generator_mt19937> uuidGen = 
		std::make_unique<boost::uuids::random_generator_mt19937>();
	try {
		if (req.method() != http::verb::post) {
			co_return app->mNetManager.bad_request("Method should be post method"s);
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}
		auto& req_body = req.body();
		auto&& [pharmacy, buf] = grape::serial::read<grape::pharmacy>(boost::asio::buffer(req_body));

		boost::trim(pharmacy.name);
		std::transform(pharmacy.name.begin(), pharmacy.name.end(), pharmacy.name.begin(),
			[](char& c) -> char {return std::tolower(c); });
		bool b = false;
		if ((b = co_await CheckIfPharmacyExists(pharmacy.name))) {
			co_return app->mNetManager.bad_request("Pharmacy already exists");
		}

		auto&& [address, buf2] = grape::serial::read<grape::address>(buf);

		//write the address
		address.id = (*uuidGen)();
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO address (address_id, country, state, lga, street, num, add_description) VALUES (?,?,?,?,?,?,?);)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
				boost::mysql::field(boost::mysql::blob(address.id.begin(), address.id.end())),
				boost::mysql::field(address.country),
				boost::mysql::field(address.state),
				boost::mysql::field(address.lga),
				boost::mysql::field(address.street),
				boost::mysql::field(address.num),
				boost::mysql::field(address.add_info),
		} };
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}

		auto d = fut.get();

		//write pharmacy
		pharmacy.id = (*uuidGen)();
		query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO pharmacy VALUES (?,?,?,?);)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(pharmacy.id.begin(), pharmacy.id.end())),
			boost::mysql::field(pharmacy.name),
			boost::mysql::field(boost::mysql::blob(address.id.begin(), address.id.end())),
			boost::mysql::field(pharmacy.info)
		}};
		fut = std::move(query->get_future());
		app->mDatabase->push(query);
		auto&& [ec2] = co_await query->m_waittime->async_wait();
		if (ec2 != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		d = fut.get();

		pharmacy.address_id = address.id;

		co_return app->OkResult(pharmacy, req.keep_alive(), http::status::created);

	}
	catch (const std::logic_error& jerr) {
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
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Arguments required");
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		using string_t = boost::fusion::vector<std::string>;
		auto&& [info, buf2] = grape::serial::read<string_t>(buf);

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE pharmacy SET info = ? WHERE id = ?;)");
		query->m_arguments = { {
				boost::mysql::field(boost::fusion::at_c<0>(info)),
				boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()))
		} };
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
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
			
		(void)fut.get();

		co_return app->OkResult("Info updated");
	}
	catch (const std::logic_error& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::PharmacyManager::OnCreateBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	std::unique_ptr<boost::uuids::random_generator_mt19937> uuidGen =
		std::make_unique<boost::uuids::random_generator_mt19937>();
	try {

		if (req.method() != http::verb::post) {
			co_return app->mNetManager.bad_request("Method should be post method"s);
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}

		//verify the request
		auto& body = req.body();
		auto&& [branch, buf] = grape::serial::read<grape::branch>(boost::asio::buffer(body));

		//extract data from the object
		branch.id = (*uuidGen)();
		boost::trim(branch.name);
		std::transform(branch.name.begin(), branch.name.end(), branch.name.begin(),
			[](char& c) -> char {return std::tolower(c); });
		bool b = false;
		if ((b = co_await  CheckIfBranchExists(branch.name, branch.pharmacy_id))) {
			co_return app->mNetManager.not_found("Branch exists");
		}

		//open the branch
		branch.state = grape::branch_state::open;

		//branch address
		auto&& [address, buf3] = grape::serial::read<grape::address>(buf);
		if (address.id == boost::uuids::nil_uuid()) {
			address.id = (*uuidGen)();
			//write the address
			auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
				R"(INSERT INTO address (address_id, country, state, lag, street, num, add_description) VALUES (?,?,?,?,?,?,?);)");
			query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
			query->m_waittime->expires_after(std::chrono::seconds(60));
			query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(address.id.begin(), address.id.end())),
					boost::mysql::field(address.country),
					boost::mysql::field(address.state),
					boost::mysql::field(address.lga),
					boost::mysql::field(address.street),
					boost::mysql::field(address.num),
					boost::mysql::field(address.add_info),
			} };
			auto fut = query->get_future();
			app->mDatabase->push(query);
			auto&& [ec] = co_await query->m_waittime->async_wait();
			if (ec != boost::asio::error::operation_aborted) {
				co_return app->mNetManager.timeout_error();
			}
			auto d = fut.get();
		}

		//write the branches
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			INSERT INTO branches VALUES (?,?,?,?,?,?);
		)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(branch.pharmacy_id.begin(), branch.pharmacy_id.end())),
					boost::mysql::field(boost::mysql::blob(branch.id.begin(), branch.id.end())),
					boost::mysql::field(boost::mysql::blob(address.id.begin(), address.id.end())),
					boost::mysql::field(branch.name),
					boost::mysql::field(static_cast<std::underlying_type_t<grape::branch_type>>(branch.type)),
					boost::mysql::field(static_cast<std::underlying_type_t<grape::branch_state>>(branch.state)),
					boost::mysql::field(branch.info)
		} };
		auto fut = std::move(query->get_future());
		app->mDatabase->push(query);
		auto&& [ec2] = co_await query->m_waittime->async_wait();
		if (ec2 != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}

		auto d = fut.get();
		co_return app->OkResult(branch, req.keep_alive(), http::status::created);
	}
	catch (const std::logic_error& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}


//creates a new branch
boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::PharmacyManager::OnOpenPharmacyBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	std::unique_ptr<boost::uuids::random_generator_mt19937> uuidGen =
		std::make_unique<boost::uuids::random_generator_mt19937>();
	try {
		
		if (req.method() != http::verb::post) {
			co_return app->mNetManager.bad_request("Method should be post method"s);
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}

		//verify the request
		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) && 
				app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		auto&& [branch, buf2] = grape::serial::read<grape::branch>(boost::asio::buffer(buf));
		
		//extract data from the object
		branch.id = (*uuidGen)();
		boost::trim(branch.name);
		std::transform(branch.name.begin(), branch.name.end(), branch.name.begin(),
			[](char& c) -> char {return std::tolower(c); });
		bool b = false;
		if ((b = co_await  CheckIfBranchExists(branch.name, branch.pharmacy_id))) {
			co_return app->mNetManager.not_found("Branch exists");
		}
		
		//open the branch
		branch.state = grape::branch_state::open;
		
		//branch address
		auto&& [address, buf3] = grape::serial::read<grape::address>(buf2);
		if (address.id == boost::uuids::nil_uuid()) {
			address.id = (*uuidGen)();

			//write the address
			auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
				R"(INSERT INTO address (address_id, country, state, lag, street, num, add_description) VALUES (?,?,?,?,?,?,?);)");
			query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
			query->m_waittime->expires_after(std::chrono::seconds(60));
			query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(address.id.begin(), address.id.end())),
					boost::mysql::field(address.country),
					boost::mysql::field(address.state),
					boost::mysql::field(address.lga),
					boost::mysql::field(address.street),
					boost::mysql::field(address.num),
					boost::mysql::field(address.add_info),
			} };
			auto fut = query->get_future();
			app->mDatabase->push(query);
			auto&& [ec] = co_await query->m_waittime->async_wait();
			if (ec != boost::asio::error::operation_aborted) {
				co_return app->mNetManager.timeout_error();
			}
			auto d = fut.get();
		}
	

		//write the branches
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			INSERT INTO branches VALUES (?,?,?,?,?,?);
		)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(branch.pharmacy_id.begin(), branch.pharmacy_id.end())),
					boost::mysql::field(boost::mysql::blob(branch.id.begin(), branch.id.end())),
					boost::mysql::field(boost::mysql::blob(address.id.begin(), address.id.end())),
					boost::mysql::field(branch.name),
					boost::mysql::field(static_cast<std::underlying_type_t<grape::branch_type>>(branch.state)),
					boost::mysql::field(branch.info)
		} };
		auto fut = std::move(query->get_future());
		app->mDatabase->push(query);
		auto&& [ec2] = co_await query->m_waittime->async_wait();
		if (ec2 != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}

		auto d = fut.get();
		co_return app->OkResult(branch);
	}
	catch (const std::logic_error &jerr) {
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
		grape::branch_state state;
		if (boost::iequals(*mt, "closed")) {
			state = grape::branch_state::open;
		}
		else if (boost::iequals(*mt, "open")) {
			state = grape::branch_state::closed;
		}
		else {
			throw std::logic_error("Invalid state specified");
		}

		auto& req_body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(req_body));

		if (!app->mAccountManager.VerifySession(cred.account_id, cred.session_id)) {
			co_return app->mNetManager.auth_error("ACCOUNT NOT AUTHORISED");
		}
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE branches set branch_state = ? WHERE branch_id = ?;)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_arguments = {
			{boost::mysql::field(static_cast<std::underlying_type_t<grape::branch_state>>(state)), 
			 boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()))} };
		auto fut = query->get_future();
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		app->mDatabase->push(query);
		auto d = fut.get();
		
		if ( state == grape::branch_state::closed ){
			mActivePharamcyBranches.erase(cred.branch_id);
		}
		else if( state == grape::branch_state::open ) {
			//put the brach into our list of open branches
			query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
				R"(SELECT * FROM branches WHERE branch_id = ?;)");
			query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
			query->m_waittime->expires_after(60s);
			query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()))
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

			auto& row = *(d->begin());
			grape::branch&& branch = grape::serial::build<grape::branch>(row.first);
			mActivePharamcyBranches.emplace(cred.branch_id, branch);
		}
		co_return app->OkResult("Successful", "Successfully changed branch state");
	}
	catch (const std::logic_error& jerr) {
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
		co_return app->OkResult("Successfully destoried pharmacy");
	}
	catch (const js::json::exception& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}

}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::PharmacyManager::OnCreateInstitutions(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
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
		auto&& [institution, buf] = grape::serial::read<grape::institution>(boost::asio::buffer(body));
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
			R"(SELECT 1 FROM pharmacy WHERE pharmacy_name = ?;)");
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
	catch (const std::exception& exp) {
		
		co_return false;
	}
}

boost::asio::awaitable<bool> grape::PharmacyManager::CheckIfInstitutionExists(const std::string& name)
{
	if (name.empty()) co_return false;
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT 1 FROM institution WHERE name = ?;)");
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
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
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

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::PharmacyManager::OnGetPharmacies(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get) {
			co_return app->mNetManager.bad_request("Get method expected");
		}
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(SELECT * FROM pharmacy;)");


		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
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
		if (!data) {
			co_return app->mNetManager.unprocessiable("Cannot get pharmacies");
		}

		grape::collection_type<grape::pharmacy> cpharmacies;
		auto& pharmacies = boost::fusion::at_c<0>(cpharmacies);
		pharmacies.reserve(pharmacies.size()); 
		for(auto& d : *data){
			grape::pharmacy p;
			p.id = boost::variant2::get<boost::uuids::uuid>(d.first[0]);
			p.name = boost::variant2::get<std::string>(d.first[1]);

			pharmacies.emplace_back(std::move(p));
		}

		co_return app->OkResult(cpharmacies, req.keep_alive());
	}
	catch (std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::PharmacyManager::OnSearchPharmacies(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::search){
			co_return app->mNetManager.bad_request("Expected a search");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected an argument");
		auto&& [searh_for, buf] = grape::serial::read<string_t>(boost::asio::buffer(body));
		auto& s = boost::fusion::at_c<0>(searh_for);
		boost::trim(s);
		boost::to_lower(s);

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT * FROM pharmacy WHERE pharmacy_name LIKE CONCAT( '%',?,'%');)");
		query->m_arguments = { {
			boost::mysql::field(s)
		}};
		
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
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
		if (!data) {
			co_return app->mNetManager.unprocessiable("Cannot get pharmacies");
		}

		grape::collection_type<grape::pharmacy> cpharmacies;
		auto& pharmacies = boost::fusion::at_c<0>(cpharmacies);
		pharmacies.reserve(pharmacies.size());
		for (auto& d : *data) {
			grape::pharmacy p;
			p.id = boost::variant2::get<boost::uuids::uuid>(d.first[0]);
			p.name = boost::variant2::get<std::string>(d.first[1]);

			pharmacies.emplace_back(std::move(p));
		}

		co_return app->OkResult(cpharmacies, req.keep_alive());
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t>
grape::PharmacyManager::OnGetBranches(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get) {
			co_return app->mNetManager.bad_request("Expected a search"s);
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected an argument"s);

		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		//get the count of branches you want to read
		auto&& [pg, buf2] = grape::serial::read<grape::page>(buf);

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT 
			branch_id,
			pharmacy_id,
			address_id,
			branch_name,
			branch_type,
			branch_state,
			branch_info
			ROW_NUMBER() OVER (PARTITION BY pharmacy_id ORDER BY name) AS row_id
			FROM branches WHERE pharmacy_id = ? AND row_id BETWEEN ? AND ?;)"s);
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
			boost::mysql::field(pg.begin),
			boost::mysql::field(pg.begin + pg.limit)
		}};


		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
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
		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("No branches for pharmacy");
		}

		grape::collection_type<grape::branch> branches;
		auto& bs = boost::fusion::at_c<0>(branches);
		bs.reserve(data->size());
		for (auto& d : *data) {
			bs.emplace_back(grape::serial::build<grape::branch>(d.first));
		}

		co_return app->OkResult(branches, req.keep_alive());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::PharmacyManager::OnClosePharmacyBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::delete_) 
		{
			co_return app->mNetManager.bad_request("Expected a delete request");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected an argument");

		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id)))
		{
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		if (!app->mAccountManager.CheckUserPrivilage(cred.account_id, grape::account_type::pharmacist)) {
			co_return app->mNetManager.auth_error("Account does not have the priviage");
		}

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE branches SET branch_state = ? WHERE pharmacy_id = ? AND branch_id = ?;)");
		query->m_arguments = { {
				boost::mysql::field(static_cast<std::underlying_type_t<grape::branch_state>>(grape::branch_state::shutdown)),
				boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
				boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()))
		} };
		
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
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

		(void)fut.get();

		co_return app->OkResult("shutdown branch");

	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::PharmacyManager::OnGetBranchesById(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get) {
			co_return app->mNetManager.bad_request("Expected a GET method");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected arguments");

		auto&& [uid, buf] = grape::serial::read<grape::uid_t>(boost::asio::buffer(body));
		auto& id = boost::fusion::at_c<0>(uid);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT * FROM branches WHERE branch_id = ?;)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(id.begin(), id.end()))
		} };

		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
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
		if (!data || data->empty()) co_return app->mNetManager.not_found("Branch not found");

		co_return app->OkResult(grape::serial::build<grape::branch>((*data->begin()).first), req.keep_alive());

	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

