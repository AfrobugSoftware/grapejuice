#include "ProductManager.h"
#include "Application.h"

bool grape::operator==(const product& a, const product& b) {
	return a.id == b.id;
}

std::size_t grape::hash_value(product const& b) {
	boost::hash<boost::uuids::uuid> hasher;
	return hasher(b.id);
}


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
	CreatePharmacyProductTable();
	CreateExpiredTable();
	CreateFormularyTable();
	CreateOrderTable();
	CreateWarningTable();
	CreateBranchTransferPendingTable();
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
				manufactures_name text
			);)");
		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);
		if (pushed)(void)fut.get(); //block until complete
		else{
			throw std::logic_error("Cannot get connection to database");
		}

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
		bool pushed = app->mDatabase->push(query);
		if (pushed) (void)fut.get(); //block till complete;
		else{
			throw std::logic_error("Cannot get connection to database");
		}
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
		bool pushed = app->mDatabase->push(query);
		if (pushed)(void)fut.get();
		else {
			throw std::logic_error("Cannot get connection to database");
		}
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
		bool pushed = app->mDatabase->push(query);
		if(pushed)(void)fut.get();
		else{
			throw std::logic_error("Cannot get connection to database");
		}
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
			R"(CREATE TABLE IF NOT EXISTS categories (
				pharmacy_id binary(16),
				branch_id binary(16),
				category_id integer,
				name text
			);)");
		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);
		if (pushed) (void)fut.get();
		else{
			throw std::logic_error("Cannot get connection to database");
		}

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
			R"(CREATE TABLE IF NOT EXISTS invoices (
				pharmacy_id binary(16),
				branch_id binary(16),
				supplier_id binary(16),
				id binary(16),
				product_id binary(16),
				inventory_id binary(16),
				input_date datetime
		);)");
		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);
		if(pushed)(void)fut.get();
		else {
			throw std::logic_error("Cannot get connection to database");
		}
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
				date_expired datetime,
				stock_count integer
			);)");
		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);
		if(pushed)(void)fut.get();
		else {
			throw std::logic_error("Cannot get connection to database");
		}
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
				stock_count integer,
				min_stock_count integer,
				date_added datetime,
				date_expire datetime,
				category_id integer
			);)");
		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);
		if(pushed)(void)fut.get();
		else {
			throw std::logic_error("Cannot get connection to database");
		}
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
	}
}

void grape::ProductManager::CreateFormularyTable() {
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS formulary (
				id binary(16),
				creator_id binary(16),
				name text,
				created_by text,
				created_date datetime,
				version text,
				access_level integer
		);)");
		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);
		if (pushed)(void)fut.get();
		else {
			throw std::logic_error("Cannot get connection to database");
		}

		query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS formulary_content (
				formulary_id binary(16),
				product_id binary(16)
		);)");
		fut = std::move(query->get_future());
		pushed = app->mDatabase->push(query);
		if(pushed)(void)fut.get();
		else {
			throw std::logic_error("Cannot get connection to database");
		}
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
	}
}

void grape::ProductManager::CreateOrderTable()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS orders (
				state integer,
				id binary(16),
				pharmacy_id binary(16),
				branch_id binary(16),
				product_id binary(16),
				total_cost binary(17),
				quantity integer
		);)");
		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);
		if (pushed)(void)fut.get();
		else {
			throw std::logic_error("Cannot get connection to database");
		}

	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
	}
}

void grape::ProductManager::CreateWarningTable()
{
	try {
		auto app = grape::GetApp();
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS warning (
				state integer,
				id binary(16),
				pharmacy_id binary(16),
				branch_id binary(16),
				product_id binary(16),
				warning_text text
			);)");
		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);
		if (pushed)(void)fut.get();
		else {
			throw std::logic_error("Cannot get connection to database");
		}
	}
	catch (std::exception& exp) {
		spdlog::error(exp.what());
	}
}

