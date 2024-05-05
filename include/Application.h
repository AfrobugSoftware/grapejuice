#pragma once
#include <boost/noncopyable.hpp>
#include <memory>


#include "netmanager.h"
#include "databasemysql.h"
namespace grape {
	class Application : public boost::noncopyable, 
		public std::enable_shared_from_this<Application>
	{
	public:
		Application(const std::string& servername);
		virtual ~Application();

		virtual bool Init();
		virtual bool Run();
		virtual bool Exit();

		void route(const std::string& target,
			pof::base::net_manager::callback&& endpoint);
		std::shared_ptr<pof::base::databasemysql> mDatabase;

		std::uint16_t mPort = 8080;
		pof::base::net_manager mNetManager;
	};

	extern std::shared_ptr<Application> GetApp();
};
