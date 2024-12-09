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
	(grape), stt,
	(std::uint32_t, dt)
	(std::chrono::year_month_day, date)
)


namespace grape
{

	class ReportManager : public boost::noncopyable
	{
	public:
		static constexpr const std::array<std::string_view, 3> ymds = {
			"YEAR(s.sale_date)",
			"MONTH(s.sale_date)",
			"DAYOFMONTH(s.sale_date)"
		};
		ReportManager();
		~ReportManager() = default;

		void SetRoute();
		boost::asio::awaitable<grape::response> OnGetProfitLoss(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetEndOf(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetEndByRange(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetInventoryPurchased(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetMonthly(grape::request&& req, boost::urls::matches&& match);

	};
};