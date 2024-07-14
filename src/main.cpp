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
    app->Init(argc, argv);

    app->route("/", [](pof::base::net_manager::req_t&& req, 
        boost::urls::matches&& m) -> boost::asio::awaitable<pof::base::net_manager::res_t> {
            pof::base::net_manager::res_t res{ http::status::ok, 11 };

           res.set(http::field::server, USER_AGENT_STRING);
           res.set(http::field::content_type, "text/html");
           res.keep_alive(req.keep_alive());


           pof::base::net_manager::res_t::body_type::value_type value;
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
        <a href="app/getoffice" class="button">Get PharmaOffice</a>
    </div>
</body>
</html>
            )"s;
           value.resize(data.size());
           std::copy(data.begin(), data.end(), value.begin());

           res.body() = std::move(value);
           res.prepare_payload();
           co_return res;
    });

    grape::credentials cred;
    cred.account_id = boost::uuids::random_generator_mt19937{}();
    cred.branch_id = boost::uuids::random_generator_mt19937{}();
    cred.pharm_id = boost::uuids::random_generator_mt19937{}();
    cred.session_id = boost::uuids::random_generator_mt19937{}();

    const size_t size = grape::serial::get_size(cred);

    grape::response::body_type::value_type value(size, 0x00);

    grape::serial::write(boost::asio::buffer(value), cred);

    std::cout << boost::lexical_cast<std::string>(cred.account_id) << std::endl;

    for (auto c : value) {
        std::cout << std::hex << c;
    }
    std::cout << std::endl;

    auto&& [cred2, buf] = grape::serial::read<grape::credentials>(boost::asio::buffer(value));

    std::cout << boost::lexical_cast<std::string>(cred2.account_id) << std::endl;


    app->Run();
    int a;
    std::cin >> a;
    app->Exit();
    return 0;
}