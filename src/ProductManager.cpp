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
				id binary(16) not null,
				serial_num integer,
				name text,
				generic_name text,
				class text,
				formulation text,
				strength text,
				strength_type text,
				usage_info text,
				description text,
				indications text,
				package_size integer,
				sideeffects text,
				barcode text,
				manufactures_name text,
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
				pharmacy_id binary(16) not null,
				branch_id binary(16) not null,
				inventory_id binary(16),
				proudct_id  binary(16),
				expire_date datetime,
				input_date datetime,
				stock_count integer,
				cost binary(17),
				supplier_uuid binary(16),
				lot_number text
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
				pharmacy_id binary(16),
				branch_id binary(16),
				pack_id binary(16),
				product_id binary(16),
				quantity integer,
				exact_cost binary(17)
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
			supplier_id binary(16),
			supplier_name text,
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
				pharmacy_id binary(16),
				branch_id binary(16),
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
				pharmacy_id binary(16),
				branch_id binary(16),
				supplier_id binary(16),
				id binary(16),
				product_id binary(16),
				inventory_id binary(16),
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

void grape::ProductManager::CreateExpiredTable()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS expired (
				pharmacy_id binary(16),
				branch_id binary(16),
				product_id binary(16),
				date datetime,
				stock_count integer,
			);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get();
	}
	catch (const boost::mysql::error_with_diagnostics& err) {
		spdlog::error(err.what());
	}
}

