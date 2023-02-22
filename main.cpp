#include "main.hpp"
#include <boost/version.hpp>
int main() {
  try {
    std::cout << "Boost version: " << BOOST_LIB_VERSION << std::endl;
    Proxy * proxy = new Proxy();
    proxy->start();
  } 
  catch (std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
  }
  return 0;
}