void grape::ProductManager::CreateBranchTransferPendingTable(){
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE TABLE IF NOT EXISTS pending_transfers (
				pend_id integer PRIMARY KEY AUTO_INCREMENT,
				pharmacy_id binary(16),
				from_branch_id binary(16),
				to_branch_id binary(16),
				product_list blob
			))");
		auto fut = query->get_future();
		bool pushed = app->mDatabase->push(query);
		if (pushed)(void)fut.get();
		else {
			throw std::logic_error("Cannot get connection to database");
		}
	}
	catch (std::exception exp) {
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
	app->route("/product/updatepharmaproduct"s, std::bind_front(&grape::ProductManager::OnUpdatePharmaProduct, this));

	app->route("/product/formulary/add"s, std::bind_front(&grape::ProductManager::OnCreateFormulary, this));
	app->route("/product/formulary/remove", std::bind_front(&grape::ProductManager::OnRemoveFormulary, this));
	app->route("/product/formulary/get", std::bind_front(&grape::ProductManager::OnGetFormulary, this));
	app->route("/product/formulary/getproducts", std::bind_front(&grape::ProductManager::OnGetProductsByFormulary, this));
	app->route("/product/formulary/getformularyproducts", std::bind_front(&grape::ProductManager::OnGetFormularyProducts, this));

	app->route("/product/inventory/add"s, std::bind_front(&grape::ProductManager::OnAddInventory, this));
	app->route("/product/inventory/remove"s, std::bind_front(&grape::ProductManager::OnRemoveInventory, this));
	app->route("/product/inventory/update"s, std::bind_front(&grape::ProductManager::OnUpdateInventory, this));
	app->route("/product/inventory/get"s, std::bind_front(&grape::ProductManager::OnGetInventory, this));
	app->route("/product/inventory/getcount"s, std::bind_front(&grape::ProductManager::OnGetInventoryCount, this));

	
	app->route("/product/category/add", std::bind_front(&grape::ProductManager::OnAddCategory, this));
	app->route("/product/category/remove", std::bind_front(&grape::ProductManager::OnRemoveCategory, this));
	app->route("/product/category/update", std::bind_front(&grape::ProductManager::OnUpdateCategory, this));
	app->route("/product/category/get", std::bind_front(&grape::ProductManager::OnGetCategory, this));

	app->route("/product/expired/mark", std::bind_front(&grape::ProductManager::OnMarkAsExpired, this));
	app->route("/product/expired/get", std::bind_front(&grape::ProductManager::OnGetExpiredProducts, this));

	app->route("/product/branch/transfer", std::bind_front(&grape::ProductManager::OnTransferProductsToBranch, this));
	app->route("/product/branch/getpendtransfers", std::bind_front(&grape::ProductManager::OnGetBranchPendTransfers, this));
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
	
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Requires an argument");

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
					if constexpr (std::disjunction_v<std::is_same<typename type::value_type, boost::uuids::uuid>,
						std::is_array<typename type::value_type>>) {
						query->m_arguments[0].push_back(boost::mysql::field(
							boost::mysql::blob(p.value().begin(),
								p.value().end())));
					}
					else if constexpr (std::is_same<typename type::value_type, pof::base::currency>::value) {
						typename type::value_type& t = p.value();
						query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(
							t.data().begin(), t.data().end())));
					}
					else if constexpr (std::is_same_v<typename type::value_type, std::chrono::system_clock::time_point>) {
						typename type::value_type& t = p.value();
						query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<
							boost::mysql::datetime::time_point::duration>(t))));
					}
					else {
						typename type::value_type& t = p.value();
						query->m_arguments[0].push_back(boost::mysql::field(t));
					}
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
		if (body.empty()) throw std::invalid_argument("Requires an argument");

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
		if (body.empty()) throw std::invalid_argument("Requires an argument");

		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		
		//get product ID
		auto&& [product, buf2] = grape::serial::read<grape::pharma_product>(buf);


		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(CALL remove_pharma_product(?, ?, ?);)");
		query->m_arguments = { {
		boost::mysql::field(boost::mysql::blob(product.product_id.begin(), product.product_id.end())),
		boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
		boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()))
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
			R"(SELECT * FROM formulary WHERE access_level = ? OR creator_id = ? LIMIT 100;)");
		query->m_arguments = { {
			boost::mysql::field(static_cast<std::underlying_type_t<
				grape::formulary_access_level>>(grape::formulary_access_level::ACCESS_PUBLIC)),
			boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end()))
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
		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("No formularies");
		}

		grape::collection_type<grape::formulary> forms;
		auto& v = boost::fusion::at_c<0>(forms);
		v.reserve(data->size());

		for (const auto& d : *data) {
			v.emplace_back(grape::serial::build<grape::formulary>(d.first));
		}

		co_return app->OkResult(forms);
	}
	catch (const std::exception& exp) {
		co_return  app->mNetManager.server_error(exp.what());
	}
}
//gets the products in this formulary that are also in the pharmacy
boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnGetProductsByFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
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
		auto&& [form, buf2] = grape::serial::read<grape::formulary>(buf);
		auto&& [pg, buf3] = grape::serial::read<grape::page>(buf2);

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT  p.id,
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
				p.manufactures_name,
				pp.category_id,
				pp.min_stock_count
			ROW_NUMBER() OVER ( ORDER BY p.name) AS row_id
			FROM products p, pharma_product pp, formulary_content fc
			WHERE pp.product_id = p.id AND  fc.product_id = p.id AND fc.id = ? AND row_id BETWEEN ? AND ?;)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(form.id.begin(), form.id.end())),
			boost::mysql::field(pg.begin),
			boost::mysql::field(pg.begin + pg.limit)
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
		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("No formularies");
		}
		using vector_type = boost::fusion::vector<boost::uuids::uuid, std::uint64_t,
			std::string, std::string, std::string, std::string, std::string, std::string, std::string, pof::base::currency,
			pof::base::currency, std::uint64_t, std::uint64_t, std::string, std::string, std::uint64_t, std::uint64_t, std::uint64_t>;
		grape::collection_type<vector_type> collection;
		auto& v = boost::fusion::at_c<0>(collection);
		v.reserve(data->size());

		for (auto& d : *data) {
			v.emplace_back(grape::serial::build<vector_type>(d.first));
		}

		co_return app->OkResult(collection);
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnGetFormularyProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
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
		auto&& [form, buf2] = grape::serial::read<grape::formulary>(buf);
		auto&& [pg, buf3] = grape::serial::read<grape::page>(buf2);
		
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT p.id
				p.serial_num,
				p.name,
				p.generic_name,
				p.class,
				p.formulation,
				p.strength,
				p.strength_type,
				p.usage_info,
				p.sideeffects,
				p.barcode,
				p.manufactures_name,
			ROW_NUMBER() OVER ( ORDER BY p.name) AS row_id
			FROM products p, formulary_content fc
			WHERE p.id = fc.product_id AND fc.formulary_id = ? AND row_id BETWEEN ? AND ?;)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(form.id.begin(), form.id.end())),
			boost::mysql::field(pg.begin),
			boost::mysql::field(pg.begin + pg.limit)
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
		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("No formularies");
		}

		collection_type<grape::product> products;
		auto& v = boost::fusion::at_c<0>(products);
		v.reserve(data->size());

		for (auto& d : *data) {
			v.emplace_back(grape::serial::build<grape::product>(d.first));
		}
		co_return app->OkResult(products);
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
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

		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		auto&& [pp, buf2] = grape::serial::read<grape::pharma_product>(buf);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO pharma_product VALUES (?,?,?,?,?,?,?,?,?,?);)");

		query->m_arguments = { {
				grape::serial::make_mysql_arg(pp)
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

		(void)fut.get();
		
		co_return app->OkResult("Product added");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnCreateFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	thread_local static boost::uuids::random_generator_mt19937 uuidGen{};
	try{
		if (req.method() != http::verb::get) {
			co_return app->mNetManager.bad_request("Get method expected");
		}
		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		auto&& [form, buf2] = grape::serial::read<grape::formulary>(buf);
		form.id = uuidGen();
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO formulary VALUES (?,?,?,?,?,?,?);)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(form.id.begin(), form.id.end())),
			boost::mysql::field(boost::mysql::blob(form.creator_id.begin(), form.creator_id.end())),
			boost::mysql::field(form.name),
			boost::mysql::field(form.created_by),
			boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<boost::mysql::datetime::time_point::duration>(form.created_date))),
			boost::mysql::field(form.version),
			boost::mysql::field(static_cast<std::underlying_type_t<grape::formulary_access_level>>(form.access_level))
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

		(void)fut.get();

		co_return app->OkResult(fmt::format("Created {} formulary", form.name));
	}
	catch (const std::logic_error& lerr) {
		co_return app->mNetManager.bad_request(lerr.what());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t>
grape::ProductManager::OnRemoveFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::delete_) {
			co_return app->mNetManager.bad_request("Get method expected");
		}

		auto& body = req.body();
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		auto&& [form, buf2] = grape::serial::read<grape::formulary>(buf);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(CALL remove_formulary(?);)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(form.id.begin(),
			 form.id.end()))
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
			
		co_return app->OkResult("Formulary removed");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}


boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnUpdatePharmaProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
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

		auto&& [pp_opt, buf2] = grape::serial::read<grape::pharma_product_opt>(buf);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase);
		query->m_arguments.resize(1);
		constexpr static const std::array<std::string_view, 7> names = {
			"unitprice",
			"costprice",
			"stock_count",
			"min_stock_count",
			"date_added",
			"date_expire",
			"category_id"
		};
		std::ostringstream os;
		os << "UPDATE pharma_products SET ";
		using range = boost::mpl::range_c<unsigned, 0, boost::mpl::size<grape::pharma_product_opt>::value>;
		boost::fusion::for_each(range(), [&](auto i) {
			using int_type = std::decay_t<decltype(i)>;
			using type = std::decay_t<decltype(boost::fusion::at<int_type>(pp_opt))>;
			if constexpr (grape::is_optional_field<type>::value) {
				type& p = boost::fusion::at<int_type>(pp_opt);
				if (p.has_value()) {
					if constexpr (int_type::value != 1) {
						os << ",";
					}
					os << fmt::format("{} = ?", names[int_type::value]);
					if constexpr (std::disjunction_v<std::is_same<typename type::value_type, boost::uuids::uuid>,
						std::is_array<typename type::value_type>>) {
						query->m_arguments[0].push_back(boost::mysql::field(
							boost::mysql::blob(p.value().begin(),
								p.value().end())));
					}
					else if constexpr (std::is_same<typename type::value_type, pof::base::currency>::value) {
						typename type::value_type& t = p.value();
						query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(
							t.data().begin(), t.data().end())));
					}
					else if constexpr (std::is_same_v<typename type::value_type, std::chrono::system_clock::time_point>) {
						typename type::value_type& t = p.value();
						query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<
							boost::mysql::datetime::time_point::duration>(t))));
					}
					else {
						typename type::value_type& t = p.value();
						query->m_arguments[0].push_back(boost::mysql::field(t));
					}
				}
			}
			});
		os << "WHERE product_id = ? AND pharmacy_id = ? AND branch_id = ?;";
		query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(pp_opt.product_id.begin(), pp_opt.product_id.end())));
		query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(pp_opt.pharmacy_id.begin(), pp_opt.pharmacy_id.end())));
		query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(pp_opt.branch_id.begin(), pp_opt.branch_id.end())));

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

	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}


void grape::ProductManager::Procedures()
{
	RemovePharamProducts();
	RemoveCategory();
	RemoveFormulary();
}

void grape::ProductManager::RemovePharamProducts()
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE PROCEDURE IF NOT EXISTS remove_pharma_product (IN pid BINARY(16), IN bid BINARY(16), IN prodid BINARY(16) )
               BEGIN 
                   START TRANSACTION;
						DELETE FROM pharma_products WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid;
						DELETE FROM expired WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid;
						DELETE FROM invoices WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid;
						DELETE FROM packs WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid;
						DELETE FROM orders WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid;
						DELETE FROM inventory WHERE product_id = prodid AND branch_id = bid AND pharmacy_id = pid;
					COMMIT;
			   END;)");
		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get(); //block until complete
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
	}
}

