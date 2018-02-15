#include "evp_networking.hpp"



// ############################## MAIN ##############################
// ############################## MAIN ##############################
// ############################## MAIN ##############################
int main(int argc, char** argv)
{
    // ----------------------------- Handle Commandline Input
    NET_SERVER_MODE = false;
    if(argc > 1)
    {
      std::string serv = "server";
      std::string arg0 = argv[1];
      if(!arg0.compare(serv))
      {
        std::cout << "Server Mode!" << std::endl;
        NET_SERVER_MODE = true;
      }
    }

    if(!NET_SERVER_MODE)
    {
      std::cout << " [INPUT HELP] use 'server' as argument to start a server!" << std::endl;
    }

    // ------------------------------------ WINDOW
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE_X, SCREEN_SIZE_Y), "first attempts", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);

    // ------------------------------------ FONT
    if (!FONT.loadFromFile("arial.ttf"))
    {
        std::cout << "font could not be loaded!" << std::endl;
        return 0;
    }

    // --------------------------- EChat
    EChat chat;


    // --------------------------- Setup Networking
    if(NET_SERVER_MODE)
    {
      // -------------- SERVER:
      // chat handler:
      auto f_chat = [](NServerClient* client, const std::string &s)
      {
        std::string line;

        if(client != NULL)
        {
          // client input:

          if(s == "/help")
          {

            std::string txt= "%16%[help]%% use the percent symbol to get all the %0%R%16%G%32%B%rand% colors!";
            client->np_send.enqueueChat(txt);
            return;
          }

          line = "%255,255,255%[" + std::to_string(client->id) + "]%% " + s;
        }else{
          // server input:
          line = "%8%[server]%% " + s;

          if(s == "/help")
          {
            NET_SERVER->chat->push("%16%[help]%% use the percent symbol to get all the %0%R%16%G%32%B%rand% colors!");
            return;
          }else if(s == "/data"){
            NET_SERVER->chat->push("%16%[data]%% request some data...");

            // test data request:
            auto f_success = [](char* data, size_t size)
            {
              std::cout << "[]data received! size: " << std::to_string(size) << std::endl;

              if(data == NULL){return;}

              for(size_t i = 0; i<size; i++)
              {
                char c = i;

                if(data[i] != c)
                {
                  std::cout << "[]off at " << i << std::endl;
                }
              }
              std::cout << "[]check complete" << std::endl;
              std::free(data);
              data = NULL;
            };

            auto f_failure = [](bool noTimeout, std::string errorMsg)
            {
              std::cout << "[].test file name. failed" << std::endl;
              std::cout << "[]ERROR: " << errorMsg << std::endl;
              if(noTimeout)
              {
                std::cout << "[]not even timeout" << std::endl;
              }else{
                std::cout << "[]timeout" << std::endl;
              }
            };

            std::map<int, NServerClient*>::iterator it;
          	for (it=NET_SERVER->map_id.begin(); it!=NET_SERVER->map_id.end(); ++it)
          	{
          		NServerClient* c = it->second;
              c->np_send.issueDataRequest("testdata", f_success, f_failure);
            }
            return;
          }
        }

        NET_SERVER->chat->push(line);
        NET_SERVER->broadcastChat(line);
      };

      // data request handler:
      auto f_dataReq = [](std::string fileName, NServerClient* client, bool& success, char* &data, size_t &data_size, std::string& errorMsg)
      {
        std::cout << "[RH]DATA REQUEST:" << std::endl;
        if(client!=NULL)
        {
          std::cout << "[RH]client id: " << std::to_string(client->id) << std::endl;
        }else{
          std::cout << "[RH]by server" << std::endl;
        }

        if(fileName == "testdata")
        {
          // set dummy data here:
          data_size = 10000;
          data = (char*)std::malloc(data_size);

          for(size_t i = 0; i<data_size; i++)
          {
            data[i] = i;
          }

          success = true;
        }else{
          // we will now just send error instead:
          success = false;
          errorMsg = "this is only the dummy dataRequestHandler !";
        }
      };

      // server
      NET_SERVER = new NServer(&chat, f_chat, f_dataReq);


    }else{
      // ------------------ CLIENT:
      // data request handler:
      auto f_dataReq = [](std::string fileName, NServerClient* client, bool& success, char* &data, size_t &data_size, std::string& errorMsg)
      {
        std::cout << "[RH]DATA REQUEST:" << std::endl;

        success = false;
        errorMsg = "Client does not handle dataRequests !";
      };

      // client:
      NET_CLIENT = new NClient(&chat, f_dataReq);

      // test data request:
      auto f_success = [](char* data, size_t size)
      {
        std::cout << "[]data received! size: " << std::to_string(size) << std::endl;

        if(data == NULL){return;}

        for(size_t i = 0; i<size; i++)
        {
          char c = i;

          if(data[i] != c)
          {
            std::cout << "[]off at " << i << std::endl;
          }
        }
        std::cout << "[]check complete" << std::endl;
        std::free(data);
        data = NULL;
      };

      auto f_failure = [](bool noTimeout, std::string errorMsg)
      {
        std::cout << "[].test file name. failed" << std::endl;
        std::cout << "[]ERROR: " << errorMsg << std::endl;
        if(noTimeout)
        {
          std::cout << "[]not even timeout" << std::endl;
        }else{
          std::cout << "[]timeout" << std::endl;
        }
      };

      for(int i=0; i<3; i++)
      {
        NET_CLIENT->np_send.issueDataRequest("testdata", f_success, f_failure);
      }
    }
    std::string textInput;

    // --------------------------- Render Loop
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
                // window closed
                case sf::Event::Closed:
                  window.close();
                  break;
                case sf::Event::TextEntered:
                  //std::cout << uint16_t(event.text.unicode) << std::endl;
                  if (event.text.unicode < 128)
                  {
                    InputHandler::push(static_cast<char>(event.text.unicode));
                    //textInput += static_cast<char>(event.text.unicode);
                    //std::cout << "ASCII character typed: " << static_cast<char>(event.text.unicode) << std::endl;
                  }
                  break;
                // we don't process other types of events
                default:
                  break;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)){
          window.close();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F1) && window.hasFocus()){
          window.close();
        }

        // chat INPUT



        // MAIN LOOP:
        window.clear();

        if(NET_SERVER_MODE)
        {
          // server
          NET_SERVER->update();
        }else{
          // client
          NET_CLIENT->update();
        }

        if(NET_SERVER_MODE)
        {
          // server
          NET_SERVER->draw(window);
        }else{
          // client
          NET_CLIENT->draw(window);
        }

        chat.update();

        chat.draw(0,600, window);

        window.display();
    }

    if(NET_SERVER_MODE)
    {
      // server
      delete(NET_SERVER);
    }else{
      // client
      delete(NET_CLIENT);
    }

    return 0;
}
