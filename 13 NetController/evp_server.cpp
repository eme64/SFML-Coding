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

// ---------- FILE SERVER
void
evp::FileServer::handleRequest(std::string &ret, const std::string &url) {
   URL u(url);
   std::cout << "path: " << u.path << " ext: " << u.pathExt << std::endl;
   const auto &it = files_.find(u.path);
   if(it!=files_.end()) {
      ret = it->second->get(u);

      if(u.pathExt == "") {
         ret = evp::Server::HTTP_text + ret;
      } else if(u.pathExt == "html") {
         ret = evp::Server::HTTP_text + ret;
      } else if(u.pathExt == "js") {
         ret = std::string("HTTP/1.0 200 Ok\n")
                 + "Content-Type: text/javascript;charset=UTF-8\n"
        	 + "Content-Length: " + std::to_string(ret.size()) + "\n"
        	 + "\n"
        	 +ret;
      } else if(u.pathExt == "png") {
         ret = std::string("HTTP/1.0 200 Ok\n")
                 + "Content-Type: image/png\n"
                 + "Content-Length: " + std::to_string(ret.size()) + "\n"
                 + "\n"
                 +ret;
      } else if(u.pathExt == "ico") {
         ret = std::string("HTTP/1.0 200 Ok\n")
                 + "Content-Type: image/x-icon\n"
                 + "Content-Length: " + std::to_string(ret.size()) + "\n"
                 + "\n"
                 +ret;
      }
   } else {
      ret = HTTP_text + std::string("404: ") + url;
   }
}

void
evp::FileServer::registerFile(const std::string &url, const std::string &filename) {
   files_[url] = new evp::FileServer::FileItem(filename);
}

void
evp::FileServer::registerString(const std::string &url, const std::string &data) {
   files_[url] = new evp::FileServer::StringItem(data);
}
void
evp::FileServer::registerFunction(const std::string &url, const evp::FileServer::FunctionItem::F f) {
   files_[url] = new evp::FileServer::FunctionItem(f);
}