void grape::ProductManager::RemoveCategory()
{
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE PROCEDURE IF NOT EXISTS remove_category(IN pharm_id binary(16), IN bid binary(16), IN cid integer)
			   BEGIN
				START TRANSACTION;
					UPDATE pharma_products SET category_id = 0 WHERE pharmacy_id = pharm_id AND branch_id = bid AND category_id = cid;
					DELETE FROM categories WHERE category_id  = cid;
				COMMIT;
			  END;
		)");

		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get(); //block until complete
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnAddInventory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	thread_local static boost::uuids::random_generator_mt19937 uuidGen{};
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::post) {
			co_return app->mNetManager.bad_request("Post method expected");
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

		auto&& [inven, buf2] = grape::serial::read<grape::inventory>(buf);
		inven.id = uuidGen();
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO inventory VALUES (?,?,?,?,?,?,?,?,?,?);)");
		query->m_arguments = {{
			boost::mysql::field(boost::mysql::blob(inven.pharmacy_id.begin(), inven.pharmacy_id.end())),
			boost::mysql::field(boost::mysql::blob(inven.branch_id.begin(), inven.branch_id.end())),
			boost::mysql::field(boost::mysql::blob(inven.id.begin(), inven.id.end())),
			boost::mysql::field(boost::mysql::blob(inven.product_id.begin(), inven.product_id.end())),
			boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<
							boost::mysql::datetime::time_point::duration>(inven.expire_date))),
			boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<
							boost::mysql::datetime::time_point::duration>(inven.input_date))),
			boost::mysql::field(inven.stock_count),
			boost::mysql::field(boost::mysql::blob(inven.cost.data().begin(), inven.cost.data().end())),
			boost::mysql::field(boost::mysql::blob(inven.supplier_id.begin(), inven.supplier_id.end())),
			boost::mysql::field(inven.lot_number)
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

		(void)fut.get();

		co_return app->OkResult("Inventory added");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}

}
boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnRemoveInventory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::delete_) {
			co_return app->mNetManager.bad_request("Delete method expected");
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

		auto&& [inven, buf2] = grape::serial::read<grape::inventory>(buf);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(DELETE FROM inventory WHERE id = ? AND product_id = ? AND pharmacy_id = ? AND branch_id = ?;)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(inven.id.begin(), inven.id.end())),
			boost::mysql::field(boost::mysql::blob(inven.product_id.begin(), inven.product_id.end())),
			boost::mysql::field(boost::mysql::blob(inven.pharmacy_id.begin(), inven.pharmacy_id.end())),
			boost::mysql::field(boost::mysql::blob(inven.branch_id.begin(), inven.branch_id.end()))
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

		co_return app->OkResult("Inventory removed");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}

}
boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnUpdateInventory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
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

		using update_type = boost::fusion::vector<
			boost::uuids::uuid,
			boost::uuids::uuid,
			boost::uuids::uuid,
			boost::uuids::uuid,
			opt_fields,
			opt_field_time_point<0>,
			opt_field_time_point<2>,
			opt_field_uint64_t<3>,
			opt_field_currency<4>,
			opt_field_string<5>
		>;
		auto&& [uinven, buf2] = grape::serial::read<update_type>(buf);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase);
		query->m_arguments.resize(1);
		constexpr static const std::array<std::string_view, 8> names = {
			"pharmacy_id",
			"branch_id",
			"id",
			"product_id",
			"expire_date",
			"input_date",
			"cost",
			"lot_number",
		};

		std::ostringstream os;
		os << "UPDATE inventory SET ";
		using range = boost::mpl::range_c<unsigned, 0, boost::mpl::size<update_type>::value>;
		boost::fusion::for_each(range(), [&](auto i) {
			using int_type = std::decay_t<decltype(i)>;
			using type = std::decay_t<decltype(boost::fusion::at<int_type>(uinven))>;
			if constexpr (grape::is_optional_field<type>::value) {
				type& p = boost::fusion::at<int_type>(uinven);
				if (p.has_value()) {
					if constexpr (int_type::value != 1) {
						os << ",";
					}
					os << fmt::format("{} = ?", names[int_type::value]);
					if constexpr (std::disjunction_v<std::is_same<typename type::value_type, boost::uuids::uuid>,
						std::is_array<typename type::value_type>>) {
						query->m_arguments[0].push_back(boost::mysql::field(
							boost::mysql::blob(p.value().begin(),
								p.value().end())));
					}
					else if constexpr (std::is_same<typename type::value_type, pof::base::currency>::value) {
						typename type::value_type& t = p.value();
						query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(
							t.data().begin(), t.data().end())));
					}
					else if constexpr (std::is_same_v<typename type::value_type, std::chrono::system_clock::time_point>) {
						typename type::value_type& t = p.value();
						query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::datetime(std::chrono::time_point_cast<
							boost::mysql::datetime::time_point::duration>(t))));
					}
					else {
						typename type::value_type& t = p.value();
						query->m_arguments[0].push_back(boost::mysql::field(t));
					}
				}
			}
			});

		os << "WHERE pharmacy_id = ? AND branch_id = ?  AND id = ? AND product_id = ?;";
		query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(boost::fusion::at_c<0>(uinven).begin(), boost::fusion::at_c<0>(uinven).end())));
		query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(boost::fusion::at_c<1>(uinven).begin(), boost::fusion::at_c<1>(uinven).end())));
		query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(boost::fusion::at_c<2>(uinven).begin(), boost::fusion::at_c<2>(uinven).end())));
		query->m_arguments[0].push_back(boost::mysql::field(boost::mysql::blob(boost::fusion::at_c<3>(uinven).begin(), boost::fusion::at_c<3>(uinven).end())));

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

		co_return app->OkResult("Inventory updated");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}
boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnGetInventory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get) {
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
		auto&& [prod_id, buf2] = grape::serial::read<grape::product_identifier>(buf);
		auto&& [pg, buf3] = grape::serial::read<grape::page>(buf2);

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT pharmacy_id,
			branch_id, id, product_id, expire_date, input_date, stock_count, cost, supplier_id,lot_number,
			ROW_NUMBER() OVER (PARTITION BY product_id ORDER BY input_date DESC) AS row_id 
			FROM inventory 
			WHERE pharmacy_id = ? AND branch_id = ? AND product_id = ? AND row_id BETWEEN ? AND ?;)");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(prod_id.pharmacy_id.begin(), prod_id.pharmacy_id.end())),
			boost::mysql::field(boost::mysql::blob(prod_id.branch_id.begin(),	prod_id.branch_id.end())),
			boost::mysql::field(boost::mysql::blob(prod_id.product_id.begin(), prod_id.product_id.end())),
			boost::mysql::field(pg.begin),
			boost::mysql::field(pg.limit)
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
		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("No inventory for product");
		}

		grape::collection_type<grape::inventory> collect;
		auto& collection = boost::fusion::at_c<0>(collect);
		collection.reserve(data->size());
		for (auto&& d : *data) {
			collection.emplace_back(grape::serial::build<grape::inventory>(d.first));
		}
		
		//create payload
		const size_t size = grape::serial::get_size(collect);
		grape::response::body_type::value_type value(size, 0x00);
		grape::serial::write(boost::asio::buffer(value), collect);

		grape::response res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/octlet-stream");
		res.keep_alive(req.keep_alive());
		res.body() = std::move(value);
		res.prepare_payload();
		co_return res;
	}
	catch (const std::exception& exp) {
		
	}
}
boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnGetInventoryCount(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get) {
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
		auto&& [prod_id, buf2] = grape::serial::read<grape::product_identifier>(buf);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT COUNT(id) FROM inventory 
			WHERE pharmacy_id = ? AND branch_id = ? AND product_id = ? GROUP BY product_id;)");
		query->m_arguments = { {
		boost::mysql::field(boost::mysql::blob(prod_id.pharmacy_id.begin(), prod_id.pharmacy_id.end())),
		boost::mysql::field(boost::mysql::blob(prod_id.branch_id.begin(),	prod_id.branch_id.end())),
		boost::mysql::field(boost::mysql::blob(prod_id.product_id.begin(), prod_id.product_id.end())),
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
		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("No inventory for product");
		}
		const size_t size = boost::variant2::get<std::uint64_t>(data->begin()->first[0]);

		grape::response::body_type::value_type value(sizeof(std::uint64_t), 0x00);
		*boost::asio::buffer_cast<std::uint64_t*>(boost::asio::buffer(value)) =  grape::bswap(size);

		grape::response res{ http::status::ok, 11 };
		res.set(http::field::server, USER_AGENT_STRING);
		res.set(http::field::content_type, "application/octlet-stream");
		res.keep_alive(req.keep_alive());
		res.body() = std::move(value);
		res.prepare_payload();
		co_return res;
		
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnAddCategory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::post) {
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
		auto&& [categories, buf2] = grape::serial::read<grape::collection_type<grape::category>>(buf);
		auto& cc = boost::fusion::at_c<0>(categories);
		
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO category VALUES (?,?,?,?))");
		query->m_arguments.reserve(cc.size());
		for (auto&& c : cc) {
			query->m_arguments.emplace_back(grape::serial::make_mysql_arg(c));
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
			co_return app->mNetManager.timeout_error();
		}

		(void)fut.get();

		co_return app->OkResult("Added category");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnRemoveCategory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::delete_) {
			co_return app->mNetManager.bad_request("Delete method expected");
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

		auto&& [category, buf2] = grape::serial::read<grape::category>(buf);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(CALL remove_category(?,?,?))");
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(category.pharmacy_id.begin(), category.pharmacy_id.end())),
			boost::mysql::field(boost::mysql::blob(category.branch_id.begin(), category.branch_id.end())),
			boost::mysql::field(category.category_id)
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

		co_return app->OkResult("Removed category");

	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnUpdateCategory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
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

		auto&& [category, buf2] = grape::serial::read<grape::category>(buf);
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE category SET name = ?	WHERE pharmacy_id = ? AND branch_id = ? AND category_id = ?;)");
		query->m_arguments = { {
			boost::mysql::field(category.name),
			boost::mysql::field(boost::mysql::blob(category.pharmacy_id.begin(), category.pharmacy_id.end())),
			boost::mysql::field(boost::mysql::blob(category.branch_id.begin(), category.branch_id.end())),
			boost::mysql::field(category.category_id)
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

		co_return app->OkResult("Category updated");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}

}
boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnGetCategory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get) {
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

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT * FROM categories WHERE pharmacy_id = ? AND branch_id = ?;)");
		query->m_arguments = { {
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
		auto data = fut.get();
		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("No inventory for product");
		}
		grape::collection_type<grape::category> categories;
		auto& v = boost::fusion::at_c<0>(categories);
		v.reserve(data->size());
		for (const auto& d : *data) {
			v.emplace_back(grape::serial::build<grape::category>(d.first));
		}
		
		co_return app->OkResult(categories, req.keep_alive());
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}
boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnAddFormularyProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::put) {
			co_return app->mNetManager.bad_request("Put method expected");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected an argument");
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		
		//looks weird but it is like a struct that holds just the uuid,
		//without all the syntaxic sugar is it a tigethly packed array of uuids.
		auto&& [form, buf2] = grape::serial::read<grape::formulary>(buf);
		using pid = grape::collection_type<boost::fusion::vector<boost::uuids::uuid>>;
	
		auto&& [pids, buf3] = grape::serial::read<pid>(buf2);
		auto& v = boost::fusion::at_c<0>(pids);
		if (v.empty()) {
			co_return app->mNetManager.bad_request("No products to add");
		}

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO formulary_content VALUES (?,?);)");
		query->m_arguments.reserve(v.size());
		for (auto& p : v) {
			auto& i = boost::fusion::at_c<0>(p);
			query->m_arguments.emplace_back(std::vector<boost::mysql::field>{
				boost::mysql::field(boost::mysql::blob(form.id.begin(), form.id.end())),
				boost::mysql::field(boost::mysql::blob(i.begin(), i.end()))
			});
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
			co_return app->mNetManager.timeout_error();
		}

		(void)fut.get();

		co_return app->OkResult("Products added to formulary");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}
boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnRemoveFormularyProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::delete_) {
			co_return app->mNetManager.bad_request("Delete method expected");
		}

		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected a body");
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		auto&& [form, buf2] = grape::serial::read<grape::formulary>(buf);
		auto&& [pids, buf3] = grape::serial::read<grape::pid>(buf2);

		auto& v = boost::fusion::at_c<0>(pids);
		if (v.empty()) {
			co_return app->mNetManager.bad_request("No products to remove");
		}

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(DELETE FROM formulary_content WHERE formulary_id = ? AND product_id = ?;)");
		query->m_arguments.reserve(v.size());
		for (auto& p : v) {
			auto& i = boost::fusion::at_c<0>(p);
			query->m_arguments.emplace_back(std::vector<boost::mysql::field>{
				boost::mysql::field(boost::mysql::blob(form.id.begin(), form.id.end())),
					boost::mysql::field(boost::mysql::blob(i.begin(), i.end()))
			});
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
			co_return app->mNetManager.timeout_error();
		}

		(void)fut.get();

		co_return app->OkResult("Products removed from formulary");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}
boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnSearchFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::search){
			co_return app->mNetManager.bad_request("Search method needed");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected a body");
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		auto&& [form, buf2] = grape::serial::read<grape::formulary>(buf);

		using searchtype = boost::fusion::vector<std::uint32_t, std::string>;
		auto&& [str, buf3] = grape::serial::read<searchtype>(buf2);

		auto& stype = boost::fusion::at_c<0>(str);
		auto& ss = boost::fusion::at_c<1>(str);
		if (ss.empty())  throw std::invalid_argument("No search string");
		std::string_view type;
		switch (stype)
		{
		case 0:
			type = "name";
			break;
		case 1:
			type = "generic_name";
			break;
		case 2:
			type = "barcode";
			break;
		default:
			throw std::invalid_argument("Invalid stype passed");
			break;
		}

		boost::trim(ss);
		boost::to_lower(ss);
		std::string search = fmt::format(R"(SELECT 
		 p.id
		 p.serial_num,
		 p.name,
		 p.generic_name,
		 p.class,
		 p.formulation,
		 p.strength,
		 p.strength_type,
		 p.usage_info,
		 p.sideeffects,
		 p.barcode,
		 p.manufactures_name
		 FROM products p, formulary_content fc
		 WHERE p.id = fc.product_id AND fc.formulary_id = ? AND p.{} LIKE '{}%' ORDER BY p.name;)", type, ss);

		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			std::move(search));
		query->m_arguments = { {
		boost::mysql::field(boost::mysql::blob(form.id.begin(), form.id.end()))
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
			co_return app->mNetManager.not_found(fmt::format("No products with \'{}\'", ss));
		}

		collection_type<grape::product> products;
		auto& v = boost::fusion::at_c<0>(products);
		v.reserve(data->size());

		for (auto& d : *data) {
			v.emplace_back(grape::serial::build<grape::product>(d.first));
		}
		co_return app->OkResult(products);
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}



void grape::ProductManager::RemoveFormulary() {
	auto app = grape::GetApp();
	try {
		auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
			R"(CREATE PROCEDURE IF NOT EXISTS remove_formulary (IN form_id binary(16))
			   BEGIN
				START TRANSACTION;
					DELETE FROM formulary WHERE id = form_id;
					DELETE FROM formulary_content WHERE formulary_id = form_id;
				COMMIT;
			  END;
		)");

		auto fut = query->get_future();
		app->mDatabase->push(query);
		(void)fut.get(); //block until complete
	}
	catch (const std::exception& exp) {
		spdlog::error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnMarkAsExpired(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::search) {
			co_return app->mNetManager.bad_request("Search method needed");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected a body");

		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		using pid_s = grape::collection_type<boost::fusion::vector<boost::uuids::uuid, std::uint64_t>>;
		auto&& [pids, buf2] = grape::serial::read<pid_s>(buf);
		auto& v = boost::fusion::at_c<0>(pids);
		if (v.empty()) throw std::invalid_argument("No products to mark");

		auto datetime = std::chrono::system_clock::now();
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(INSERT INTO expired VALUES ( ?, ?,?,?,? ) )");
		query->m_arguments.reserve(v.size());
		for (auto& p : v) {
			auto& pi = boost::fusion::at_c<0>(p);
			auto& s  = boost::fusion::at_c<1>(p);
			query->m_arguments.emplace_back(
				std::vector<boost::mysql::field>{
				boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
					boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
					boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
					boost::mysql::field(boost::mysql::blob(pi.begin(), pi.end())),
					boost::mysql::field(s)});
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
			//cancel the operation from the database
			co_return app->mNetManager.timeout_error();
		}
		(void)fut.get();
		
		co_return app->OkResult("Marked as expired");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}
boost::asio::awaitable<pof::base::net_manager::res_t> grape::ProductManager::OnGetExpiredProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match) {
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::get) {
			co_return app->mNetManager.bad_request("Search method needed");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected a body");

		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		auto&& [dq, buf2] = grape::serial::read<grape::date_query_t>(buf);
		auto& date = boost::fusion::at_c<1>(dq);
		std::string sql;
		if (date.has_value() && date.value().ok()) {
			sql = R"(SELECT * FROM expired WHERE pharmacy_id = ? AND branch_id = ? AND 
			MONTH(expired_date) = ? AND YEAR(expired_date) = ?;)";
		}
		else {
			throw std::invalid_argument("Expects a valid date to get expired products");
		}
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			std::move(sql));
		query->m_arguments = { {
			boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
			boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
			boost::mysql::field(static_cast<unsigned>(date.value().month())),
			boost::mysql::field(static_cast<int>(date.value().year()))
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
		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("No expired products"s);
		}
		grape::collection_type<grape::expired> expires;
		auto& v = boost::fusion::at_c<0>(expires);
		v.reserve(data->size());

		for (auto& d : *data) {
			v.emplace_back(grape::serial::build<grape::expired>(d.first));
		}
		co_return app->OkResult(expires);
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnMarkUpPharmaProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::put) {
			co_return app->mNetManager.bad_request("Put method needed");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected a body");

		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		//how to do this ?
		using mark_t = boost::fusion::vector<std::uint32_t>;
		using costupdate_t = boost::fusion::vector<pof::base::currency, pof::base::currency>;

		auto&& [m, buf2] = grape::serial::read<mark_t>(buf);
		std::uint32_t& percent = boost::fusion::at_c<0>(m);
		if (percent > 100) throw std::invalid_argument("Precent should be between 0 and  100");

		const float multiplier = 1.0f + (static_cast<float>(percent) / 100.0f);
		auto&& [opt, buf3] = grape::serial::read<grape::optional_list_t>(buf2);
		auto& v = boost::fusion::at_c<1>(opt);
		
		//a list of product to update
		//this list might be very large
		//may need to page it+
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase);
		query->m_hold_connection = true;
		if (v.has_value()) {
			query->m_sql = R"(SELECT product_id, costprice, unitprice FROM pharma_products
			WHERE pharmacy_id = ? AND branch_id = ? AND product_id = ?;)";
			query->m_arguments.reserve(v.value().size());
			for (auto& i : v.value()) {
				query->m_arguments.emplace_back(
					std::vector<boost::mysql::field>{
					boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
					boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
					boost::mysql::field(boost::mysql::blob(i.begin(), i.end()))});
			}
			v.value().clear();
		}
		else {
			query->m_sql = R"(SELECT product_id, costprice, unitprice FROM pharma_products
			WHERE pharmacy_id = ? AND branch_id = ?;)";
			query->m_arguments = { {
				boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
					boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end()))
			} };
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
			co_return app->mNetManager.timeout_error();
		}
		auto data = fut.get();
		query->m_arguments.clear();
		ec = co_await query->close();
		if (ec) throw std::system_error(ec);


		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("No products in store"s);
		}

		query->m_sql = R"(UPDATE pharma_products SET unitprice = ? 
			WHERE pharmacy_id = ? AND branch_id = ? AND product_id = ?; )";
		query->m_arguments.reserve(data->size());
		for (auto& d : *data) {
			auto& prodid    = boost::variant2::get<boost::uuids::uuid>(d.first[0]);
			auto& costprice = boost::variant2::get<pof::base::currency>(d.first[1]);
			auto& unitprice = boost::variant2::get<pof::base::currency>(d.first[2]);

			unitprice = static_cast<double>(costprice) * multiplier;
			query->m_arguments.emplace_back(
			std::vector<boost::mysql::field>{
			boost::mysql::field(boost::mysql::blob(unitprice.data().begin(), unitprice.data().end())),
			boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
			boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
			boost::mysql::field(boost::mysql::blob(prodid.begin(), prodid.end()))});
		}
		data->clear();

		query->m_waittime->expires_after(60s);
		fut = query->get_future();
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
		query->unborrow(); //return the connection we are holding
		(void) fut.get();
		
		co_return app->OkResult("Marked up products");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}


