#pragma once
#include "netmanager.h"
#include "databasemysql.h"
#include <boost/lexical_cast.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/noncopyable.hpp>

#include <algorithm>

namespace grape {
	class PharmacyManager  : public boost::noncopyable{
	public:
		PharmacyManager();
		~PharmacyManager();


		void CreatePharmacyTable();
		void CreateBranchTable();
		void CreateAddressTable();
		void SetRoutes();
		//branch state
		enum : std::uint32_t {
			NONE_STATE,
			OPEN,
			CLOSED,
		};

		//pharmacy
		enum {
			PHARMACY_ID,
			PHARMACY_NAME,
			PHARMACY_ADDRESS_ID,
			PHARMACY_INFO,
		};

		//pharmacy branches
		enum {
			BRANCH_ID,
			BRANCH_PHARMACY_ID,
			BRANCH_ADDRESS_ID,
			BRANCH_NAME,
			BRANCH_STATE,
			BRANCH_INFO,
		};

		//routes handles
		boost::asio::awaitable<pof::base::net_manager::res_t>
			OnCreatePharmacy(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t>
				OnPharmacyInfoUpdate(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t>
				OnOpenPharmacyBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t>
				OnSetBranchState(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		//this is gonna be a very slow process
		//removes all traces of the pharamcy in grape juice
		boost::asio::awaitable<pof::base::net_manager::res_t>
				OnDestroyPharmacy(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

		//queries
		boost::asio::awaitable<pof::base::net_manager::res_t>
				OnGetPharmacyBranches(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

	private:
		boost::asio::awaitable<bool> CheckIfPharmacyExists(const std::string& name);
		boost::asio::awaitable<bool> CheckIfBranchExists(const std::string& bn, const boost::uuids::uuid& pid);
		//thread safe, active pharmacy cache?
		boost::concurrent_flat_map<boost::uuids::uuid, 
			pof::base::data::row_t> mActivePharamcyBranches;
	};
};