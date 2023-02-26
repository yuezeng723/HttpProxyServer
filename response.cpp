#include "response.hpp"

void Response::parseHeader() {
    for (auto& r : response.base()) {
        // std::cout << "Field: " << r.name() << "/text: " << r.name_string() << ", Value: " << r.value() << "\n";
        // Print like：
        // Field: Cache-Control/text: Cache-Control, Value: max-age=604800
        // Field: Date/text: Date, Value: Tue, 23 Jun 2020 16:43:43 GMT
        // Field: ETag/text: Etag, Value: "3147526947+ident"
        // Field: Expires/text: Expires, Value: Tue, 30 Jun 2020 16:43:43 GMT
        // Field: Last-Modified/text: Last-Modified, Value: Thu, 17 Oct 2019 07:18:26 GMT
        boost::beast::string_view name = r.name_string();
        boost::beast::string_view value = r.value();
        headers.push_back(std::make_pair(name, value));
    }
}

void Response::searchCacheControl() {//If there is no-cache, return true
    std::string targetName = "Cache-Control";
    auto result = std::find_if(headers.begin(), headers.end(), [targetName](const std::pair<std::string, std::string>& p) {
        return p.first == targetName;
    });

    if (result != headers.end()) {//find targetName
        //std::cout << "Match found: " << result->second << std::endl;
        noCache = result->second.find("no-cache")!=std::string::npos;
        noStore = result->second.find("no-store")!=std::string::npos;
        pri = result->second.find("private")!=std::string::npos;
        mustRevalidate = result->second.find("must-revalidate")!=std::string::npos;
    } else {
        //std::cout << "No match found." << std::endl;
        noCache = 0;
        noStore = 0;
        pri = 0;
        mustRevalidate = 0;
    }
}
