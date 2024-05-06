#include <iostream>
#include <date/date.h>
#include <spdlog/spdlog.h>
#include <format>
#include <filesystem>
#include <fstream>
#include <boost/asio/streambuf.hpp>

#include "Application.h"
#include "packages.h"

namespace fs = std::filesystem;
int main(int argc, char** argv)
{
    std::cout << "grapejuice 1.0.0" << std::endl;
 
    auto app = grape::GetApp();
    app->Init();

    app->route("/about", [](http::request<http::dynamic_body>& req, 
        boost::urls::matches& m) -> http::response<http::dynamic_body> {
            http::response<http::dynamic_body> res{ http::status::ok, 11 };

           res.set(http::field::server, USER_AGENT_STRING);
           res.set(http::field::content_type, "text/html");
           res.keep_alive(req.keep_alive());


           http::dynamic_body::value_type value;
           const auto data = "This is grape juice"s;
           auto buffer = value.prepare(data.size());
           boost::asio::buffer_copy(buffer, boost::asio::buffer(data));
           value.commit(data.size());

           res.body() = value;
           res.prepare_payload();
           return res;
    });

    app->route("/about/{path}/{second}", [&](http::request<http::dynamic_body>& req, boost::urls::matches& m)
        ->http::response<http::dynamic_body> {
            auto p = m["path"];
            auto a = m["second"];
            http::response<http::dynamic_body> res{ http::status::ok, 11 };

            res.set(http::field::server, USER_AGENT_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());

            http::dynamic_body::value_type value;
            const auto data = "This is grape juice: "s + std::string(p) + " " + std::string();
            auto buffer = value.prepare(data.size());
            boost::asio::buffer_copy(buffer, boost::asio::buffer(data));
            value.commit(data.size());

            res.body() = value;
            res.prepare_payload();
            return res;
     });

    app->Run();
    int a;
    std::cin >> a;
    app->Exit();
    return 0;
}