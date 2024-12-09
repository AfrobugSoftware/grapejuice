#include "ReportManager.h"
#include "Application.h"

grape::ReportManager::ReportManager()
{

}

void grape::ReportManager::SetRoute()
{
	auto app = grape::GetApp();
	app->route("/reports/pl", std::bind_front(&grape::ReportManager::OnGetProfitLoss, this));
	app->route("/reports/{e}", std::bind_front(&grape::ReportManager::OnGetEndOf, this));
	app->route("/reports/ebr", std::bind_front(&grape::ReportManager::OnGetEndByRange, this));
	app->route("/reports/purchased", std::bind_front(&grape::ReportManager::OnGetInventoryPurchased, this));
}

boost::asio::awaitable<grape::response> grape::ReportManager::OnGetProfitLoss(grape::request&& req, boost::urls::matches&& match)
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
		auto&& [st, buf2] = grape::serial::read<grape::stt>(buf);
		auto&& [pg, buf3] = grape::serial::read<grape::page>(buf2);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase);
		query->m_sql = fmt::format(
			R"(SELECT * FROM ( SELECT p.id,
			 s.sale_id, 
			 s.sale_date, 
			 p.name,
			 s.quantity,
			 s.total,
			 s.unit_cost_price,
			 s.payment_method,
			ROW_NUMER() OVER (ORDER BY s.sale_date) AS row_id
			FROM sales s
			INNER JOIN products p
			ON p.id = s.prouct_id
			WHERE  s.pharmacy_id = ? AND s.branch_id = ? AND sale_state = 0 AND {} = ? ) as sub
            HAVING row_id BETWEEN ? AND ?;)",
			ymds[st.dt].data());
		auto& item = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item.resize(5);
		item[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		switch (st.dt)
		{
		case 0:
			item[2] = boost::mysql::field((std::int32_t)boost::fusion::at_c<1>(st).year());
			break;
		case 1:
			item[2] = boost::mysql::field((std::uint32_t)boost::fusion::at_c<1>(st).month());
			break;
		case 2:
			item[2] = boost::mysql::field((std::uint32_t)boost::fusion::at_c<1>(st).day());
			break;
		default:
			break;
		}
		item[3] = boost::mysql::field(pg.begin);
		item[4] = boost::mysql::field(pg.begin + pg.limit);
		auto data = co_await app->run_query(query);
		if (!data || data->empty())
			co_return app->mNetManager.not_found("No report");
		grape::collection_type<grape::reports> rdata;
		auto& rd = boost::fusion::at_c<0>(rdata);
		rd.reserve(data->size());
		for (auto& d : *data)
		{
			auto& b = rd.emplace_back(grape::serial::build<grape::reports>(d.first));
			b.cost *= static_cast<double>(b.quantity);
		}
		co_return app->OkResult(rdata, req.keep_alive());
	}
	catch (const std::exception& exp)
	{
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));
		app->mNetManager.server_error(exp.what());
	}

}

boost::asio::awaitable<grape::response> grape::ReportManager::OnGetEndOf(grape::request&& req, boost::urls::matches&& match)
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
		boost::string_view m = match["e"];
		if (m.empty() || !boost::iequals(m, "eod") || !boost::iequals(m, "eom"))
		{
			co_return app->mNetManager.bad_request("Invalid request");
		}

		auto&& [st, buf2] = grape::serial::read<grape::stt>(buf);
		auto&& [pg, buf3] = grape::serial::read<grape::page>(buf2);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			fmt::format(R"((SELECT * FROM ( SELECT p.id,
			 s.sale_id, 
			 s.sale_date, 
			 p.name,
			 s.quantity,
			 s.total,
			 s.unit_cost_price,
			 s.payment_method,
			ROW_NUMER() OVER (ORDER BY s.sale_date) AS row_id
			FROM sales s
			INNER JOIN products p
			ON p.id = s.prouct_id
			WHERE  s.pharmacy_id = ? AND s.branch_id = ? AND {} = ? ) as sub
            HAVING row_id BETWEEN ? AND ?;)", ymds[st.dt]));
		auto& item = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item.resize(5);
		item[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		switch (st.dt)
		{
		case 1:
			item[2] = boost::mysql::field((std::uint32_t)boost::fusion::at_c<1>(st).month());
			break;
		case 2:
			item[2] = boost::mysql::field((std::uint32_t)boost::fusion::at_c<1>(st).day());
			break;
		default:
			break;
		}
		item[3] = boost::mysql::field(pg.begin);
		item[4] = boost::mysql::field(pg.begin + pg.limit);

		auto data = co_await app->run_query(query);
		if (!data || data->empty())
			co_return app->mNetManager.not_found("No month report");

		grape::collection_type<reports> ret;
		auto& r = boost::fusion::at_c<0>(ret);
		r.reserve(data->size());
		for (auto& d : *data) {
			r.emplace_back(grape::serial::build<grape::reports>(d.first));
		}

		co_return app->OkResult(ret, req.keep_alive());
	}
	catch (const std::exception& exp)
	{
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));
		app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<grape::response> grape::ReportManager::OnGetEndByRange(grape::request&& req, boost::urls::matches&& match)
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
		auto&& [stbeg, buf2] = grape::serial::read<grape::stt>(buf);
		auto&& [stend, buf3] = grape::serial::read<grape::stt>(buf2);
		auto&& [pg, buf4] = grape::serial::read<grape::page>(buf3);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"((SELECT * FROM ( SELECT p.id,
			 s.sale_id, 
			 s.sale_date, 
			 p.name,
			 s.quantity,
			 s.total,
			 s.unit_cost_price,
			 s.payment_method,
			ROW_NUMER() OVER (ORDER BY s.sale_date) AS row_id
			FROM sales s
			INNER JOIN products p
			ON p.id = s.prouct_id
			WHERE  s.pharmacy_id = ? AND s.branch_id = ? AND s.sale_date BETWEEN ? AND ?) as sub
            HAVING row_id BETWEEN ? AND ?;)");
		auto& item = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item.resize(6);
		item[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		item[2] = boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(std::chrono::sys_days{ stbeg.date })));
		item[3] = boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(std::chrono::sys_days{ stend.date })));
		item[4] = boost::mysql::field(pg.begin);
		item[5] = boost::mysql::field(pg.begin + pg.limit);

		auto data = co_await app->run_query(query);
		if (!data || data->empty())
			co_return app->mNetManager.not_found("No month report");

		grape::collection_type<reports> ret;
		auto& r = boost::fusion::at_c<0>(ret);
		r.reserve(data->size());
		for (auto& d : *data) {
			r.emplace_back(grape::serial::build<grape::reports>(d.first));
		}

		co_return app->OkResult(ret, req.keep_alive());

	}
	catch (const std::exception& exp)
	{
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));
		app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<grape::response> grape::ReportManager::OnGetInventoryPurchased(grape::request&& req, boost::urls::matches&& match)
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
	}
	catch (const std::exception& exp)
	{
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));
		app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<grape::response> grape::ReportManager::OnGetMonthly(grape::request&& req, boost::urls::matches&& match)
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
	}
	catch (const std::exception& exp)
	{
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));
		app->mNetManager.server_error(exp.what());
	}
}
