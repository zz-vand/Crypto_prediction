cmake_minimum_required(VERSION 3.10)
project(Visualization)

add_compile_options(-fsanitize=address,undefined)
add_link_options(-fsanitize=address,undefined)

set(MATPLOTPP_DIR "/usr/local/lib/cmake/Matplot++")
find_package(Matplot++ REQUIRED PATHS ${MATPLOTPP_DIR} NO_DEFAULT_PATH)


find_package(OpenSSL REQUIRED)  
find_package(cpr REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(visualization visualization.cpp)
target_link_libraries(visualization PRIVATE 
    Matplot++::matplot
    nlohmann_json::nlohmann_json
    cpr::cpr 
    OpenSSL::SSL 
)