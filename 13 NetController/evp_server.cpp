#include "evp_server.hpp"

// --------- SPLIT
std::vector<std::string>
evp::split(const std::string &text, char sep) {
   std::vector<std::string> tokens;
   std::size_t start = 0, end = 0;
   while ((end = text.find(sep, start)) != std::string::npos) {
      tokens.push_back(text.substr(start, end - start));
      start = end + 1;
   }
   tokens.push_back(text.substr(start));
   return tokens;
}

// --------- SERVER
void
evp::Server::handleError(std::string &ret, const std::string &error) {
   ret = HTTP_text + std::string("Error: ") + error;
}

void
evp::Server::handleRequest(std::string &ret, const std::string &url) {
   ret = HTTP_text + std::string("404: ") + url;
}

const std::string
evp::Server::HTTP_text ="HTTP/ 1.1 200 OK\nContent-Type: text/html\n\n";
