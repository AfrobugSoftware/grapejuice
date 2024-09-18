#include "SaleManager.h"
#include "Application.h"


void grape::SaleManager::CreateTables()
{
	CreateSaleTable();
}

void grape::SaleManager::SetRoutes()
{
	auto app = grape::GetApp();
	app->route("/sale/checkout", std::bind_front(&grape::SaleManager::OnSale, this));
	app->route("/sale/get", std::bind_front(&grape::SaleManager::OnGetSale, this));
	app->route("/sale/gethistory", std::bind_front(&grape::SaleManager::OnGetSaleHistory, this));
	app->route("/sale/return", std::bind_front(&grape::SaleManager::OnReturn, this));
}

void grape::SaleManager::CreateSaleTable()
{
	auto app = grape::GetApp();
	try {
		//sale_payment_method is an index into a sale_method table
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS sales  (
				pharmacy_id     binary(16),
				branch_id       binary(16),
				user_id         binary(16),
				sale_id         binary(16) NOT NULL,
				product_id      binary(16),
				formulary_id    binary(16),
				sale_date       datetime,
				unit_cost_price binary(17),
				unit_sale_price binary(17),
				discount        binary(17),
				total_amount    binary(17),
				quantity        integer,
				payment_method  VARCHAR(64),
				payment_addinfo text,
				product_label   text,
				sale_state      integer,
				sale_add_info   text,
				PRIMARY KEY     (sale_id)
			);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto d = fut.get();
	}
	catch (const std::exception& exp) {
		
		spdlog::error(exp.what());
	}
}

void grape::SaleManager::CreateProcedure()
{
	CreateSaleProcedure();
	CreateReturnProcedure();
}

void grape::SaleManager::CreateSaleProcedure()
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE PROCEDURE IF NOT EXISTS do_sale (
				IN pharmacy_id binary(16),
				IN branch_id    binary(16),
				IN user_id      binary(16),
				IN sale_id      binary(16),
				IN product_id   binary(16),
				IN formulary_id binary(16),
				IN sale_date       datetime,
				IN unit_cost_price binary(17),
				IN unit_sale_price binary(17),
				IN discount        binary(17),
				IN total_amount    binary(17),
				IN quantity        integer,
				IN payment_method  VARCHAR(256),
				IN payment_addinfo text,
				IN product_label   text,
				IN sale_state      integer,
				IN sale_add_info   text)
			   BEGIN
				START TRANSACTION;
				INSERT IGNORE INTO sales VALUES (pharmacy_id,branch_id,user_id,sale_id,product_id,formulary_id,
				sale_date,unit_cost_price, unit_sale_price, discount,total_amount,quantity,payment_method,payment_addinfo,
				product_label,sale_state,sale_add_info);
				UPDATE pharma_products pp SET pp.stock_count = pp.stock_count - quantity WHERE pp.pharmacy_id = pharmacy_id AND pp.branch_id = branch_id AND pp.product_id = product_id;
				COMMIT;
			  END;
		)");

		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get(); //block until complete
	}
	catch (const std::exception& exp) {
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));
	}
}

void grape::SaleManager::CreateReturnProcedure()
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE PROCEDURE IF NOT EXISTS do_return(IN pharm_id binary(16), 
			   IN bid binary(16), 
			   IN sid binary(16),
			   IN pid binary(16),
			   IN ret integer,
			   IN quantity integer)
			   BEGIN
				START TRANSACTION;
				UPDATE sales s SET state = ret WHERE s.sale_id = sid  AND s.pharmacy_id = pharm_id AND s.branch_id = bid AND s.product_id = pid;
				UPDATE pharma_products pp SET pp.stock_count = pp.stock_count + quantity WHERE pp.pharmacy_id = pharm_id AND pp.branch_id = bid AND pp.product_id = pid;
				COMMIT;
			  END;
		)");

		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get(); //block until complete
	
	}
	catch (const std::exception& exp) {
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));

	}
}

