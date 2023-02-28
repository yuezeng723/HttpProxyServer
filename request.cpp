#include "request.hpp"

void Request::parseHeader() {
    auto cache_control = request[http::field::cache_control];
    cache_control_string = cache_control.to_string();
    searchCacheControl();
}

void Request::searchCacheControl() {  // If there is no-cache, return true
    // Find the position of the '=' and ',' characters for max-age and max-stale
    std::size_t maxStale_pos = cache_control_string.find("max-stale=");
    std::size_t minFresh_pos = cache_control_string.find("min-fresh=");

    // Extract the value of max-stale
    if (maxStale_pos != std::string::npos) {
        has_max_stale = 1;
        std::size_t comma_pos = cache_control_string.find(',', maxStale_pos);
        if (comma_pos != std::string::npos) {
            std::string maxStale_str = cache_control_string.substr(maxStale_pos + 10, comma_pos - maxStale_pos - 10);
            max_stale = std::stoi(maxStale_str);
            // std::cout << "max-stale value: " << max_stale_value << std::endl; // 3600ms / 10
        } else {
            std::string maxStale_str = cache_control_string.substr(maxStale_pos + 10, cache_control_string.length() - maxStale_pos - 10);
            max_stale = std::stoi(maxStale_str);
            // std::cout << "max-stale value: " << max_stale_value << std::endl;
        }
    } else {
        has_max_stale = 0;
        max_stale = 0;
    }

    // Extract the value of min-fresh
    if (minFresh_pos != std::string::npos) {
        std::size_t comma_pos = cache_control_string.find(',', minFresh_pos);
        if (comma_pos != std::string::npos) {
            std::string minFresh_str = cache_control_string.substr(minFresh_pos + 10, comma_pos - minFresh_pos - 10);
            min_fresh = std::stoi(minFresh_str);
            // std::cout << "max-stale value: " << max_stale_value << std::endl; // 3600ms / 10
        } else {
            std::string minFresh_str = cache_control_string.substr(minFresh_pos + 10, cache_control_string.length() - minFresh_pos - 10);
            min_fresh = std::stoi(minFresh_str);
            // std::cout << "max-stale value: " << max_stale_value << std::endl;
        }
    } else {
        has_min_fresh = 0;
        min_fresh = 0;
    }
}