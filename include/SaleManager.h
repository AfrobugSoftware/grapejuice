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