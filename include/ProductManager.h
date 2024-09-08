#pragma once
#include "netmanager.h"
#include "databasemysql.h"
#include <boost/lexical_cast.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include "serialiser.h"

#include "zip.h"
#include <algorithm>
//products
namespace grape {
	//formulary access level
	enum class formulary_access_level : std::uint32_t {
		ACCESS_PRIVATE = 0x01,
		ACCESS_PUBLIC = 0x02,
	};

	//order state
	enum class order_state : std::uint32_t {
		PENDING,
		ORDERED,
		COMPLETED,
		DELIVERED,
	};

	//warning level
	enum class warning_level : std::uint32_t {
		SIMPLE,
		CRITICAL,
	};

	//actions
	enum class action :std::uint32_t
	{
		STOCK_CHECKED,
		BROUGHT_FORWARD,
		CHECK_TIME,
		DATA_BACKUP,
	};
};

BOOST_FUSION_DEFINE_STRUCT(
	(grape), product, 
	(boost::uuids::uuid, id)
	(std::uint64_t, serial_num)
	(std::string, name)
	(std::string, generic_name)
	(std::string, class_)
	(std::string, formulation)
	(std::string, strength)
	(std::string, strength_type)
	(std::string, usage_info)
	(std::string, description)
	(std::string, indications)
	(std::uint64_t, package_size)
	(std::string, sideeffects)
	(std::string, barcode)
	(std::string, manufactures_name)
)

template<size_t N>
using opt_field_string = grape::optional_field<std::string, N>;

template<size_t N>
using opt_field_uint64_t = grape::optional_field<std::uint64_t, N>;

template<size_t N>
using opt_field_time_point = grape::optional_field<std::chrono::system_clock::time_point, N>;

template<size_t N>
using opt_field_currency = grape::optional_field<pof::base::currency, N>;

template<size_t N>
using opt_field_uuid = grape::optional_field<boost::uuids::uuid, N>;

BOOST_FUSION_DEFINE_STRUCT(
	(grape), product_opt,
	(boost::uuids::uuid, id)
	(std::uint64_t, serial_num)
	(grape::opt_fields, fields)
	(opt_field_string<0>, name)
	(opt_field_string<1>, generic_name)
	(opt_field_string<2>, class_)
	(opt_field_string<3>, formulation)
	(opt_field_string<4>, strength)
	(opt_field_string<5>, strength_type)
	(opt_field_string<6>, usage_info)
	(opt_field_string<7>, description)
	(opt_field_string<8>, indications)
	(opt_field_uint64_t<9>, package_size)
	(opt_field_string<10>, sideeffects)
	(opt_field_string<11>, barcode)
	(opt_field_string<12>, manufactures_name)
)

//inventory
BOOST_FUSION_DEFINE_STRUCT(
	(grape), inventory,
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, id)
	(boost::uuids::uuid, product_id)
	(std::chrono::system_clock::time_point, expire_date)
	(std::chrono::system_clock::time_point, input_date)
	(std::int64_t, stock_count)
	(pof::base::currency, cost)
	(boost::uuids::uuid, supplier_id)
	(std::string, lot_number)
)

//packs
BOOST_FUSION_DEFINE_STRUCT(
	(grape), pack,
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, id)
	(boost::uuids::uuid, product_id)

)

//pharma products
BOOST_FUSION_DEFINE_STRUCT(
	(grape), pharma_product,
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, product_id)
	(pof::base::currency, unitprice)
	(pof::base::currency, costprice)
	(std::uint64_t, stock_count)
	(std::uint64_t, min_stock_count)
	(std::chrono::system_clock::time_point, date_added)
	(std::chrono::system_clock::time_point, date_expired)
	(std::uint64_t, category_id)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape), pharma_product_opt,
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, product_id)
	(grape::opt_fields, fields)
	(opt_field_currency<0>, unitprice)
	(opt_field_currency<1>, costprice)
	(opt_field_uint64_t<2>, stock_count)
	(opt_field_uint64_t<3>, min_stock_count)
	(opt_field_time_point<4>, date_added)
	(opt_field_time_point<5>, date_expired)
	(opt_field_uint64_t<6>, category_id)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape), product_identifier,
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, product_id)
)

