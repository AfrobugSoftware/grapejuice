#include "ProductManager.h"
#include "Application.h"

grape::ProductManager::ProductManager()
{
}

grape::ProductManager::~ProductManager()
{
}

void grape::ProductManager::CreateTables()
{
	CreateProductTable();
	CreateInventoryTable();
	CreatePackTable();
	CreateSupplierTable();
	CreateCategoryTable();
	CreateInvoiceTable();
}

void grape::ProductManager::CreateProductTable()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS products (
				pharmacy_id blob,
				branch_id blob,
				id blob,
				serial_num integer,
				name text,
				generic_name text,
				class text,
				formulation text,
				strength text,
				strength_type text,
				usage_info text,
				sell_price blob,
				cost_price blob,
				package_size integer,
				stock_count integer,
				sideeffects text,
				barcode text,
				category_id integer,
				min_stock_count integer
			);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get(); //block until complete

	}
	catch (boost::mysql::error_with_diagnostics& err) {
		spdlog::error(err.what());
	}
}

void grape::ProductManager::CreateInventoryTable()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS inventory (
				pharmacy_id blob,
				branch_id blob,
				inventory_id blob,
				proudct_id  blob,
				expire_date datetime,
				input_date datetime,
				stock_count integer,
				cost blob,
				supplier_uuid blob,
				lot_number integer
			);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get(); //block till complete;
	}
	catch (const boost::mysql::error_with_diagnostics& err) {
		spdlog::error(err.what());
	}
}

void grape::ProductManager::CreatePackTable()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS packs (
				pharmacy_id blob,
				branch_id blob,
				pack_id blob,
				product_id blob,
				quantity integer,
				exact_cost blob
			);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get();
	}
	catch (const boost::mysql::error_with_diagnostics& err) {
		spdlog::error(err.what());
	}
}

void grape::ProductManager::CreateSupplierTable()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS suppliers (
			supplier_id blob,
			date_created datetime,
			date_modified datetime,
			info text
		);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get();
	}
	catch (const boost::mysql::error_with_diagnostics& err) {
		spdlog::error(err.what());
	}
}

void grape::ProductManager::CreateCategoryTable()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXSITS categories (
				pharmacy_id blob,
				branch_id blob,
				category_id integer autoincrement,
				name text
			);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get();

	}
	catch (const boost::mysql::error_with_diagnostics& err) {
		spdlog::error(err.what());
	}
}

void grape::ProductManager::CreateInvoiceTable()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXSITS invoices (
				pharmacy_id blob,
				branch_id blob,
				supplier_id blob,
				id blob,
				product_id blob,
				inventory_id blob,
				input_date datetime
		);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get();
	}
	catch (const boost::mysql::error_with_diagnostics& err) {
		spdlog::error(err.what());
	}
}

std::pair<boost::uuids::uuid, boost::uuids::uuid> grape::ProductManager::SplitPidBid(boost::core::string_view str)
{
	std::array<boost::core::string_view, 2> out;
	int i = 0;
	for (const auto word : std::views::split(str, "_")) {
		out[i] = boost::core::string_view(word.begin(), word.end());
		i++;
	}
	return std::make_pair(boost::lexical_cast<boost::uuids::uuid>(out[0]),
			boost::lexical_cast<boost::uuids::uuid>(out[1]));
}

