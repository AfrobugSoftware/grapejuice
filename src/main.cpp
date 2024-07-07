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

    app->route("/", [](http::request<http::dynamic_body>&& req, 
        boost::urls::matches&& m) -> boost::asio::awaitable<http::response<http::dynamic_body>> {
            http::response<http::dynamic_body> res{ http::status::ok, 11 };

           res.set(http::field::server, USER_AGENT_STRING);
           res.set(http::field::content_type, "text/html");
           res.keep_alive(req.keep_alive());


           http::dynamic_body::value_type value;
           const auto data = R"(
 <!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PharmaOffice 2!</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f0f0f0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }
        .container {
            text-align: center;
            background-color: #fff;
            padding: 50px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
            border-radius: 8px;
        }
        h1 {
            color: #333;
            margin-bottom: 20px;
        }
        p {
            color: #666;
            margin-bottom: 40px;
        }
        .button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            text-decoration: none;
            border-radius: 5px;
        }
        .button:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Welcome to PharmaOffice!</h1>
        <p>We are attempting to change the way pharmacy is practiced in Nigeria, through innovation.</p>
        <a href="#" class="button">Get PharmaOffice</a>
    </div>
</body>
</html>
            )"s;
           auto buffer = value.prepare(data.size());
           boost::asio::buffer_copy(buffer, boost::asio::buffer(data));
           value.commit(data.size());

           res.body() = value;
           res.prepare_payload();
           co_return res;
    });

    app->route("/about/{path}/{second}", [&](http::request<http::dynamic_body>&& req, boost::urls::matches&& m)
        -> boost::asio::awaitable<http::response<http::dynamic_body>> {
            auto p = m["path"];
            auto a = m["second"];
            http::response<http::dynamic_body> res{ http::status::ok, 11 };

            res.set(http::field::server, USER_AGENT_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());

            http::dynamic_body::value_type value;
            std::ostringstream os;
            os << "<h1>This is grape juice: "s << std::string(p) << " " << "</h1>" << std::endl;
            os << "<p>Welcome to the server</p>";
            const auto data = os.str();
            auto buffer = value.prepare(data.size());
            boost::asio::buffer_copy(buffer, boost::asio::buffer(data));
            value.commit(data.size());

            res.body() = value;
            res.prepare_payload();
            co_return res;
     });

    app->Run();
    int a;
    std::cin >> a;
    app->Exit();
    return 0;
}