//collections
BOOST_FUSION_DEFINE_STRUCT(
	(grape)(collection), products,
	(std::vector<grape::product>, group)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape)(collection), pharma_products,
	(std::vector<grape::pharma_product>, group)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape), formulary,
	(boost::uuids::uuid, id)
	(boost::uuids::uuid, creator_id)
	(std::string, name)
	(std::string, created_by)
	(std::chrono::system_clock::time_point, created_date)
	(std::string, version)
	(std::uint64_t, usage_count)
	(grape::formulary_access_level, access_level)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape), invoice, 
	(boost::uuids::uuid, pharm_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, supplier_id)
	(boost::uuids::uuid, id)
	(boost::uuids::uuid, product_id)
	(boost::uuids::uuid, inventory_id)
	(std::chrono::system_clock::time_point, input_date)

)

BOOST_FUSION_DEFINE_STRUCT(
	(grape), supplier, 
	(boost::uuids::uuid, pharm_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, id)
	(std::string, name)
	(std::chrono::system_clock::time_point, date_created)
	(std::chrono::system_clock::time_point, date_modified)
	(std::string, info)
)


BOOST_FUSION_DEFINE_STRUCT(
	(grape), order,
	(grape::order_state, state)
	(boost::uuids::uuid, id)
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, product_id)
	(pof::base::currency, total_cost)
	(std::uint64_t, quantity)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape), warnings,
	(grape::warning_level, state)
	(boost::uuids::uuid, id)
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, product_id)
	(std::string, warning_text)
)


BOOST_FUSION_DEFINE_STRUCT(
	(grape), category,
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, branch_id)
	(std::uint64_t, category_id)
	(std::string, name)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape), expired,
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, product_id)
	(std::chrono::system_clock::time_point, expired_date)
	(std::uint64_t, stock_count)
)

namespace grape {
	using pid_s = boost::fusion::vector<boost::unordered_flat_map<boost::uuids::uuid, std::uint64_t>>;
	using pend_t = boost::fusion::vector<std::uint64_t, grape::branch, boost::unordered_flat_map<grape::product, std::uint64_t>>;



	//allow product to be hashable
	extern bool operator==(const product& a, const product& b);
	extern std::size_t hash_value(product const& b);

	class ProductManager : public boost::noncopyable {
	public:
		ProductManager();
		~ProductManager();

		//mysql tables
		void CreateTables();
		void CreateProductTable();
		void CreateInventoryTable();
		void CreatePackTable();
		void CreateSupplierTable();
		void CreateCategoryTable();
		void CreateInvoiceTable();
		void CreateExpiredTable();
		void CreatePharmacyProductTable();
		void CreateFormularyTable();
		void CreateFormularyOverrideTable();
		void CreateOrderTable();
		void CreateWarningTable();
		void CreateBranchTransferPendingTable();

		//utilities
		std::pair<boost::uuids::uuid, boost::uuids::uuid> SplitPidBid(boost::core::string_view str);

		//routes
		void SetRoutes();
		
		//product routes
		boost::asio::awaitable<pof::base::net_manager::res_t> OnAddProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnUpdateProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnRemoveProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnAddPharmacyProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetPharmacyProductCount(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnUpdatePharmaProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnMarkUpPharmaProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnSearchProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		


		//formulary routes
		boost::asio::awaitable<pof::base::net_manager::res_t> OnCreateFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnRemoveFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetProductsByFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetFormularyProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnAddFormularyProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnRemoveFormularyProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnSearchFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnLoadFormulary(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnCheckFormularyName(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetAttachedFormulary(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnCheckHasFormulary(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnImportFormulary(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetFormularyForProduct(grape::request&& req, boost::urls::matches&& match);
	


		//category
		boost::asio::awaitable<pof::base::net_manager::res_t> OnAddCategory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnRemoveCategory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnUpdateCategory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetCategory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

		//inventory
		boost::asio::awaitable<pof::base::net_manager::res_t> OnAddInventory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnRemoveInventory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnUpdateInventory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetInventory(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetInventoryCount(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

		//expired products
		boost::asio::awaitable<pof::base::net_manager::res_t> OnMarkAsExpired(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetExpiredProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

		//invoices and suppliers
		boost::asio::awaitable<grape::response> OnCreateInvoice(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnRemoveInvoice(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetInvoices(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetInvoicesByDate(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetProductsInInvoice(grape::request&& req, boost::urls::matches&& match);

		boost::asio::awaitable<grape::response> OnCreateSupplier(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnRemoveSupplier(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetSupplier(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetSupplierByDate(grape::request&& req, boost::urls::matches&& match);
		


		//inter branch product management
		boost::asio::awaitable<pof::base::net_manager::res_t> OnTransferProductsToBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetBranchPendTransfers(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnApproveBranchTransfers(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnRejectBranchTransfers(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);



		// mysql procedures
		void Procedures();
		void RemovePharamProducts();
		void RemoveCategory();
		void RemoveFormulary();

	};
};