boost::asio::awaitable<pof::base::net_manager::res_t>
grape::ProductManager::OnTransferProductsToBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::put) {
			co_return app->mNetManager.bad_request("Put method needed");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected a body");

		//read credentials for the branch you are transfering from
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}

		//read the crednetials for the branch to transfer to. 
		auto&& [cred2, buf2] = grape::serial::read<grape::credentials>(buf);
		if (!app->mAccountManager.IsUser(cred2.account_id, cred2.pharm_id)) {
			co_return app->mNetManager.auth_error("branch is not authorised for transfer");
		}

		auto&& [pids, buf3] = grape::serial::read<pid_s>(buf2);
		auto& v = boost::fusion::at_c<0>(pids);
		if (v.empty()) co_return app->mNetManager.bad_request("No products to transfer");


		//take out these stock from the from branch, remove from list if there are 0 stock
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT product_id, stock_count FROM pharma_products 
			WHERE pharmacy_id = ? AND branch_id = ? AND product_id = ?;)");
		query->m_hold_connection = true;

		// add list to the pending table awaiting approval
		//need to put this in a pending stuff until approved by the pharmacy branch that accepts the transfer
		query->m_arguments.reserve(v.size());
		for (auto& i : v) {
			const auto& prodid = i.first;
			const auto& stock = i.second;
			query->m_arguments.emplace_back(
				std::vector<boost::mysql::field>{
				boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
				boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
				boost::mysql::field(boost::mysql::blob(prodid.begin(), prodid.end()))});
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
			co_return app->mNetManager.timeout_error();
		}

		auto data = fut.get();
		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("Cannot transfer any product listed");
		}

		//update the stocks
		query->m_arguments.clear();
		ec = co_await query->close();
		if (ec) throw std::system_error(ec);

		query->m_sql = R"(UPDATE pharma_products SET stock_count = ? 
		WHERE pharmacy_id = ? AND branch_id = ? AND product_id = ?)";

		for (auto& d : *data) {
			auto& prodid = boost::variant2::get<boost::uuids::uuid>(d.first[0]);
			auto& stock = boost::variant2::get<std::uint64_t>(d.first[1]);
			
			auto found = v.find(prodid);
			if (found == v.end()) continue;

			if (found->second >= stock) {
				v.erase(found->first);
				continue;
			}
			stock -= found->second; //update the stock for this branch
			query->m_arguments.emplace_back(
				std::vector<boost::mysql::field>{
				boost::mysql::field(stock),
				boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
				boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
				boost::mysql::field(boost::mysql::blob(prodid.begin(), prodid.end()))});
		}
		data->clear();

		query->m_waittime->expires_after(60s);
		fut = query->get_future();
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
		(void)fut.get();


		//save to pending table
		query->m_arguments.clear();
		ec = co_await query->close();
		if (ec) throw std::system_error(ec);



		query->m_sql = R"(INSERT INTO pending_transfers VALUES(?,?,?,?);)";
		grape::response::body_type::value_type listlblock(grape::serial::get_size(pids), 0x00);
		grape::serial::write(boost::asio::buffer(listlblock), pids);
		query->m_arguments = { {
				boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
				boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
				boost::mysql::field(boost::mysql::blob(cred2.branch_id.begin(), cred2.branch_id.end())),
				boost::mysql::field(boost::mysql::blob(listlblock.begin(), listlblock.end()))
		} };

		query->m_waittime->expires_after(60s);
		fut = query->get_future();
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
		(void)fut.get();

		query->unborrow();
		co_return app->OkResult("Transfer request pending");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t> 
