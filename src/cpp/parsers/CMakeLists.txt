cmake_minimum_required(VERSION 3.10)
project(Parser)

find_package(OpenSSL REQUIRED)  
find_package(cpr REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(parser parser_main.cpp)
target_link_libraries(parser PRIVATE 
    cpr::cpr 
    nlohmann_json::nlohmann_json
    OpenSSL::SSL  
)