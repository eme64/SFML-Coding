#include "evp_networking.hpp"
#include <fstream>


/* TODO:
asdfasdfasdfasdf
*/

int main(int argc, char** argv)
{
    // ----- network config:
    unsigned short net_conf_port = 54000;
    sf::IpAddress net_conf_ip = "localhost";

    std::ifstream net_conf_file("networking.conf");
    std::vector<std::string> lines;
    for(std::string l; std::getline(net_conf_file, l);)
    {
      lines.push_back(l);
    }

    if(lines.size() >1)
    {
      net_conf_ip = lines[0];
      net_conf_port = (uint16_t)evp::str2int(lines[1]);
    }

    // ----------------------------- Handle Commandline Input
    bool isServer = false;
    if(argc > 1)
    {
      std::string serv = "server";
      std::string arg0 = argv[1];
      if(!arg0.compare(serv))
      {
        std::cout << "Server Mode!" << std::endl;
        isServer = true;
      }
    }

    if(!isServer)
    {
      std::cout << " [INPUT HELP] use 'server' as argument to start a server!" << std::endl;
    }

    // ---------------------------- Start Connector
    evp::Connector *con = NULL;
    if(isServer)
    {
      con = new evp::Connector(net_conf_port);
    }else{
      con = new evp::Connector(net_conf_port, net_conf_ip);
    }

    while(true)
    {
      con->update();
    }

    return 0;
}
