#pragma once
#include "netmanager.h"
#include "databasemysql.h"
#include <boost/lexical_cast.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/noncopyable.hpp>

#include <algorithm>
#include <boost/fusion/include/define_struct.hpp>


//institution
BOOST_FUSION_DEFINE_STRUCT(
	(grape), institution,
	(boost::uuids::uuid, id)
	(std::string, name)
	(std::uint8_t, type)
	(boost::uuids::uuid, address_id)
	(std::string, info)
)

//pharmacy
BOOST_FUSION_DEFINE_STRUCT(
	(grape), pharmacy,
	(boost::uuids::uuid, id)
	(std::string, name)
	(boost::uuids::uuid, address_id)
	(std::string, info)
)

//branches
BOOST_FUSION_DEFINE_STRUCT(
	(grape), branch,
	(boost::uuids::uuid, id)
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, address_id)
	(std::string, name)
	(std::uint32_t, state)
	(std::string, info)
)

//branch collection
BOOST_FUSION_DEFINE_STRUCT(
	(grape)(collection),
	branches,
	(std::vector<grape::branch>, group)
)

//pharmacy collection
BOOST_FUSION_DEFINE_STRUCT(
	(grape)(collection),
	pharmacies,
	(std::vector<grape::pharmacy>, group)
)

namespace grape {
	class PharmacyManager  : public boost::noncopyable{
	public:
		PharmacyManager();
		~PharmacyManager();

		void CreateTables();
		void CreatePharmacyTable();
		void CreateInstitution();
		void CreateBranchTable();
		void CreateAddressTable();
		void SetRoutes();
		//branch state
		enum : std::uint8_t {
			NONE_STATE = 0x00,
			OPEN = 0x01,
			CLOSED = 0x02,
		};

		enum : std::uint8_t {
			NONE = 0x00,
			HOSPITAL = 0x02,
			INSURANCE = 0x04,
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



		//institutions
		boost::asio::awaitable<pof::base::net_manager::res_t>
			OnCreateInstitutions(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

	private:
		boost::asio::awaitable<bool> CheckIfPharmacyExists(const std::string& name);
		boost::asio::awaitable<bool> CheckIfInstitutionExists(const std::string& name);
		boost::asio::awaitable<bool> CheckIfBranchExists(const std::string& bn, const boost::uuids::uuid& pid);
		//thread safe, active pharmacy cache?
		boost::concurrent_flat_map<boost::uuids::uuid, grape::branch> mActivePharamcyBranches;
		boost::concurrent_flat_map<boost::uuids::uuid, grape::institution> mActiveInstitutions;
	};
};