#include <iostream>
#include <fstream>
#include <chrono>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <vector>

using json = nlohmann::json;

std::string generate_filename(const std::string& symbol) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "/app/data/parser_input/" << symbol << "_data_"
       << ".json";
    return ss.str();
}

json get_binance_data(const std::string& symbol, const std::string& interval, int limit) {
    std::string url = "https://api.binance.com/api/v3/klines";
    cpr::Parameters params = {
        {"symbol", symbol},
        {"interval", interval},
        {"limit", std::to_string(limit)}
    };

    auto response = cpr::Get(cpr::Url{url}, params);
    
    if (response.status_code != 200) {
        throw std::runtime_error("Ошибка запроса для " + symbol + ": " + response.text);
    }

    return json::parse(response.text);
}

json process_data(const json& raw_data) {
    json result = json::array();
    
    for (const auto& item : raw_data) {
        result.push_back({
            {"timestamp", item[0]},
            {"open", item[1]},
            {"high", item[2]},
            {"low", item[3]},
            {"close", item[4]},
            {"volume", item[5]}
        });
    }
    
    return result;
}

int main() {
    try {
        const std::string interval = "1m";
        const int limit = 1440;
        
        std::vector<std::string> top_symbols = {
            "BTCUSDT",  
            "ETHUSDT",  
            "BNBUSDT", 
            "XRPUSDT", 
            "ADAUSDT", 
            "SOLUSDT",  
            "MATICUSDT", 
            "DOTUSDT",  
            "TRXUSDT",  
            "AVAXUSDT"  
        };
        
        for (const auto& symbol : top_symbols) {
            try {
                auto raw_data = get_binance_data(symbol, interval, limit);
                auto processed_data = process_data(raw_data);
                auto file_path = generate_filename(symbol);
                
                std::ofstream out_file(file_path);
                out_file << processed_data.dump(4);
                out_file.close();
                
                std::cout << "Data for " << symbol <<  " upload in " << file_path << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error " << symbol << ": " << e.what() << std::endl;
                continue;
            }
        }
        
        
    } catch (const std::exception& e) {
        std::cerr << "Error" << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}