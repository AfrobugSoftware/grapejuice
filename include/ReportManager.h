#pragma once
#include "netmanager.h"
#include "databasemysql.h"
#include <../base/bcrypt/include/bcrypt.h>

#include <boost/lexical_cast.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/chrono.hpp>
#include <boost/noncopyable.hpp>
#include <boost/fusion/include/define_struct.hpp>
#include "serialiser.h"

BOOST_FUSION_DEFINE_STRUCT(
	(grape), reports,
	(boost::uuids::uuid, prod_id)
	(boost::uuids::uuid, sale_id)
	(std::chrono::system_clock::time_point, date)
	(std::string, name)
	(std::int64_t, quantity)
	(pof::base::currency, price)
	(pof::base::currency, cost)
	(std::string, payment_type)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape), inev_report,
	(boost::uuids::uuid, prod_id)
	(boost::uuids::uuid, inven_id)
	(std::string, name)
	(std::chrono::system_clock::time_point, input_date)
	(std::int64_t, stock_count)
	(pof::base::currency, cost)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape), stt,
	(std::uint32_t, dt)
	(std::chrono::year_month_day, date)
)

BOOST_FUSION_DEFINE_STRUCT(
	(grape), dashboard,
	(std::int32_t, product_count)
	(std::int32_t, sales_count)
	(std::int32_t, out_of_stock)
	(std::int32_t, expired_stock)
	(pof::base::currency, total_revenue)
	(pof::base::currency, total_purchase)
	(pof::base::currency, total_stock_amount)
)



namespace grape
{

	class ReportManager : public boost::noncopyable
	{
	public:
		static constexpr const std::array<std::string_view, 3> ymds = {
			"YEAR",
			"MONTH",
			"DAY"
		};
		ReportManager();
		~ReportManager() = default;

		void SetRoutes();

		constexpr bool checkDate(const std::chrono::year_month_day& ymd) { return (ymd.ok() && ymd.year() >= std::chrono::year{ 1970 }); }
		boost::asio::awaitable<grape::response> OnGetProfitLoss(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetEndOf(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetEndByRange(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetInventoryPurchased(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetMonthly(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetDashboardRecords(grape::request&& req, boost::urls::matches&& match);


	};
};