boost::asio::awaitable<grape::response> grape::SaleManager::OnSale(grape::request&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::post)
			co_return app->mNetManager.bad_request("expected a post");
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected a body");
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) && app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		auto&& [sale, buf2] = grape::serial::read<grape::collection_type<grape::sale>>(buf);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase, R"(CALL do_sales(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);)"s);
		auto& s = boost::fusion::at_c<0>(sale);
		query->m_arguments.reserve(s.size());
		auto sale_date = std::chrono::system_clock::now();
		for (auto& i : s) {
			//for each item to sell
			auto& item = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
			item.resize(19);
			i.sale_date = sale_date;
			i.state = sale_state::complete;

			auto salebody = grape::serial::make_mysql_arg(i);
			std::ranges::move(salebody, item.begin());

			item[15] = boost::mysql::field(i.quantity);
			item[16] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
			item[17] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
			item[18] = boost::mysql::field(boost::mysql::blob(i.product_id.begin(), i.product_id.end()));
		}
		auto data = app->run_query(query);

		//no error, generate receipt
		co_return app->OkResult("Sale complete");

	}
	catch (const std::exception& exp) {
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<grape::response> grape::SaleManager::OnGetSale(grape::request&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get)
			co_return app->mNetManager.bad_request("expected a get");
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected a body");
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) && app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		using stt = boost::fusion::vector<std::uint32_t, std::chrono::year_month_day, grape::sale_state>;
		auto&& [st, buf2] = grape::serial::read<stt>(buf);
		auto&& [pg, buf3] = grape::serial::read<grape::page>(buf2);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase);
		constexpr const std::array<std::string_view, 3> ymds = {
			"YEAR(s.sale_date)",
			"MONTH(s.sale_date)",
			"DAYOFMONTH(s.sale_date)"
		};
		if (boost::fusion::at_c<0>(st) >= ymds.size())
			throw std::invalid_argument("invalid date value");

		query->m_sql = std::format(R"(
			SELECT * FROM ( SELECT s.*,
			ROW_NUMER() OVER (ORDER BY p.sale_date) AS row_id
			FROM sales s
			WHERE s.pharmacy_id = ? AND s.branch_id = ? AND s.sale_state = ? 
			AND {} = ?;
		) as sub
		HAVING row_id BETWEEN ? AND ?;)", ymds[boost::fusion::at_c<0>(st)].data());

		auto& item = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item.resize(6);

		item[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		item[2] = boost::mysql::field(static_cast<std::underlying_type_t<grape::sale_state>>(boost::fusion::at_c<2>(st)));
		switch (boost::fusion::at_c<0>(st))
		{
		case 0:
			item[3] = boost::mysql::field((std::int32_t)boost::fusion::at_c<1>(st).year());
			break;
		case 1:
			item[3] = boost::mysql::field((std::uint32_t)boost::fusion::at_c<1>(st).month());
			break;
		case 2:
			item[3] = boost::mysql::field((std::uint32_t)boost::fusion::at_c<1>(st).day());
			break;
		default:
			break;
		}
		item[4] = boost::mysql::field(pg.begin);
		item[5] = boost::mysql::field(pg.begin + pg.limit);

		auto data = co_await app->run_query(query);
		if (!data || data->empty())
			co_return app->mNetManager.not_found("No sale");

		collection_type<grape::sale> sales;
		auto& ss = boost::fusion::at_c<0>(sales);
		ss.reserve(data->size());
		for (auto& d : *data) {
			ss.emplace_back(grape::serial::build<grape::sale>(d.first));
		}
		
		co_return app->OkResult(sales);
	}
	catch (const std::exception& exp) {
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<grape::response> grape::SaleManager::OnReturn(grape::request&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::post)
			co_return app->mNetManager.bad_request("expected a post");
		auto& body = req.body();
		if (body.empty()) 
			throw std::invalid_argument("Expected a body");

		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) && app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		auto&& [sid, buf2]  = grape::serial::read<grape::uid_t>(buf);
		auto&& [pid, buf3]  = grape::serial::read<grape::uid_t>(buf2);
		auto&& [quan, buf4] = grape::serial::read<boost::fusion::vector<std::int64_t>>(buf3);
		auto& sale_id  = boost::fusion::at_c<0>(sid);
		auto& prod_id  = boost::fusion::at_c<0>(pid);
		auto& quantity = boost::fusion::at_c<0>(quan);

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(CALL do_return(?,?,?,?,?,?);)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
			boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
			boost::mysql::field(boost::mysql::blob(sale_id.begin(), sale_id.end())),
			boost::mysql::field(boost::mysql::blob(prod_id.begin(), prod_id.end())),
			boost::mysql::field(static_cast<std::underlying_type_t<sale_state>>(sale_state::returned)),
			boost::mysql::field(quantity)
		} };
		auto d = co_await app->run_query(query);

		co_return app->OkResult("Sale returned");
	
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<grape::response> grape::SaleManager::OnGetSaleHistory(grape::request&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get)
			co_return app->mNetManager.bad_request("expected a get");
		auto& body = req.body();
		if (body.empty()) 
			throw std::invalid_argument("Expected a body");

		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) && app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		using stt = boost::fusion::vector<std::uint32_t, std::chrono::year_month_day, grape::sale_state>;
		auto&& [prod_id, buf2] = grape::serial::read<grape::uid_t>(buf);
		auto&& [st, buf3]      = grape::serial::read<stt>(buf2);
		auto&& [pg, buf4]      = grape::serial::read<grape::page>(buf3);
		auto& pid = boost::fusion::at_c<0>(prod_id);

		constexpr const std::array<std::string_view, 2> ymds = {
			"MONTH(s.sale_date)",
			"DAYOFMONTH(s.sale_date)"
		};

		if (boost::fusion::at_c<0>(st) >= ymds.size())
			throw std::invalid_argument("invalid date value");

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase);
		query->m_sql = std::format(R"(SELECT * 
			FROM ( SELECT s.id, s.prod_id, s.quanitity, s.total, a.account_username
			ROW_NUMER() OVER (ORDER BY p.sale_date) AS row_id
			FROM sales s
			JOIN acccounts a
			ON s.user_id = a.account_id
			WHERE s.pharmacy_id = ? AND s.branch_id = ? AND s.prod_id = ? AND {} = ?;
		) as sub
		HAVING row_id BETWEEN ? AND ?;)", ymds[boost::fusion::at_c<0>(st)].data());

		auto& item = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item.resize(6);

		item[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		item[2] = boost::mysql::field(boost::mysql::blob(pid.begin(), pid.end()));
		switch (boost::fusion::at_c<0>(st))
		{
		case 1:
			item[3] = boost::mysql::field((std::uint32_t)boost::fusion::at_c<1>(st).month());
			break;
		case 2:
			item[3] = boost::mysql::field((std::uint32_t)boost::fusion::at_c<1>(st).day());
			break;
		default:
			break;
		}
		item[4] = boost::mysql::field(pg.begin);
		item[5] = boost::mysql::field(pg.begin + pg.limit);

		auto data = co_await app->run_query(query);
		if (!data || data->empty())
			co_return app->mNetManager.not_found("no sale history for product");

		grape::collection_type<grape::sale_history> collect;
		auto& sh = boost::fusion::at_c<0>(collect);
		sh.reserve(data->size());
		for (auto& d : *data) {
			sh.emplace_back(grape::serial::build<sale_history>(d.first));
		}
		
		co_return app->OkResult(collect, req.keep_alive());
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
		co_return app->mNetManager.server_error(exp.what());
	}
}