void grape::ProductManager::SetRoutes()
{
	auto app = grape::GetApp();
	app->route("/product/add/{pid_bid}"s, std::bind_front(&grape::ProductManager::OnAddProduct, this));
	app->route("/product/update/{pid_bid}"s, std::bind_front(&grape::ProductManager::OnUpdateProduct, this));
	app->route("/product/getproducts/{pid_bid}"s, std::bind_front(&grape::ProductManager::OnGetProducts, this));
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
	grape::ProductManager::OnAddProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (!app->mAccountManager.AuthuriseRequest(req)) {
			co_return app->mNetManager.auth_error("Cannot authorize user");
		}
		if (req.method() != http::verb::put) {
			co_return app->mNetManager.bad_request("Put method expected");
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}
		auto& pid_bid = match["pid_bid"];
		size_t pos = pid_bid.find_first_of("_");
		if (pos == boost::core::string_view::npos) {
			co_return app->mNetManager.bad_request("pid_bid in wrong format");
		}
		auto&& [pid, bid] = SplitPidBid(pid_bid);

		auto prodbuf = req.body().data();
		std::vector<char> buf(boost::asio::buffer_size(prodbuf));
		boost::asio::buffer_copy(boost::asio::buffer(buf), prodbuf);
		pof::base::data prodData;

		//unpack the payload
		pof::base::unpacker{prodData}(buf);

		if (prodData.empty()) {
			co_return app->mNetManager.unprocessiable("Product payload is corrupt/not processiable"s);
		}

		//wrtie product to database
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO products (
				pharmacy_id,
				branch_id,
				id,
				serial_num,
				name,
				generic_name,
				class,
				formulation,
				strength,
				strength_type,
				usage_info,
				sell_price,
				cost_price,
				package_size,
				stock_count,
				sideeffects,
				barcode,
				category_id,
				min_stock_count) VALUES ( ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);)");
		query->m_arguments.resize(prodData.size());
		for (size_t i = 0; i < prodData.size(); i++) {
			auto& arg = query->m_arguments[i];
			arg.reserve(19);
			auto& prod = prodData[i];
			auto& v = prod.first;

			auto& productid = boost::variant2::get<boost::uuids::uuid>(v[0]); //product id

			arg.emplace_back(boost::mysql::field(boost::mysql::blob(pid.begin(), pid.end())));
			arg.emplace_back(boost::mysql::field(boost::mysql::blob(bid.begin(), bid.end())));
			arg.emplace_back(boost::mysql::field(boost::mysql::blob(productid.begin(), productid.end())));
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::uint64_t>(v[1]))); //product serial number
			
			std::string& name = boost::variant2::get<std::string>(v[2]); //product name
			boost::trim(name);
			std::transform(name.begin(), name.end(), name.begin(), [](char c) -> char {return std::tolower(c); });

			arg.emplace_back(boost::mysql::field(name)); 
			
			std::string& gen_name = boost::variant2::get<std::string>(v[3]);
			boost::trim(gen_name);
			std::ranges::transform(gen_name, gen_name.begin(), [](char c) -> char {return std::tolower(c);  });
			arg.emplace_back(boost::mysql::field(gen_name)); //generic name

			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::string>(v[4]))); //class OTC, POM, P
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::string>(v[5]))); // formulation
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::string>(v[6]))); // strength
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::string>(v[7]))); //strength type
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::string>(v[8]))); //usage info
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::string>(v[9]))); //description
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::string>(v[10]))); //health conditions

			auto& cost_price = boost::variant2::get<boost::uuids::uuid>(v[11]);
			auto& sale_price = boost::variant2::get<boost::uuids::uuid>(v[12]);

			arg.emplace_back(boost::mysql::field(boost::mysql::blob(sale_price.begin(), sale_price.end()))); //unit price
			arg.emplace_back(boost::mysql::field(boost::mysql::blob(cost_price.begin(), cost_price.end()))); //cost price
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::uint32_t>(v[13]))); //package size
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::uint64_t>(v[14]))); // stock count
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::string>(v[15]))); // side effects
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::string>(v[16]))); // barcode
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::string>(v[17]))); // category
			arg.emplace_back(boost::mysql::field(boost::variant2::get<std::uint64_t>(v[18]))); // minimum stock count
		}
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_waittime->expires_after(60s);
		auto fut = query->get_future();

		//auto ec = co_await app->mDatabase->retry(query); //try to push into the queue multiple times

		auto&& [ec] = co_await query->m_waittime->async_wait();
		if (ec != boost::asio::error::operation_aborted) {
			//the operation my have completed, but we timed out in waiting, how do we resolve this?
			co_return app->mNetManager.timeout_error();
		}

		(void)fut.get();



	}
	catch (const js::json::exception& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnUpdateProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	return boost::asio::awaitable<pof::base::net_manager::res_t>();
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnGetProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	return boost::asio::awaitable<pof::base::net_manager::res_t>();
}
