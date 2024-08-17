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

#include <algorithm>
namespace grape {
	enum class sale_state : std::uint32_t {
		returned,
		complete,
		pending,
		reversed,
		deleted,
	};

};


BOOST_FUSION_DEFINE_STRUCT(
	(grape), sale,
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, branch_id)
	(boost::uuids::uuid, id)
	(boost::uuids::uuid, product_id)
	(std::chrono::system_clock::time_point, sale_date)
	(pof::base::currency, unit_cost)
	(pof::base::currency, unit_price)
	(pof::base::currency, total)
	(std::uint32_t, quantity)
	(std::string, payment_method)
	(std::string, product_label)
	(grape::sale_state, state)
	(std::string, add_info)
)

namespace grape {
	class SaleManager : public boost::noncopyable {
	public:
		SaleManager() = default;
		~SaleManager() = default;

		void CreateTables();
		void SetRoutes();

		//table functions
		void CreateSaleTable();

	};

};