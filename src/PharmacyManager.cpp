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
		auto& req_body = req.body();
		auto&& [pharmacy, buf] = grape::serial::read<grape::pharmacy>(boost::asio::buffer(req_body));

		boost::trim(pharmacy.name);
		std::transform(pharmacy.name.begin(), pharmacy.name.end(), pharmacy.name.begin(),
			[](char& c) -> char {return std::tolower(c); });
		bool b = false;
		if (!(b = co_await CheckIfPharmacyExists(pharmacy.name))) {
			co_return app->mNetManager.bad_request("Pharmacy already exists");
		}

		auto&& [address, buf2] = grape::serial::read<grape::address>(buf);

		//write the address
		address.id = uuidGen();
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO address (country, state, lag, street, num, add_description) VALUES (?,?,?,?,?,?);)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
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
		pharmacy.id = uuidGen();
		query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO pharmacy VALUES (?,?,?,?);)");
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


	}
	catch (const std::logic_error& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

//creates a new branch
boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::PharmacyManager::OnOpenPharmacyBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
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

		//verify the request
		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) && 
				app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		auto&& [branch, buf2] = grape::serial::read<grape::branch>(boost::asio::buffer(buf));
		
		//extract data from the object
		branch.id = uuidGen();
		boost::trim(branch.name);
		std::transform(branch.name.begin(), branch.name.end(), branch.name.begin(),
			[](char& c) -> char {return std::tolower(c); });
		bool b = false;
		if ((b = co_await  CheckIfBranchExists(branch.name, branch.pharmacy_id))) {
			co_return app->mNetManager.not_found("Branch exists");
		}
		
		//open the branch
		branch.state = OPEN;
		
		//branch address
		auto&& [address, buf3] = grape::serial::read<grape::address>(buf2);
		address.id = uuidGen();
		
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
	

		//write the branches
		query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(
			INSERT INTO branches VALUES (?,?,?,?,?,?);
		)");
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(branch.pharmacy_id.begin(), branch.pharmacy_id.end())),
					boost::mysql::field(boost::mysql::blob(branch.id.begin(), branch.id.end())),
					boost::mysql::field(boost::mysql::blob(address.id.begin(), address.id.end())),
					boost::mysql::field(branch.name),
					boost::mysql::field(branch.state),
					boost::mysql::field(branch.info)
		} };
		fut = std::move(query->get_future());
		app->mDatabase->push(query);
		auto&& [ec2] = co_await query->m_waittime->async_wait();
		if (ec2 != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}

		d = fut.get();
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

		auto& req_body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(req_body));

		if (!app->mAccountManager.VerifySession(cred.account_id, cred.session_id)) {
			co_return app->mNetManager.auth_error("ACCOUNT NOT AUTHORISED");
		}
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE branches set branch_state = ? WHERE branch_id = ?;)");
		query->m_arguments = {
			{boost::mysql::field(state), 
			 boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()))} };
		auto fut = query->get_future();
		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			co_return app->mNetManager.timeout_error();
		}
		app->mDatabase->push(query);
		auto d = fut.get();
		
		if ( state == CLOSED ){
			mActivePharamcyBranches.erase(cred.branch_id);
		}
		else if( state == OPEN ) {
			//put the brach into our list of open branches
			query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
				R"(SELECT * FROM branches WHERE branch_id = ?;)");
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

boost::asio::awaitable<pof::base::net_manager::res_t>
	grape::PharmacyManager::OnGetPharmacyBranches(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (!app->mAccountManager.AuthuriseRequest(req)) {
			co_return app->mNetManager.auth_error("Account not authorized");
		}

		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		//create the query
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT * FROM branches WHERE pharmacy_id = ?;)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(std::chrono::seconds(60));
		query->m_arguments = { {
					boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()))
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
		grape::collection::branches branches;
		std::vector<grape::branch>& group = branches.group;
		group.reserve(d->size());
		for (auto& row : *d) {
			auto&& b = grape::serial::build<grape::branch>(row.first);
			group.emplace_back(std::move(b));
		}

		co_return app->OkResult(branches);
	}
	catch (const js::json::exception& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (const std::exception& err) {
		co_return app->mNetManager.server_error(err.what());
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
