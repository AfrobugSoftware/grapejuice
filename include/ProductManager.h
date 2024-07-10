#pragma once
#include "netmanager.h"
#include "databasemysql.h"
#include <boost/lexical_cast.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include "serialiser.h"

#include <algorithm>
//products
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
	(std::uint64_t, stock_count)
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



//collections
BOOST_FUSION_DEFINE_STRUCT(
	(grape)(collection), products,
	(std::vector<grape::product>, group)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape)(collection), pharma_products,
	(std::vector<grape::pharma_product>, group)
)


namespace grape {
	class ProductManager : public boost::noncopyable {
	public:
		enum : std::uint8_t {
			FORMULARY_ID,
			FORMULARY_CREATOR_ID,
			FORMULARY_NAME,
			FORMULARY_CREATED_BY_NAME,
			FORMULARY_CREATED_DATE,
			FORMULARY_VERSION,
			FORMULARY_ACCESS_LEVEL
		};

		//formulary access level
		enum : std::uint8_t {
			ACCESS_PRIVATE =  0x01,
			ACCESS_PUBLIC  =  0x02,
		};

		//formulary content
		enum : std::uint8_t {
			FORUMARY_ID,
			PRODUCT_ID
		};

		//pharmacy products
		enum : std::uint8_t {
			PHARMACY_ID,
			BRANCH_ID,
			PHARMA_PRODUCT_ID,
			PHARMA_PRODUCT_UNIT_PRICE,
			PHARMA_PRODUCT_COST_PRICE,
			PHARMA_PRODUCT_STOCK_COUNT, // TRACKS THE PRESENT STOCK OF THE PRODUCT
			PHARMA_PRODUCT_MIN_STOCK_COUNT,
			PHARMA_PRODUCT_DATE_ADDED,
			PHARMA_PRODUCT_DATE_EXPIRE,
			PHARMA_PRODUCT_CATEGORY,
		};

		//products table
		enum : std::uint8_t {
			PRODUCT_UUID,
			PRODUCT_SERIAL_NUM,
			PRODUCT_NAME,
			PRODUCT_GENERIC_NAME, //FOR PHARMACEUTICES WITH GENERIC NAME, MIGHT CONTAIN MORE THAN ONE GENERIC NAME FOR EXAMPLE ATHERTHER/LUMEFANTRINE 
			PRODUCT_CLASS,
			PRODUCT_FORMULATION,
			PRODUCT_STRENGTH, // GIVEN IN mg, g %v/v, %m/v -> need to have a list of approved stengths,
			PRODUCT_STRENGTH_TYPE, // GIVEN as mg, g %v/v, %m/v -> need to have a list of approved stengths,
			PRODUCT_USAGE_INFO,
			PRODUCT_DESCRIP,
			PRODUCT_HEALTH_CONDITIONS, //COMMA SEPERATED
			PRODUCT_PACKAGE_SIZE,
			PRODUCT_SIDEEFFECTS,
			PRODUCT_BARCODE,
			PRODUCT_MANUFACTURES_NAME,
			PRODUCT_MAX
		};

		enum : std::uint8_t {
			INVENTORY_PHARMACY_UUID,
			INVENTORY_BRANCH_UUID,
			INVENTORY_ID,
			INVENTORY_PRODUCT_UUID, //same UUID AS THE PRODUCT
			INVENTORY_EXPIRE_DATE, // MMYY FORMAT 
			INVENTORY_INPUT_DATE, // DATE ADDED 
			INVENTORY_STOCK_COUNT, //THE STOCK ASSOCIATED WITH THIS INVENTORY
			INVENTORY_COST, //INVENTORY COST PRICE
			INVENTORY_SUPPLIER_ID,
			INVENTORY_LOT_NUMBER, //HAVE MULTIPLE BACTHES -> 
			INVENTORY_MAX
		};

		enum : std::uint8_t {
			CATEGORY_PHARMACY_UUID,
			CATEGORY_BRANCH_UUID,
			CATEGORY_ID,
			CATEGORY_NAME,
			CATEGORY_MAX
		};

		enum : std::uint8_t {
			ORDER_BRANCH_UUID,
			ORDER_PRODUCT_UUID,
			ORDER_PRODUCT_NAME,
			ORDER_DATE,
			ORDER_QUANTITY,
			ORDER_COST,
			ORDER_STATE,
			ORDER_MAX
		};

		enum : std::uint8_t {
			PACK_PHARAMCY_UUID,
			PACK_BRANCH_UUID,
			PACK_UUID,
			PACK_PROD_UUID,
			PACK_PROD_QUANTITY,
			PACK_PROD_EXT_COST,
			PACK_PROD_MAX
		};

		//suppliers ?? what are there in the context of grape juice??
		enum : std::uint8_t {
			SUPPLIER_ID,
			SUPPLIER_NAME,
			SUPPLIER_DATE_CREATED,
			SUPPLIER_DATE_MODIFIED,
			SUPPLIER_INFO,
			SUPPLIER_MAX
		};

		enum :std::uint8_t {
			INVOICE_PHARMACY_ID,
			INVOICE_BRANCH_ID,
			INVOICE_SUPP_ID,
			INVOICE_ID,
			INVOICE_PROD_UUID,
			INVOICE_INVENTORY_ID,
			INVOICE_MAX,
		};

		enum : std::uint8_t {
			EXPIRED_PHARMACY_ID,
			EXPIRED_BRANCH_ID,
			EXPIRED_PRODUCT_ID,
			EXPIRED_QUANTITY,
			EXPIRED_DATE,
		};

		//order state
		enum : std::uint8_t {
			PENDING,
			ORDERED,
			COMPLETED,
			DELIVERED,
		};


		//warning level
		enum : std::uint8_t {
			SIMPLE,
			CRITICAL,
		};

		//actions
		enum :std::uint8_t
		{
			STOCK_CHECKED,
			BROUGHT_FORWARD,
			CHECK_TIME,
			DATA_BACKUP,
		};

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

		//utilities
		std::pair<boost::uuids::uuid, boost::uuids::uuid> SplitPidBid(boost::core::string_view str);

		//routes
		void SetRoutes();
		boost::asio::awaitable<pof::base::net_manager::res_t>
			OnAddProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t>
			OnUpdateProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t>
			OnGetProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t>
			OnRemoveProducts(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t>
			OnGetFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t>
			OnGetProductsByFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t>
			OnAddPharmacyProduct(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t>
			OnCreateFormulary(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);


		// mysql procedures
		void Procedures();
		void RemovePharamProducts();

	};
};