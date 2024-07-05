#pragma once
#include "netmanager.h"
#include "databasemysql.h"
#include <boost/lexical_cast.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>

namespace grape {
	class ProductManager : public boost::noncopyable {
	public:
		enum : std::uint8_t {
			FORMULARY_ID,
			FORMULARY_NAME,
			FORMULARY_CREATED_BY_NAME,
			FORMULARY_CREATED_DATE,
			FORMULARY_ACCESS_LEVEL
		};

		//formulary access leve
		enum : std::uint8_t {
			ACCESS_PRIVATE =  0x01,
			ACCESS_PUBLIC  =  0x02,
		};

		enum : std::uint8_t {
			FORUMARY_ID,
			PRODUCT_ID
		};

		enum : std::uint8_t {
			PRODUCT_PHARMACY_UUID,
			PRODUCT_BRANCH_UUID,
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
			PRODUCT_UNIT_PRICE,
			PRODUCT_COST_PRICE,
			PRODUCT_PACKAGE_SIZE,
			PRODUCT_STOCK_COUNT, // TRACKS THE PRESENT STOCK OF THE PRODUCT
			PRODUCT_SIDEEFFECTS,
			PRODUCT_BARCODE,
			PRODUCT_CATEGORY,
			PRODUCT_MIN_STOCK_COUNT,
			PRODUCT_FORMULARY_ID,

			//Product setting
			//PRODUCT_EXPIRE_PERIOD,
			//PRODUCT_TO_EXPIRE_DATE, //NUMBER OF PERIOD TO WAIT TO BE INFORMED
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
			PHARMACY_ID,
			BRANCH_ID,
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

	};
};