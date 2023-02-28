#include "response.hpp"

void Response::parseHeader() {
    auto cache_control = response[http::field::cache_control];
    cache_control_string = cache_control.to_string();
    searchCacheControl();
    auto etag = response[http::field::etag];
    etag_string = etag.to_string();
    auto last_modify = response[http::field::last_modified];
    last_modify_string = last_modify.to_string();
}

void Response::searchCacheControl() {  // If there is no-cache, return true
    noCache = cache_control_string.find("no-cache") != std::string::npos;
    noStore = cache_control_string.find("no-store") != std::string::npos;
    pri = cache_control_string.find("private") != std::string::npos;
    mustRevalidate = cache_control_string.find("must-revalidate") != std::string::npos;

    // Find the position of the '=' and ',' characters for max-age and max-stale
    std::size_t maxAge_pos = cache_control_string.find("max-age=");

    // Extract the value of max-age
    if (maxAge_pos != std::string::npos)
    {
        std::size_t comma_pos = cache_control_string.find(',', maxAge_pos);
        if (comma_pos != std::string::npos)
        {
            std::string maxAge_str = cache_control_string.substr(maxAge_pos + 8, comma_pos - maxAge_pos - 8);
            max_age = std::stoi(maxAge_str);
            //std::cout << "max-age value: " << max_age_value << std::endl;
        }
        else{
            std::string maxAge_str = cache_control_string.substr(maxAge_pos + 8, cache_control_string.length() - maxAge_pos - 8);
            max_age = std::stoi(maxAge_str);
            //std::cout << "max-age value: " << max_age_value << std::endl;
        }
    }
    else{
        max_age = 0;
    }
}
