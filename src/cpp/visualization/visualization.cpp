#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <matplot/matplot.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>
#include <cpr/cpr.h>
#include <mutex>

using json = nlohmann::json;
namespace fs = std::filesystem;
namespace plt = matplot;

std::atomic<bool> keep_running{true};
std::mutex data_mutex;

const std::vector<std::string> symbols = {
    "BTCUSDT", "ETHUSDT", "BNBUSDT", "XRPUSDT", "ADAUSDT",
    "SOLUSDT", "MATICUSDT", "DOTUSDT", "TRXUSDT", "AVAXUSDT"
};

struct CoinData {
    std::vector<double> predictions;
    std::vector<double> real_prices;
    plt::figure_handle figure;
};

std::map<std::string, CoinData> coins;

std::vector<double> load_predictions(const std::string& filepath) {
    std::ifstream file(filepath);
    json data;
    file >> data;
    
    std::vector<double> predictions;
    for (const auto& inner_vec : data["predictions"]) {
        if (!inner_vec.empty()) {
            predictions.push_back(inner_vec[0]);
        }
    }
    return predictions;
}

double get_latest_close_price(const std::string& symbol) {
    auto response = cpr::Get(
        cpr::Url{"https://api.binance.com/api/v3/klines"},
        cpr::Parameters{
            {"symbol", symbol},
            {"interval", "1m"},
            {"limit", "1"}
        }
    );
    
    if (response.status_code != 200) {
        throw std::runtime_error("Error " + symbol);
    }
    
    auto json_data = json::parse(response.text);
    return std::stod(json_data[0][4].get<std::string>());
}

void initialize_coins() {
    const std::string pred_dir = "/app/data/model_output";
    
    for (const auto& symbol : symbols) {
        try {
            std::string pred_file;
            for (const auto& entry : fs::directory_iterator(pred_dir)) {
                if (entry.path().string().find(symbol) != std::string::npos) {
                    pred_file = entry.path().string();
                    break;
                }
            }
            
            if (pred_file.empty()) {
                throw std::runtime_error("No prediction " + symbol);
            }
            
            CoinData data;
            data.predictions = load_predictions(pred_file);
            data.figure = plt::figure(true);
            
            std::vector<double> time(data.predictions.size());
            for (size_t i = 0; i < time.size(); ++i) time[i] = i;
            
            plt::plot(time, data.predictions, "r-")
                ->line_width(2);
            
            plt::title(symbol + " Price Prediction vs Reality");
            plt::xlabel("Time (minutes)");
            plt::ylabel("Price");
            plt::grid(true);
            
            coins[symbol] = std::move(data);
            
        } catch (const std::exception& e) {
            std::cerr << "Error initializing " << symbol << ": " << e.what() << std::endl;
        }
    }
}

constexpr auto PREDICTION_UPDATE_INTERVAL = std::chrono::hours(4);
constexpr auto PRICE_UPDATE_INTERVAL = std::chrono::minutes(1);

void update_prices() {
    auto last_prediction_update = std::chrono::steady_clock::now();
    
    while (keep_running) {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(data_mutex);
        
        bool update_predictions = (now - last_prediction_update) >= PREDICTION_UPDATE_INTERVAL;
        
        for (auto& [symbol, data] : coins) {
            try {
                double price = get_latest_close_price(symbol);
                data.real_prices.push_back(price);
                
                plt::figure(data.figure);
                
                if (update_predictions) {
                    plt::cla();
                    
                    std::vector<double> pred_time(data.predictions.size());
                    for (size_t i = 0; i < pred_time.size(); ++i) pred_time[i] = i;
                    
                    plt::plot(pred_time, data.predictions, "r-")
                        ->line_width(2);
                    
                    plt::title(symbol + " Price Prediction vs Reality");
                    plt::xlabel("Time (minutes)");
                    plt::ylabel("Price");
                }
                
                std::vector<double> real_time(data.real_prices.size());
                for (size_t i = 0; i < real_time.size(); ++i) real_time[i] = i;
                
                plt::hold(plt::on);
                plt::plot(real_time, data.real_prices, "b-")
                    ->line_width(2);
                plt::hold(plt::off);
                plt::grid(true);
                
                const std::string output_dir = "/app/data/results";
                if (!fs::exists(output_dir)) fs::create_directory(output_dir);
                plt::save(output_dir + "/" + symbol + ".png");
                
            } catch (const std::exception& e) {
                std::cerr << "Error updating " << symbol << ": " << e.what() << std::endl;
            }
        }
        
        if (update_predictions) {
            last_prediction_update = now;
        }
        
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - now);
        auto sleep_time = PRICE_UPDATE_INTERVAL - elapsed;
        
        if (sleep_time > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleep_time);
        }
    }
}

int main() {
    try {
        initialize_coins();
        
        std::thread update_thread(update_prices);
        
        update_thread.join();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        keep_running = false;
        return 1;
    }
    
    return 0;
}