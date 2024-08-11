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




//pharmacy
BOOST_FUSION_DEFINE_STRUCT(
	(grape), pharmacy,
	(boost::uuids::uuid, id)
	(std::string, name)
	(boost::uuids::uuid, address_id)
	(std::string, info)
)

namespace grape {
	enum class branch_type : std::uint32_t {
		community,
		hospital,
		industry,
		drf,
		educational
	};

	enum class branch_state : std::uint32_t {
		open,
		closed,
		shutdown,
	};

	enum class institution_type : std::uint32_t {
		hospital,
		insurance,
		none
	};
};

//institution
BOOST_FUSION_DEFINE_STRUCT(
	(grape), institution,
	(boost::uuids::uuid, id)
	(std::string, name)
	(grape::institution_type, type)
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
	(grape::branch_type, type)
	(grape::branch_state, state)
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
			NONE = 0x00,
			HOSPITAL = 0x02,
			INSURANCE = 0x04,
		};

		//pharmacies
		boost::asio::awaitable<pof::base::net_manager::res_t> OnCreatePharmacy(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnPharmacyInfoUpdate(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetPharmacies(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnSearchPharmacies(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnDestroyPharmacy(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetPharmacyAddress(grape::request&& req, boost::urls::matches&& match);
		boost::asio::awaitable<grape::response> OnGetPharmacyById(grape::request&& req, boost::urls::matches&& match);


		//branches
		boost::asio::awaitable<pof::base::net_manager::res_t> OnCreateBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetBranches(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetBranchesById(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnSetBranchState(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnOpenPharmacyBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnClosePharmacyBranch(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		boost::asio::awaitable<pof::base::net_manager::res_t> OnGetBranchAddress(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);
		


		//this is gonna be a very slow process
		//removes all traces of the pharamcy in grape juice

		//institutions
		boost::asio::awaitable<pof::base::net_manager::res_t> OnCreateInstitutions(pof::base::net_manager::req_t&& req, boost::urls::matches&& match);

	private:
		boost::asio::awaitable<bool> CheckIfPharmacyExists(const std::string& name);
		boost::asio::awaitable<bool> CheckIfInstitutionExists(const std::string& name);
		boost::asio::awaitable<bool> CheckIfBranchExists(const std::string& bn, const boost::uuids::uuid& pid);
		//thread safe, active pharmacy cache?
		boost::concurrent_flat_map<boost::uuids::uuid, grape::branch> mActivePharamcyBranches;
		boost::concurrent_flat_map<boost::uuids::uuid, grape::institution> mActiveInstitutions;
	};
};