grape::ProductManager::OnApproveBranchTransfers(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::put) {
			co_return app->mNetManager.bad_request("Put method needed");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected a body");

		//read credentials for the branch you are transfering from
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		//the transfer from branch
		auto&&[pendings, buf2] = grape::serial::read<grape::collection_type<pend_t>>(buf);
		
		//v an array for the approved transfers
		auto& v = boost::fusion::at_c<0>(pendings);
		if (v.empty()) throw std::invalid_argument("No pending list");
		
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(UPDATE pharma_products SET stock_count = stock_count + ? 
			WHERE pharmacy_id = ? AND branch_id = ? AND product_id = ?;)");
		query->m_hold_connection = true;
		query->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);
		
		//all the ids are approved
		auto pendel = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(DELETE FROM pending_transfers WHERE pend_id = ?;)");
		pendel->m_hold_connection = true;
		pendel->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);

		for (auto& approv : v) {
			auto& id     = boost::fusion::at_c<0>(approv);
			auto& branch = boost::fusion::at_c<1>(approv);
			auto& map    = boost::fusion::at_c<2>(approv);

			query->m_arguments.clear();
			query->m_arguments.reserve(map.size());

			for (auto& i : map) {
				query->m_arguments.emplace_back(
					std::vector<boost::mysql::field>{
						boost::mysql::field(i.second),
						boost::mysql::field(boost::mysql::blob(cred.pharm_id.begin(), cred.pharm_id.end())),
						boost::mysql::field(boost::mysql::blob(cred.branch_id.begin(), cred.branch_id.end())),
						boost::mysql::field(boost::mysql::blob(i.first.id.begin(), i.first.id.end()))
				});
			}

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

			pendel->m_arguments = { {
				boost::mysql::field(id)
			} };
			pendel->m_waittime->expires_after(60s);
			fut = std::move(pendel->get_future());
			tried = app->mDatabase->push(pendel);
			if (!tried) {
				tried = co_await app->mDatabase->retry(pendel); //try to push into the queue multiple times
				if (!tried) {
					co_return app->mNetManager.server_error("Error in query");
				}
			}
			auto&& [ec2] = co_await pendel->m_waittime->async_wait();
			if (ec2 != boost::asio::error::operation_aborted) {
				co_return app->mNetManager.timeout_error();
			}
			(void)fut.get();
		}

		boost::system::error_code ec = co_await query->close();
		if (ec) throw std::system_error(ec);
		query->unborrow();

		ec = co_await pendel->close();
		if (ec) throw std::system_error(ec);
		pendel->unborrow();

		
		co_return app->OkResult("Updated stock with transfer");
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}

boost::asio::awaitable<pof::base::net_manager::res_t>
grape::ProductManager::OnGetBranchPendTransfers(pof::base::net_manager::req_t&& req, boost::urls::matches&& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::put) {
			co_return app->mNetManager.bad_request("Put method needed");
		}
		auto& body = req.body();
		if (body.empty()) throw std::invalid_argument("Expected a body");

		//read credentials for the branch you are transfering from
		auto&& [cred, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(body));
		if (!(app->mAccountManager.VerifySession(cred.account_id, cred.session_id) &&
			app->mAccountManager.IsUser(cred.account_id, cred.pharm_id))) {
			co_return app->mNetManager.auth_error("Account not authorised");
		}
		auto query = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT * FROM pending_transfers 
			WHERE pharmacy_id = ? AND to_branch_id = ?;)");
		query->m_hold_connection = true;
		query->m_arguments = { {
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
		auto data = fut.get();

		query->m_arguments.clear();
		ec = co_await query->close();
		if (ec) throw std::system_error(ec);


		if (!data || data->empty()) {
			co_return app->mNetManager.not_found("No pending transfers"s);
		}
		//the transfer from branch
		grape::collection_type<pend_t> pendings;
		auto& v = boost::fusion::at_c<0>(pendings);
		v.reserve(data->size());


		query->m_sql = R"(SELECT  p.id
				p.serial_num,
				p.name,
				p.generic_name,
				p.class,
				p.formulation,
				p.strength,
				p.strength_type,
				p.usage_info,
				p.sideeffects,
				p.barcode,
				p.manufactures_name
				FROM products p
				WHERE p.id =  ?;)";

		auto bquery = std::make_shared<pof::base::datastmtquery>(app->mDatabase,
			R"(SELECT * FROM branches WHERE branch_id = ?;)");
		bquery->m_hold_connection = true;
		bquery->m_waittime = pof::base::dataquerybase::timer_t(co_await boost::asio::this_coro::executor);

		
		//for each pending list for this branch, get the products and the from branch
		for (auto& d : *data) {

			query->m_arguments.clear();
			bquery->m_arguments.clear();

			auto& frombranch = boost::variant2::get<pof::base::data::duuid_t>(d.first[2]);
			auto& pendlist = boost::variant2::get<pof::base::data::blob_t>(d.first[4]);

			auto&& [map, b] = grape::serial::read<pid_s>(boost::asio::buffer(pendlist));
			auto& m = boost::fusion::at_c<0>(map);
			if (m.empty()) continue;
			for (auto& i : m) {
				query->m_arguments.emplace_back(std::vector<boost::mysql::field>{
					boost::mysql::field(boost::mysql::blob(i.first.begin(), i.first.end()))
				});
			}
			query->m_waittime->expires_after(60s);
			fut = std::move(query->get_future());
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
			auto pd = fut.get();
			if (!pd || pd->empty()) continue;

			//get the branch that this pending is associated with, that is the from branch
			bquery->m_arguments = { {
				boost::mysql::field(boost::mysql::blob(frombranch.begin(), frombranch.end()))
			} };
			bquery->m_waittime->expires_after(60s);
			fut = std::move(bquery->get_future());
			tried = app->mDatabase->push(bquery);
			if (!tried) {
				tried = co_await app->mDatabase->retry(bquery); //try to push into the queue multiple times
				if (!tried) {
					co_return app->mNetManager.server_error("Error in query");
				}
			}
			std::tie(ec) = co_await bquery->m_waittime->async_wait();
			if (ec != boost::asio::error::operation_aborted) {
				co_return app->mNetManager.timeout_error();
			}

			auto br = fut.get();
			if (!br || br->empty()) continue;
			auto& brow = *(br->begin());


			pend_t p;
			auto& pendid   = boost::fusion::at_c<0>(p);
			auto& pbranch  = boost::fusion::at_c<1>(p);
			auto& pmap     = boost::fusion::at_c<2>(p);
			
			pendid         = boost::variant2::get<std::uint64_t>(d.first[0]);
			pbranch        = grape::serial::build<grape::branch>(brow.first);

			for (auto& prod : *pd) {
				auto pp = grape::serial::build<grape::product>(prod.first);
				auto f = m.find(pp.id);
				if (f == m.end()) continue;

				pmap.try_emplace(pp, f->second);
			}
			v.emplace_back(std::move(p));
		}

		ec = co_await query->close();
		if (ec) throw std::system_error(ec);
		query->unborrow();

		ec = co_await bquery->close();
		if (ec) throw std::system_error(ec);
		bquery->unborrow();


		co_return app->OkResult(pendings);
	}
	catch (const std::exception& exp) {
		co_return app->mNetManager.server_error(exp.what());
	}
}


