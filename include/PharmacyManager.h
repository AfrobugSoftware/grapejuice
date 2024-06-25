#pragma once
#include "netmanager.h"
#include "databasemysql.h"
#include <boost/lexical_cast.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/algorithm/string.hpp>

namespace grape {
	class PharmacyManager {
	public:
		PharmacyManager();
		~PharmacyManager();


		void CreatePharmacyTable();
		void CreateBranchTable();
		void SetRoutes();

		//routes handles
		pof::base::net_manager::res_t OnCreatePharmacy(pof::base::net_manager::req_t& req,
			boost::urls::matches& match);


		//this is gonna be a very slow process
		//removes all traces of the pharamcy in grape juice
		pof::base::net_manager::res_t OnDestroyPharmacy(pof::base::net_manager::req_t& req,
			boost::urls::matches& match);

	private:

		//thread safe, active pharmacy cache?
		boost::concurrent_flat_map<boost::uuids::uuid,
			pof::base::data::row_t> mActivePharamcy;
	};
};