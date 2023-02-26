#include "response.hpp"


void Response::parseHeader() {
  for (auto& r : response.base()) {
    //std::cout << "Field: " << r.name() << "/text: " << r.name_string() << ", Value: " << r.value() << "\n";
    //Print like：
    //Field: Cache-Control/text: Cache-Control, Value: max-age=604800
    //Field: Date/text: Date, Value: Tue, 23 Jun 2020 16:43:43 GMT
    //Field: ETag/text: Etag, Value: "3147526947+ident"
    //Field: Expires/text: Expires, Value: Tue, 30 Jun 2020 16:43:43 GMT
    //Field: Last-Modified/text: Last-Modified, Value: Thu, 17 Oct 2019 07:18:26 GMT
    boost::beast::string_view name = r.name_string();
    boost::beast::string_view value = r.value();
    headers.push_back(std::make_pair(name,value));
  }
}

bool Response::noCache(){

}