void grape::ProductManager::CreatePharmacyProductTable()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS pharma_products (
				pharmacy_id binary(16),
				branch_id binary(16),
				product_id binary(16),
				unitprice binary(17),
				costprice binary(17),
				stock_count largeint,
				min_stock_count largeint,
				date_added datetime,
				date_expire datetime,
				category_id integer,
			);)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get();
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
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
	app->route("/product/add"s, std::bind_front(&grape::ProductManager::OnAddProduct, this));
	app->route("/product/update"s, std::bind_front(&grape::ProductManager::OnUpdateProduct, this));
	app->route("/product/getproducts"s, std::bind_front(&grape::ProductManager::OnGetProducts, this));
	app->route("/product/removeproducts"s, std::bind_front(&grape::ProductManager::OnRemoveProducts, this));
	app->route("/product/addproducts"s, std::bind_front(&grape::ProductManager::OnAddPharmacyProduct, this));
	app->route("/product/addformulary"s, std::bind_front(&grape::ProductManager::OnCreateFormulary, this));
	
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
	grape::ProductManager::OnAddProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::put) {
			co_return app->mNetManager.bad_request("Put method expected");
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}
		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		auto&& [products, buf2] = grape::serial::read<grape::collection::products>(buf);
		//wrtie product to database
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
		R"(INSERT INTO products (
				id,
				serial_num,
				name,
				generic_name,
				class,
				formulation,
				strength,
				strength_type,
				usage_info,
				description,
				package_size,
				sideeffects,
				barcode,
				manufactures_name) VALUES ( ?,?,?,?,?,?,?,?,?,?,?,?,?);)");
		query->m_arguments.resize(products.group.size());
		for (size_t i = 0; i < products.group.size(); i++) {
			auto& arg = query->m_arguments[i];
			arg.reserve(19);
			auto& prod = products.group[i];


			arg.emplace_back(boost::mysql::field(boost::mysql::blob(prod.id.begin(), prod.id.end())));
			arg.emplace_back(boost::mysql::field(prod.serial_num)); //product serial number
			
			boost::trim(prod.name);
			std::transform(prod.name.begin(), prod.name.end(), prod.name.begin(), [](char c) -> char {return std::tolower(c); });
			arg.emplace_back(boost::mysql::field(prod.name)); 
			
			boost::trim(prod.generic_name);
			std::ranges::transform(prod.generic_name, prod.generic_name.begin(), [](char c) -> char {return std::tolower(c);  });
			arg.emplace_back(boost::mysql::field(prod.generic_name)); //generic name

			arg.emplace_back(boost::mysql::field(prod.class_)); //class OTC, POM, P
			arg.emplace_back(boost::mysql::field(prod.formulation)); // formulation
			arg.emplace_back(boost::mysql::field(prod.strength)); // strength
			arg.emplace_back(boost::mysql::field(prod.strength_type)); //strength type
			arg.emplace_back(boost::mysql::field(prod.usage_info)); //usage info
			arg.emplace_back(boost::mysql::field(prod.description)); //description
			arg.emplace_back(boost::mysql::field(prod.indications)); //health conditions
			arg.emplace_back(boost::mysql::field(prod.package_size)); //package size
			arg.emplace_back(boost::mysql::field(prod.sideeffects)); // side effects
			arg.emplace_back(boost::mysql::field(prod.barcode)); // barcode
			arg.emplace_back(boost::mysql::field(prod.manufactures_name)); // manufacturres name
		}
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
			//the operation my have completed, but we timed out in waiting, how do we resolve this?
			co_return app->mNetManager.timeout_error();
		}

		(void)fut.get();
		co_return app->OkResult("Successfully added product");
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
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::put) {
			co_return app->mNetManager.bad_request("Put method expected");
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}

		if (!app->mAccountManager.AuthuriseRequest(req)) {
			co_return app->mNetManager.auth_error("Cannot authorize user");
		}
		
		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		auto&& [product_opt, buf2] = grape::serial::read<grape::product_opt>(buf);

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase);
		query->m_arguments.resize(1);
		constexpr static const std::array<std::string_view, 12> names = {
			"name",
			"generic_name",
			"class",
			"formulation",
			"strength",
			"strength_type",
			"usage_info",
			"package_size",
			"stock_count",
			"barcode",
			"manufactures_name"
		};

		std::ostringstream os;
		os << "UPDATE products SET";
		using range = boost::mpl::range_c<unsigned, 0, boost::mpl::size<grape::product_opt>::value>;
		boost::fusion::for_each(range(), [&](auto i) {
			using int_type = std::decay_t<decltype(i)>;
			using type = std::decay_t<decltype(boost::fusion::at<int_type>(product_opt))>;
			if constexpr (grape::is_optional_field<type>::value) {
				auto& p = boost::fusion::at<int_type>(product_opt);
				if (p.has_value()){
					if constexpr (int_type::value != 1) {
						os << ",";
					}
					os << fmt::format("{} = ?", names[int_type::value]);
					query->m_arguments[0].push_back(boost::mysql::field(p.value()));
				}
			}
		});
		os << "WHERE id = ?;";
		query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(product_opt.id.begin(), product_opt.id.end())));

		
		query->m_sql = std::move(os.str());
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
		if (ec != boost::asio::error::operation_aborted){
			co_return app->mNetManager.timeout_error();
		}

		(void)fut.get();
		co_return app->OkResult("Product Updated");
	}
	catch (const std::logic_error& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnGetProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get) {
			co_return app->mNetManager.bad_request("Get method expected");
		}
		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT p.id,
				p.serial_num,
				p.name,
				p.generic_name,
				p.class,
				p.formulation,
				p.strength,
				p.strength_type,
				p.usage_info,
				pp.sell_price,
				pp.cost_price,
				p.package_size,
				pp.stock_count,
				p.sideeffects,
				p.barcode,
				pp.category_id,
				pp.min_stock_count
	   FROM products p, pharma_products pp
       WHERE pp.pharmacy_id = ? AND pp.branch_id = ? AND p.id = pp.product_id;)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_arguments = { {
		boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
		boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()))	
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
		if (data->empty()) {
			co_return app->mNetManager.not_found("No products for pharmacy");
		}
		using vector_type = boost::fusion::vector<boost::uuids::uuid, std::uint64_t,
			std::string, std::string, std::string, std::string, std::string, std::string, std::string, pof::base::currency,
			pof::base::currency, std::uint64_t, std::uint64_t, std::string, std::string, std::uint64_t, std::uint64_t>;
		
		boost::fusion::vector<std::vector<vector_type>> ret;
		auto& group = boost::fusion::at_c<0>(ret);
	

		size_t size = 0, i = 0;
		group.resize(data->size());

		for (auto& d : *data) {
			auto& vec = group[i];
			grape::serial::compose(vec, d.first);
			size += grape::serial::get_size(vec);
			i++;
		}

		grape::response::body_type::value_type value(size, 0x00);
		grape::serial::write(boost::asio::buffer(value), ret);

		grape::response res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/octlet-stream");
		res.keep_alive(req.keep_alive());
		res.body() = std::move(value);
		res.prepare_payload();
		co_return res;
	}
	catch (const std::logic_error& err) {
		co_return app->mNetManager.bad_request(err.what());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

//long function, deleting products from all the points.
boost::asio::awaitable<pof::base::net_manager::res_t> 
	grape::ProductManager::OnRemoveProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::delete_) {
			co_return app->mNetManager.bad_request("Get method expected");
		}

		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Body is required");
		}
		
		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		
		//get product ID
		auto&& [product, buf2] = grape::serial::read<grape::pharma_product>(buf);


		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(CALL remove_pharma_product(?, ?, ?);)");
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		query->m_arguments = { {
		boost::mysql::field(boost::mysql::blob(product.product_id.begin(), product.product_id.end())),
		boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
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

		(void)fut.get();

		co_return app->OkResult("Remove product from pharmacy");
	}
	catch (const js::json::exception& jerr) {
		co_return app->mNetManager.bad_request(jerr.what());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnGetFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	return boost::asio::awaitable<pof::base::net_manager::res_t>();
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnGetProductsByFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	return boost::asio::awaitable<pof::base::net_manager::res_t>();
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnAddPharmacyProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::put) {
			co_return app->mNetManager.bad_request("Put method expected");
		}
		if (!req.has_content_length()) {
			co_return app->mNetManager.bad_request("Expected a body");
		}


	}
	catch (const std::exception exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnCreateFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	return boost::asio::awaitable<pof::base::net_manager::res_t>();
}

void grape::ProductManager::Procedures()
{
	RemovePharamProducts();
}

void grape::ProductManager::RemovePharamProducts()
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE PROCEDURE IF NOT EXISTS remove_pharma_product (IN pid CHAR(16), 
               IN bid CHAR(16), IN prodid CHAR(16) 
               BEGIN 
                   START TRANSACTION
						DELETE FROM pharma_products WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid
						DELETE FROM expired WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid
						DELETE FROM invoices WHERE WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid
						DELETE FROM packs WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid
						DELETE FROM orders WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid
						DELETE FROM categories WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid
						DELETE FROM inventory WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid
					COMMIT
			   END;)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get(); //block until complete
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
	}
}
