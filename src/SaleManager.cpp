#include "SaleManager.h"
#include "Application.h"


void grape::SaleManager::CreateTables()
{
	CreateSaleTable();
}

void grape::SaleManager::SetRoutes()
{
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
