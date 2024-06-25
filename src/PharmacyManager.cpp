#include "PharmacyManager.h"
#include "Application.h"

grape::PharmacyManager::PharmacyManager()
{
}

grape::PharmacyManager::~PharmacyManager()
{
}

void grape::PharmacyManager::CreatePharmacyTable()
{
	auto app = grape::GetApp();
	//pharmacy info is a json sting
	auto query = std::make_shared<pof::base::dataquerybase>(app->mDatabase,
		R"(
			CREATE TABLE IF NOT EXISTS pharmacy (
			pharmacy_id blob,
			pharmacy_name text,
			pharamcy_address integer,
			pharmacy_info text
		);)");
	auto fut = query->get_future();
	app->mDatabase->push(query);
	try {
		auto data = fut.get(); //block
	}
	catch (boost::mysql::error_with_diagnostics& err) {
		spdlog::error(err.what());
	}
}

void grape::PharmacyManager::SetRoutes()
{
	auto app = grape::GetApp();
	app->route("/pharmacy/create", std::bind_front(&grape::PharmacyManager::OnCreatePharmacy, this));

}

pof::base::net_manager::res_t grape::PharmacyManager::OnCreatePharmacy(pof::base::net_manager::req_t& req, boost::urls::matches& match)
{
	auto app = grape::GetApp();
	try {
		if (req.method() != http::verb::post) {
			return app->mNetManager.bad_request("Method should be post method"s);
		}
		if (!req.has_content_length()) {
			return app->mNetManager.bad_request("Expected a body");
		}
		size_t len = boost::lexical_cast<size_t>(req.at(boost::beast::http::field::content_length));
		auto& req_body = req.body();
		std::string data;
		data.resize(len);

		auto buffer = req_body.data();
		boost::asio::buffer_copy(boost::asio::buffer(data), buffer);
		req_body.consume(len);

		js::json jsonData = js::json::parse(data);
		/*
		* {
			"name" : "pharmacy",
			"address" : "address string"
			
		}
		*/

	}
	catch (std::exception& exp) {
		return app->mNetManager.server_error(exp.what());
	}
}

pof::base::net_manager::res_t grape::PharmacyManager::OnDestroyPharmacy(pof::base::net_manager::req_t& req, boost::urls::matches& match)
{
	return pof::base::net_manager::res_t();
}
