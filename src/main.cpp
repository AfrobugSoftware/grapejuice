#include <iostream>
#include <crow.h>
#include <date/date.h>
#include <spdlog/spdlog.h>
#include <format>
#include <filesystem>
#include <fstream>


namespace fs = std::filesystem;
int main(int argc, char** argv)
{
    std::cout << "grapejuice 1.0.0" << std::endl;
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]() {
        spdlog::info("Route page reached");
        return "hello world";
    });

    CROW_ROUTE(app, "/form")([]() {
        auto page = crow::mustache::load_text("forms.html");
        return page;
    });

    CROW_ROUTE(app, "/example")([]() {
        auto page = crow::mustache::load_text("example.html");
        return page;
    });

    CROW_ROUTE(app, "/<string>")([](const std::string& str) {
        std::string filepath= fmt::format("{}.html", str);
        auto page = crow::mustache::load_text(filepath);
        return page;
    });

    CROW_ROUTE(app, "/img/<string>")([](const std::string& str) {
        crow::response res;
        std::filesystem::path path = fs::current_path() / "templates" / "img" / str;
    
        res.set_static_file_info_unsafe(path.string());
        return res;
    });




    app.port(18080).multithreaded().run();
    return 0;
}