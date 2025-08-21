#include "ReportManager.h"
#include "Application.h"

grape::ReportManager::ReportManager()
{

}

void grape::ReportManager::SetRoutes()
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
		if (!checkDate(st.date)) co_return app->mNetManager.bad_request("Invalid date");

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
			WHERE  s.pharmacy_id = ? AND s.branch_id = ? AND sale_state = 0 AND  TIMESTAMPDIFF({}, '1970-01-01', s.sale_date) = ? ) as sub
            HAVING row_id BETWEEN ? AND ? LIMIT 1000;)",
			ymds[st.dt].data());
		auto& item = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item.resize(5);
		item[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		int epoch_count = 0;
		switch (st.dt)
		{
		case 0:
			epoch_count = static_cast<int>(st.date.year()) - 1970;
			break;
		case 1:
			epoch_count = (static_cast<int>(st.date.year()) - 1970) * 12 + (static_cast<unsigned>(st.date.month()) - 1);
			break;
		case 2:
		{
			auto sd = std::chrono::sys_days{ st.date };
			epoch_count = sd.time_since_epoch().count();
		}
			break;
		default:
			break;
		}
		item[2] = boost::mysql::field(epoch_count);
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
		co_return app->mNetManager.server_error(exp.what());
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
		if (m.empty() || (!boost::iequals(m, "eod") && !boost::iequals(m, "eom")))
		{
			co_return app->mNetManager.bad_request("Invalid request");
		}
		
		auto&& [st, buf2] = grape::serial::read<grape::stt>(buf);
		auto&& [pg, buf3] = grape::serial::read<grape::page>(buf2);
		if (!checkDate(st.date)) co_return app->mNetManager.bad_request("Invalid date");
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			fmt::format(R"(SELECT * FROM ( SELECT p.id,
			 s.sale_id, 
			 s.sale_date, 
			 p.name,
			 s.quantity,
			 s.total_amount,
			 s.unit_sale_price,
			 s.payment_method,
			ROW_NUMER() OVER (ORDER BY s.sale_date) AS row_id
			FROM sales s
			INNER JOIN products p
			ON p.id = s.product_id
			WHERE  s.pharmacy_id = ? AND s.branch_id = ?  AND TIMESTAMPDIFF({}, '1970-01-01', s.sale_date) = ? ) as sub
            HAVING row_id BETWEEN ? AND ?;)", ymds[st.dt]));
		auto& item = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item.resize(5);
		item[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		int epoch_count = 0;
		switch (st.dt)
		{
		case 1:
			epoch_count = (static_cast<int>(st.date.year()) - 1970) * 12 + (static_cast<unsigned>(st.date.month()) - 1);
			break;
		case 2:
		{
			auto sd = std::chrono::sys_days{ st.date };
			epoch_count = sd.time_since_epoch().count();
		}
			break;
		default:
			break;
		}
		item[2] = boost::mysql::field(epoch_count);
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
		co_return app->mNetManager.server_error(exp.what());
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
		if (!(checkDate(stbeg.date) && checkDate(stend.date))) co_return app->mNetManager.bad_request("Invalid date");

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
            HAVING row_id BETWEEN ? AND ? LIMIT 1000;)");
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
		co_return app->mNetManager.server_error(exp.what());
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
		auto&& [stbeg, buf2] = grape::serial::read<grape::stt>(buf);
		auto&& [pg,    buf4] = grape::serial::read<grape::page>(buf2);
		if (!checkDate(stbeg.date)) co_return app->mNetManager.bad_request("Invalid date");


		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			fmt::format(R"( SELECT * FROM ( SELECT 
			i.product_id,
			i.inventory_id,
			p.name,
			i.input_date,
			i.stock_count,
			i.cost,
			ROW_NUMBER() OVER (order by i.input_date) as row_id
			FROM inventory i
			INNER JOIN products
			ON i.product_id = p.id
			WHERE pharmacy_id = ? AND branch_id = ? AND TIMESTAMPDIFF({}, '1970-01-01', s.sale_date) = ? ) as sub
			HAVING row_id BETWEEN ? AND ? LIMIT 1000;)", ymds[stbeg.dt]));
		auto& item = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item.resize(5);
		item[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		int epoch_count = 0;
		switch (stbeg.dt)
		{
		case 1:
			epoch_count = (static_cast<int>(stbeg.date.year()) - 1970) * 12 + (static_cast<unsigned>(stbeg.date.month()) - 1);
			break;
		case 2:
		{
			const auto sd = std::chrono::sys_days{ stbeg.date };
			epoch_count = sd.time_since_epoch().count();
		}
		break;
		default:
			break;
		}
		item[2] = boost::mysql::field(epoch_count);
		item[3] = boost::mysql::field(pg.begin);
		item[4] = boost::mysql::field(pg.begin + pg.limit);
		auto data = co_await app->run_query(query);
		if (!data || data->empty()){
			co_return app->mNetManager.not_found("No stock purchased");
		}
		grape::collection_type<grape::inev_report> ret;
		auto& r = boost::fusion::at_c<0>(ret);
		r.reserve(data->size());
		for (auto& d : *data){
			r.emplace_back(grape::serial::build<grape::inev_report>(d.first));
		}

		co_return app->OkResult(ret, req.keep_alive());
	}
	catch (const std::exception& exp)
	{
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));
		co_return app->mNetManager.server_error(exp.what());
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
		co_return app->mNetManager.server_error(exp.what());
	}
}


