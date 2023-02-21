#include "main.hpp"
int main() {
  try {
    Proxy * proxy = new Proxy();
    proxy->start();
  } 
  catch (std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
  }
  return 0;
}