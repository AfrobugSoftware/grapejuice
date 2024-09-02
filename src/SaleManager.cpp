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
}

void grape::SaleManager::CreateSaleTable()
{
	auto app = grape::GetApp();
	try {
		//sale_payment_method is an index into a sale_method table
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS sales  (
				pharmacy_id binary(16),
				branch_id binary(16),
				user_id binary(16),
				sale_id binary(16) NOT NULL,
				product_id binary(16),
				formulary_id binary(16),
				sale_date datetime,
				unit_cost_price binary(17),
				unit_sale_price binary(17),
				discount binary(17),
				total_amount binary(17),
				quantity integer,
				payment_method text,
				payment_addinfo text,
				product_label text,
				sale_state integer,
				sale_add_info text,
				PRIMARY KEY (sale_id)
			);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		auto d = fut.get();
	}
	catch (const std::exception& exp) {
		
		spdlog::error(exp.what());
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
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
		R"(
			BEGIN;
			INSERT IGNORE INTO sales VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);
			UPDATE pharma_products SET stock_count = stock_count - ? 
			WHERE pharmacy_id = ? AND branch_id = ? AND product_id = ?;
			COMMIT;	
		)"s);
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
		spdlog::error(exp.what());
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
			"MONTHDAY(s.sale_date)"
		};
		std::string sql = std::format(R"(
			SELECT * FROM ( SELECT s.*,
			ROW_NUMER() OVER (ORDER BY p.sale_date) AS row_id
			FROM sales
			WHERE s.pharmacy_id = ? AND s.branch_id = ? AND s.sale_state = ? 
			AND {} = ?;
		) as sub
		HAVING row_id BETWEEN ? AND ?;)", ymds[boost::fusion::at_c<0>(st)].data());
		query->m_sql = std::move(sql);
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
		spdlog::error(exp.what());
		co_return app->mNetManager.server_error(exp.what());
	}
}