boost::asio::awaitable<grape::response> 
grape::ReportManager::OnGetDashboardRecords(grape::request&& req, boost::urls::matches&& match)
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
		if (!checkDate(st.date)) co_return app->mNetManager.bad_request("Invalid date");

		//get product count
		const int year_epoch = (static_cast<int>(st.date.year()) - 1970) * 12 + (static_cast<unsigned>(st.date.month()) - 1);
		const int month_epoch = std::chrono::sys_days{st.date}.time_since_epoch().count();

		grape::dashboard db;

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT COUNT(p.id) FROM pharma_products p WHERE p.pharmacy_id = ? AND p.branch_id = ?; )");
		query->m_hold_connection = true;
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		auto reset = [&]() -> boost::asio::awaitable<void> {
			query->m_arguments.clear();
			auto ec = co_await query->close();
			if (ec) throw std::system_error(ec);
			query->m_promise = {};
		};

		auto& item = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item.resize(2);
		item[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));

		auto data = co_await app->run_query(query);
		if (!data || data->empty())
		{
			db.product_count = 0;
		}
		else {
			db.product_count = boost::variant2::get<0>((*data->begin()).first[0]);
		}
		co_await reset();

		query->m_sql = fmt::format(R"(SELECT COUNT(s.sale_id) FROM sales s
        WHERE s.pharmacy_id = ? AND s.branch_id = ? AND TIMESTAMPDIFF({}, '1970-01-01', s.sale_date) = ? ;)", ymds[st.dt]);
		auto& item2 = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item2.resize(3);
		item2[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item2[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		if (st.dt == 0) item2[2] = boost::mysql::field(year_epoch);
		else item2[2] = boost::mysql::field(month_epoch);

		data = co_await app->run_query(query);
		if (!data || data->empty())
		{
			db.sales_count = 0;
		}
		else {
			db.sales_count = boost::variant2::get<0>((*data->begin()).first[0]);
		}
		co_await reset();

		query->m_sql = R"(SELECT COUNT(p.id) FROM pharma_products p WHERE p.stock_count = 0 
		 s.pharmacy_id = ? AND s.branch_id = ?;)";
		auto& item3 = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item3.resize(2);
		item3[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(),  cred.pharm_id.end()));
		item3[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));

		data = co_await app->run_query(query);
		if (!data || data->empty())
		{
			db.out_of_stock = 0;
		}
		else {
			db.out_of_stock = boost::variant2::get<0>((*data->begin()).first[0]);

		}
		co_await reset();

		query->m_sql = fmt::format(R"(SELECT s.total FROM sales s
		WHERE s.pharmacy_id = ? AND s.branch_id = ? AND TIMESTAMPDIFF({}, '1970-01-01', s.sale_date) = ?;)", ymds[st.dt]);
		auto& item4 = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item4.resize(3);
		item4[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item4[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		if (st.dt == 0) item4[2] = boost::mysql::field(year_epoch);
		else item4[2] = boost::mysql::field(month_epoch);

		data = co_await app->run_query(query);
		if (!data || data->empty())
		{
			db.total_revenue = pof::base::currency{};
		}else {
			for (auto& d : *data) {
				db.total_revenue += boost::variant2::get<pof::base::currency>(d.first[0]);
			}
		}
		co_await reset();

		query->m_sql = fmt::format(R"(
				SELECT i.cost FROM inventory i WHERE i.pharmacy_id = ? AND branch_id = ? AND TIMESTAMPDIFF({}, '1970-01-01', i.input_date) = ?; 
		)", ymds[st.dt]);
		auto& item5 = query->m_arguments.emplace_back(std::vector<boost::mysql::field>{});
		item5.resize(3);
		item5[0] = boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()));
		item5[1] = boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()));
		if (st.dt == 0) item5[2] = boost::mysql::field(year_epoch);
		else item5[2] = boost::mysql::field(month_epoch);
		data = co_await app->run_query(query);
		if (!data || data->empty())
		{
			db.total_purchase = pof::base::currency{};
		}
		else {
			for (auto& d : *data) {
				db.total_purchase += boost::variant2::get<pof::base::currency>(d.first[0]);
			}
		}
		co_await reset();

		std::error_code ec = co_await query->close();
		if (ec) throw std::system_error(ec);
		query->unborrow();

		co_return app->OkResult(db, req.keep_alive());
	}
	catch (const std::exception& exp)
	{
		spdlog::error(std::format("{}: {}", std::source_location::current(), exp.what()));
		co_return app->mNetManager.server_error(exp.what());
	}
}

