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
	(grape), branches,
	(boost::uuids::uuid, pharmacy_id)
	(boost::uuids::uuid, address_id)
	(std::string, name)
	(std::uint8_t, state)
	(std::string, info)
)

//address
BOOST_FUSION_DEFINE_STRUCT(
	(grape), address,
	(boost::uuids::uuid, id)
	(std::string, country)
	(std::string, state)
	(std::string, lga)
	(std::string, street)
	(std::string, num)
	(std::string, add_info)
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

		//institution
		enum {
			INSTITUTION_ID,
			INSTITUTION_NAME,
			INSTITUTION_TYPE,
			INSTITUTION_ADDRESS_ID,
			INSTITUTION_INFO,
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

		//address
		enum {
			ADDRESS_ID,
			COUNTRY,
			STATE,
			LGA,
			STREET,
			NUM,
			ADD_INFO, //Contains a json with polar coordinates, probably from goolge map
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
		boost::concurrent_flat_map<boost::uuids::uuid, 
			pof::base::data::row_t> mActivePharamcyBranches;
		boost::concurrent_flat_map<boost::uuids::uuid,
			pof::base::data::row_t> mActiveInstitutions;
	};
};