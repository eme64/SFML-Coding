#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <list>
#include <queue>
#include <map>
#include <time.h>

struct NServerClient_identifier;
struct NServerClient;
struct NServer;
struct NClient;
struct cmpByNSC_ident;
struct NPacketRcvHandler;
struct NPacketSendHandler;

/* TODO

 --- completed:

 - package sequence wrap around -> infinite messages!
*/

typedef sf::Uint16 NPSeq;
const NPSeq NPSeq_HALF = 32768;
const NPSeq NPSeq_START_AT = 32768 + 32750;
/*
sf::Uint16 - 32768
*/

typedef std::map<NServerClient_identifier, NServerClient*, cmpByNSC_ident> NSCMapIdentifyer;
typedef std::map<int, NServerClient*> NSCMapId;
//----------------------------------- STATIC VARIABLES
float SCREEN_SIZE_X = 800;
float SCREEN_SIZE_Y = 600;

sf::Font FONT;

bool NET_SERVER_MODE;
NServer* NET_SERVER;
NClient* NET_CLIENT;

sf::Clock PROGRAM_CLOCK;

// --------------------------------------------------------- DRAWING
void DrawDot(float x, float y, sf::RenderWindow &window, sf::Color color = sf::Color(255,0,0))
{
  float r = 3;
  sf::CircleShape shape(r);
  shape.setFillColor(color);
  shape.setPosition(x-r, y-r);
  window.draw(shape, sf::BlendAdd);
}

void DrawLine(float x1, float y1, float x2, float y2, sf::RenderWindow &window)
{
  sf::Vertex line[] =
  {
    sf::Vertex(sf::Vector2f(x1,y1), sf::Color(0,0,0)),
    sf::Vertex(sf::Vector2f(x2,y2), sf::Color(255,255,255))
  };
  window.draw(line, 2, sf::Lines, sf::BlendAdd);
}

void DrawText(float x, float y, std::string text, float size, sf::RenderWindow &window, sf::Color color = sf::Color(255,255,255))
{
  sf::Text shape(text, FONT);//(text, FONT);
  //shape.setFont(FONT);
  //shape.setString(text); //std::to_string(input[i])
  shape.setCharacterSize(size);
  shape.setColor(color);
  shape.setPosition(x,y);
  //shape.setStyle(sf::Text::Bold | sf::Text::Underlined);
  window.draw(shape, sf::BlendAdd);
}

// ------------------------------------------------------- Networking
// just send a string
const int NET_HEADER_ECHO = 100;

const int NET_HEADER_PING = 200;
const int NET_HEADER_PONG = 201;

// announce the recieve port of client
const int NET_HEADER_CLIENT_ANNOUNCE = 2000;
// server id ping -> as first message over client channel
const int NET_HEADER_SERVERCLIENT_IDPING = 3000;

// the NPacket
const int NET_HEADER_NPacket = 4000;

// ---------------- The Necessary Packets:
/*
Idea:
packets that are enqueued here must be received
but the order is not guaranteed
do not use this for speed critical stuff
to be used for chat, commands, data

Each message has a tag and sequence number.
the tag is used to seperate for different uses
the idea is that one can listen for them elsewhere -> set a function!

the structure of a message:
seq. number, tag, string
*/
inline bool sequence_greater_than( NPSeq s1, NPSeq s2 )
{
  return ( ( s1 > s2 ) && ( s1 - s2 <= NPSeq_HALF ) ) ||
  ( ( s1 < s2 ) && ( s2 - s1  > NPSeq_HALF ) );
}

struct NPacket
{
  NPSeq seq;
  sf::Uint16 tag;
  std::string str;

  // book keeping:
  sf::Int32 time_sent; // for timeout and resend
};

struct NPacketRcvHandler
{
  NPSeq highest_seq_num_rcvd = NPSeq_START_AT;

  sf::Uint16 packet_counter = 0;

  std::map<NPSeq, NPSeq> map_missing;


  void createAckMessage(sf::Packet &p);
  bool hasNews; // if the ack situation changed since last createAckMessage

  void incoming(sf::Packet &p, NPacketSendHandler &np_send);
};

struct NPacketSendHandler
{
  NPSeq last_sequence_number = NPSeq_START_AT;
  NPSeq oldest_sequence_waiting = NPSeq_START_AT;

  sf::Int32 msgTimeOutLimit = 500; // then resend it
  int num_wait_ack_limit = 100; // this will affect the transmission speed!

  std::queue<NPacket> queue;
  /*
  q.push(root)
  --
  size_t c = q.front();
  q.pop();
  */
  std::map<NPSeq, NPacket> map_sent;

  sf::Int32 time_last_send;
  sf::Int32 minimum_send_delay = 200;

  NPacketSendHandler()
  {
    time_last_send = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
  }

  int enqueue(sf::Uint16 tag, const std::string &txt);
  void update(NPacketRcvHandler &np_rcv, sf::UdpSocket &socket, const sf::IpAddress &ip, const unsigned short &port);
  void readAcks(sf::Packet &p);

  std::string missingToString();
};

int NPacketSendHandler::enqueue(sf::Uint16 tag, const std::string &txt)
{
  // returns sequence number of the Packet
  last_sequence_number++;

  queue.push(NPacket{last_sequence_number, tag, txt});

  return queue.size();
}

void NPacketSendHandler::update(NPacketRcvHandler &np_rcv, sf::UdpSocket &socket, const sf::IpAddress &ip, const unsigned short &port)
{
  sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

  if((now_time - time_last_send)>minimum_send_delay && (queue.size() > 0 || map_sent.size() < num_wait_ack_limit || np_rcv.hasNews))
  {
    time_last_send = now_time;
    // something to send:
    sf::Packet p;
    p << NET_HEADER_NPacket;

    np_rcv.createAckMessage(p);

    // update from map (the ones that have timed out)
    std::map<NPSeq, NPacket>::iterator it;

    for (it=map_sent.begin(); it!=map_sent.end(); ++it)
    {
      NPSeq seq = it->first;
      sf::Int32 sent_time = it->second.time_sent;
      if( (now_time - sent_time) > msgTimeOutLimit)
      {
        if( ((float) rand() / (RAND_MAX)) < 0.8 )
        {
          p << it->second.seq << it->second.tag << it->second.str;

          if( ((float) rand() / (RAND_MAX)) < 0.1 )
          {
            p << it->second.seq << it->second.tag << it->second.str; // send double!
          }
        }

        it->second.time_sent = now_time;
      }
    }

    // append new ones
    while(queue.size() > 0 && map_sent.size() < num_wait_ack_limit && p.getDataSize() < 50000)
    {
      // send from queue, append to map
      NPacket np = queue.front();
      queue.pop();

      np.time_sent = now_time;

      // artificially loose and double some packets:
      if( ((float) rand() / (RAND_MAX)) < 0.8 )
      {
        p << np.seq << np.tag << np.str;

        if( ((float) rand() / (RAND_MAX)) < 0.1 )
        {
          p << np.seq << np.tag << np.str; // send double!
        }
      }

      map_sent.insert(
        std::pair<NPSeq, NPacket>
        (np.seq, np)
      );
    }

    // send packet:
    if (socket.send(p, ip, port) != sf::Socket::Done)
    {
        std::cout << "Error on send ! (NPacket)" << std::endl;
    }
  }
}

void NPacketSendHandler::readAcks(sf::Packet &p)
{
  NPSeq highest;
  sf::Uint16 num_missing;

  p >> highest >> num_missing;

  std::vector<NPSeq> missingVec;
  missingVec.reserve(num_missing+1);
  std::vector<NPSeq> missingWrapVec;
  missingWrapVec.reserve(num_missing);

  for(sf::Uint16 i = 0; i<num_missing; i++)
  {
    NPSeq seq;
    p >> seq;

    if(seq > highest){
      // wrapping around!
      missingWrapVec.push_back(seq);
    }else{
      // normal
      missingVec.push_back(seq);
    }
  }
  missingVec.push_back(highest+1);

  std::vector<NPSeq> deletees;
  deletees.reserve(map_sent.size());
  std::map<NPSeq, NPacket>::iterator it;

  // go about deleting left over high ones (wrap)
  it = map_sent.upper_bound(NPSeq_HALF);
  if(highest < NPSeq_HALF && it != map_sent.end())
  {
    std::cout << "WRAP AROUND DANGER!" << std::endl;

    size_t i = 0; // index in missingWrapVec

    do {
      // work through rest of high elements:
      NPSeq seq = it->first;
      std::cout << seq << std::endl;

      while(i<missingWrapVec.size() && seq > missingWrapVec[i])
      {i++;}

      if(i>=missingWrapVec.size() || seq < missingWrapVec[i])
      {
        // can delete:
        deletees.push_back(seq);
        std::cout << "del." << std::endl;
      }

      // loop stuff below
      ++it;
    } while(it != map_sent.end() && i<missingWrapVec.size());
  }

  size_t i = 0; // index in missingVec

  // iterate through all that are still to be acked:
  for (it=map_sent.begin(); it!=map_sent.end(); ++it)
  {
    NPSeq seq = it->first;

    if( seq <= highest )// seq <= highest
    {
      while(i<missingVec.size() && sequence_greater_than(seq, missingVec[i]))
      {
        // advance in acks:
        i++;
      }

      if(i<missingVec.size() && sequence_greater_than(missingVec[i], seq))
      {
        // acked!
        //map_sent.erase(seq);

        deletees.push_back(seq);
      }
    }
  }

  // actually delete the deletees:
  for(size_t i =0 ; i< deletees.size(); i++)
  {
    map_sent.erase(deletees[i]);
  }

  /*

  NPSeq highest = 0;
  sf::Uint16 num_missing = 0;

  p >> highest >> num_missing;

  NPSeq next_missing;
  int ack_counter = 0;

  if(num_missing == 0)
  {
    next_missing = highest+1;
  }else{
    p >> next_missing;
    ack_counter++;
  }

  std::vector<NPSeq> deletees;

  // iterate through all that are still to be acked:
  std::map<NPSeq, NPacket>::iterator it;
  int counter = 0;

  it = map_sent.upper_bound(NPSeq_HALF);
  if(highest < NPSeq_HALF && it != map_sent.end())
  {
    std::cout << "WRAP AROUND DANGER! " << next_missing << std::endl;

    do {
      // work through rest of high elements:
      NPSeq seq = it->first;
      std::cout << seq << std::endl;

      // loop stuff below
      ++it;
    } while(it != map_sent.end());
  }


  for (it=map_sent.begin(); it!=map_sent.end(); ++it)
  {
    NPSeq seq = it->first;

    if( !sequence_greater_than(seq, highest) )// seq <= highest
    {
      while(sequence_greater_than(seq, next_missing) && ack_counter < num_missing)
      {
        // advance in acks:
        p >> next_missing;
        ack_counter++;
      }

      if(sequence_greater_than(next_missing, seq))
      {
        // acked!
        //map_sent.erase(seq);

        deletees.push_back(seq);
      }
    }
  }

  // actually delete the deletees:
  for(size_t i =0 ; i< deletees.size(); i++)
  {
    map_sent.erase(deletees[i]);
  }

  // read rest:
  while(ack_counter < num_missing)
  {
    // advance in acks:
    p >> next_missing;
    ack_counter++;
  }

  */
}


std::string NPacketSendHandler::missingToString()
{
  std::string ret;
  ret = "n: " + std::to_string(map_sent.size()) + ".  ";

  std::map<NPSeq, NPacket>::iterator it;
  for (it=map_sent.begin(); it!=map_sent.end(); ++it)
  {
    NPSeq seq = it->first;
    ret.append(" " + std::to_string(seq) + ",");
  }

  return ret;
}
// ---------------- NPacketRcvHandler

void NPacketRcvHandler::createAckMessage(sf::Packet &p)
{
  hasNews = false;

  p << highest_seq_num_rcvd;
  p << (sf::Uint16)(map_missing.size());

  // send all that are missing:
  std::map<NPSeq, NPSeq>::iterator it;
	for (it=map_missing.begin(); it!=map_missing.end(); ++it)
	{
		p << (it->second);
  }
}

void NPacketRcvHandler::incoming(sf::Packet &p, NPacketSendHandler &np_send)
{
  // header already removed.

  // read acks
  np_send.readAcks(p);

  // read messages
  while(!p.endOfPacket())
  {
    hasNews = true;

    NPacket np;
    p >> np.seq >> np.tag >> np.str;

    if(sequence_greater_than(np.seq, highest_seq_num_rcvd))
    {
      // insert all between:
      //if(np.seq > highest_seq_num_rcvd)
      {
        for(NPSeq i = highest_seq_num_rcvd+1; sequence_greater_than(np.seq, i); i++)
        {
          map_missing.insert(
            std::pair<NPSeq, NPSeq>
            (i,i)
          );
        }
      }/*else{
        std::cout << "wrap around ???" << std::endl;
      }*/

      highest_seq_num_rcvd = np.seq;

      packet_counter++;
      std::cout << "newest: " << np.seq << ":" << np.tag << " -> " << np.str << std::endl;
    }else{
      // one of the missing ones? else reject.
      std::map<NPSeq,NPSeq>::iterator it;

      it = map_missing.find(np.seq);
      if(it != map_missing.end()){
        map_missing.erase(np.seq);

        packet_counter++;
        std::cout << "missing: " << np.seq << ":" << np.tag << " -> " << np.str << std::endl;
  		}else{
  			// not found -> was a double!
  		}
    }
  }
}

// ---------------- Server - Client
struct NServerClient_identifier
{
  sf::IpAddress remoteIp;
  unsigned short remotePort;
};

struct cmpByNSC_ident {
  bool operator() (const NServerClient_identifier& lhs, const NServerClient_identifier& rhs) const
  {
    if(lhs.remotePort == rhs.remotePort)
    {
      return lhs.remoteIp < rhs.remoteIp;
    }else{
      return lhs.remotePort < rhs.remotePort;
    }
  }
};

struct NServerClient
{
  static int id_last;
  int id;
  sf::IpAddress remoteIp;
  unsigned short remotePort;

  NPacketRcvHandler np_rcv;
  NPacketSendHandler np_send;

  // time out ticker:
  sf::Int32 last_rcv;
  sf::Int32 last_ping;

  NServerClient(sf::IpAddress remoteIp, unsigned short remotePort)
    : remoteIp(remoteIp), remotePort(remotePort)
  {
    NServerClient::id_last++;
    id = NServerClient::id_last;

    std::cout << "my ID: " << id << std::endl;

    last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
    last_ping = last_rcv;
  }

  void sendPacket(sf::Packet &p, sf::UdpSocket &socket)
  {
    if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void sendIdPing(sf::UdpSocket &socket)
  {
    sf::Packet p;

    p << NET_HEADER_SERVERCLIENT_IDPING << id;
    if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void sendPing(sf::UdpSocket &socket)
  {
    sf::Packet p;
    p << NET_HEADER_PING;
    if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void sendPong(sf::UdpSocket &socket)
  {
    sf::Packet p;
    p << NET_HEADER_PONG;
    if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void rcvPacket(sf::Packet &p, sf::UdpSocket &socket)
  {
    last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

    int header;
    p >> header;

    switch (header)
    {
      case NET_HEADER_ECHO:
        {
          std::string txt;
          p >> txt;

          std::cout << "(" << id <<") Echo: " << txt << std::endl;

          break;
        }
      case NET_HEADER_PING:
      {
        // send back pong:
        sendPong(socket);
        break;
      }
      case NET_HEADER_PONG:
      {
        std::cout << "(" << id <<") Pong" << std::endl;
        break;
      }
      case NET_HEADER_NPacket:
      {
        np_rcv.incoming(p, np_send);
        break;
      }
      default:   // ------------ DEFAULT
        std::cout << "unreadable header: "<< header << std::endl;
        break;
    }
  }

  void update(sf::UdpSocket &socket)
  {
    // time out prevention:
    sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

    if( (now_time - last_rcv) > 500)
    {
      if( (now_time - last_ping) > 300)
      {
        last_ping = now_time;
        sendPing(socket);
      }
    }

    np_send.update(np_rcv, socket, remoteIp, remotePort);
  }

  void draw(float x, float y, sf::RenderWindow &window)
  {
    DrawText(x,y,
      "ID: " + std::to_string(id) + ", " + remoteIp.toString() + ":" + std::to_string(remotePort),
      12, window, sf::Color(255,0,0)
    );

    DrawText(x+150,y,
      "q: " + std::to_string(np_send.queue.size()) + ", w: " + std::to_string(np_send.map_sent.size()),
      12, window, sf::Color(255,255,255)
    );


    DrawText(x+230,y,
      "m: " + std::to_string(np_rcv.map_missing.size()) + ", h: " + std::to_string(np_rcv.highest_seq_num_rcvd) + ", cnt: " + std::to_string(np_rcv.packet_counter),
      12, window, sf::Color(0,255,255)
    );

    sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

    if( (now_time - last_rcv) > 500)
    {
      DrawText(x+400,y,
        "Timeout Danger: " + std::to_string( (float)(now_time - last_rcv)/1000.0 ) + " / 10",
        12, window, sf::Color(255,0,0)
      );
    }

  }
};
int NServerClient::id_last = 0;

struct NServer
{
  unsigned short socket_port = 54000;
  sf::UdpSocket socket;

  NSCMapIdentifyer map_identifyer;
  NSCMapId map_id;

  NServer()
  {
    // bind socket:
    if(socket.bind(socket_port) != sf::Socket::Done)
    {
      std::cout << "could not bind socket!" << std::endl;
      return;
    }
    socket.setBlocking(false);
    std::cout << "Server Created." << std::endl;
  }

  void addClient(NServerClient* c){
		map_identifyer.insert(
      std::pair<NServerClient_identifier, NServerClient*>
      (NServerClient_identifier{c->remoteIp, c->remotePort}, c)
    );

    map_id.insert(
      std::pair<int, NServerClient*>
      (c->id, c)
    );
	}

  void removeClient(NServerClient* c)
  {
    map_identifyer.erase(
      NServerClient_identifier{c->remoteIp, c->remotePort}
    );

    map_id.erase(
      c->id
    );
  }

	NServerClient* getClient(sf::IpAddress ip, unsigned short port){
		std::map<NServerClient_identifier, NServerClient*>::iterator it;

		it = map_identifyer.find(NServerClient_identifier{ip, port});

		if(it != map_identifyer.end()){
			return it->second;
		}else{
			return NULL;
		}
	}

  void draw(sf::RenderWindow &window)
  {
    DrawText(5,5, "Server Mode.", 12, window, sf::Color(255,0,0));

    // draw all clients:
    std::map<int, NServerClient*>::iterator it;

    int counter = 0;
		for (it=map_id.begin(); it!=map_id.end(); ++it)
		{
			NServerClient* c = it->second;

      c->draw(10, 20+counter*15,window);

      counter++;
		}
  }

  void update()
  {
    bool repeat;

    do
    {
      sf::Packet p;
      sf::IpAddress remoteIp;
      unsigned short remotePort;
      auto status = socket.receive(p, remoteIp, remotePort);

      repeat = false;
      if(status == sf::Socket::Done)
      {
        repeat = true;

        // find client:
        NServerClient* c = getClient(remoteIp, remotePort);
        if(c == NULL)
        {
          // create only if this was a port announcement:
          int header;
          p >> header;

          if( header == NET_HEADER_CLIENT_ANNOUNCE)
          {
            // create !
            c = new NServerClient(remoteIp, remotePort);
            addClient(c);

            c->sendIdPing(socket);
          }else{
            std::cout << "ignoring msg (header " << header << ")" << std::endl;
          }
        }else{
          // ok, can rcv packet for the client:
          c->rcvPacket(p, socket);
        }
      }
    }while(repeat);

    // update all clients:
    std::map<int, NServerClient*>::iterator it;

    sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

		for (it=map_id.begin(); it!=map_id.end(); ++it)
		{
			NServerClient* c = it->second;

      if((now_time - c->last_rcv) > 10000)
      {
        // timed out
        std::cout << "timeout " << c->id << std::endl;

        removeClient(c);
      }

      c->update(socket);
		}

  }
};

struct NClient
{
  sf::UdpSocket socket;
  unsigned short destination_port = 54000;
  sf::IpAddress recipient = "localhost";

  unsigned short socket_port;

  NPacketRcvHandler np_rcv;
  NPacketSendHandler np_send;

  int id_on_server;

  // ------------------------------------- STATE MACHINE start
  int finite_state = 0;
  // 0 - initial
  sf::Int32 last_announce; // repeated announcement !
  int number_announcements = 0;

  // 1 - got answer from server -> id
  sf::Int32 last_rcv;

  // ------------------------------------- STATE MACHINE end

  NClient()
  {
    if(socket.bind(sf::Socket::AnyPort) != sf::Socket::Done)
    {
      std::cout << "could not bind socket!" << std::endl;
      return;
    }
    socket.setBlocking(false);
    socket_port = socket.getLocalPort();

    announcePort();

    last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

    std::cout << "Client Created. listen on " << socket_port << std::endl;

    np_send.enqueue(112, "hello world ! 1");
    np_send.enqueue(121, "hello world ! 2");
    np_send.enqueue(211, "hello world ! 3");
  }

  void draw(sf::RenderWindow &window)
  {
    DrawText(5,5, "Client Mode. connected to " + recipient.toString() + ":" + std::to_string(destination_port), 12, window, sf::Color(0,255,0));

    DrawText(5,20, "State: " + std::to_string(finite_state), 12, window, sf::Color(0,255,0));

    sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
    if( (now_time - last_rcv) > 500)
    {
      DrawText(5,35, "Time out? " + std::to_string( (float)(now_time - last_rcv)/1000.0), 12, window, sf::Color(255,0,0));
    }

    DrawText(5,50,
      "q: " + std::to_string(np_send.queue.size()) + ", w: " + std::to_string(np_send.map_sent.size())
       + np_send.missingToString(),
      12, window, sf::Color(255,255,255)
    );

    DrawText(5,65,
      "m: " + std::to_string(np_rcv.map_missing.size()) + ", h: " + std::to_string(np_rcv.highest_seq_num_rcvd) + ", cnt: " + std::to_string(np_rcv.packet_counter),
      12, window, sf::Color(0,255,255)
    );


  }

  void update()
  {
    bool repeat = false;
    do
    {
      sf::Packet p;
      sf::IpAddress remoteIp;
      unsigned short remotePort;
      auto status = socket.receive(p, remoteIp, remotePort);

      repeat = false;
      if(status == sf::Socket::Done)
      {
        last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();


        repeat = true;
        int header;
        p >> header;

        switch (header)
        {
          case NET_HEADER_ECHO:
            {
              std::string txt;
              p >> txt;

              std::cout << "IP: " << remoteIp << " : " << remotePort << std::endl;
              std::cout << "Echo: " << txt << std::endl;

              break;
            }
          case NET_HEADER_SERVERCLIENT_IDPING:
            {
              p >> id_on_server;

              std::cout << "IP: " << remoteIp << " : " << remotePort << std::endl;
              std::cout << "Id Ping: " << id_on_server << std::endl;

              if(finite_state == 0)
              {
                finite_state = 1;
              }

              break;
            }
          case NET_HEADER_PING:
            {
              // send pong back
              sendPong();
              break;
            }
          case NET_HEADER_PONG:
            {
              std::cout << "pong" << std::endl;
              break;
            }
          case NET_HEADER_NPacket:
            {
              np_rcv.incoming(p, np_send);
              break;
            }
          default:   // ------------ DEFAULT
            std::cout << "unreadable header: "<< header << std::endl;
            break;
        }
      }
    }
    while(repeat);

    // STATE MACHINE ITSELF
    switch(finite_state)
    {
      case 0:
      {
        sf::Int32 now_time = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

        if((now_time-last_announce) > 1000){
          announcePort();
          std::cout << "send announce Port" << std::endl;
        }
        break;
      }
      case 1:
      {
        np_send.update(np_rcv, socket, recipient, destination_port);

        break;
      }
      default:
      {
        std::cout << "undefined state: " << finite_state << std::endl;
        break;
      }
    }
  }

  void send(int header, const std::string &txt)
  {
    sf::Packet p;
    p << header << txt;

    if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void announcePort()
  {
    sf::Packet p;
    p << NET_HEADER_CLIENT_ANNOUNCE << socket_port;

    if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }

    number_announcements++;
    last_announce = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
  }

  void sendPing()
  {
    sf::Packet p;
    p << NET_HEADER_PING;

    if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void sendPong()
  {
    sf::Packet p;
    p << NET_HEADER_PONG;

    if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }
};


//-------------------------------------------------------- Main
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

    // --------------------------- Setup Networking

    if(NET_SERVER_MODE)
    {
      // server
      NET_SERVER = new NServer();
    }else{
      // client
      NET_CLIENT = new NClient();
    }

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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && window.hasFocus()){
          if(!NET_SERVER_MODE)
          {
            std::string txt = "hello " + std::to_string(rand());
            NET_CLIENT->np_send.enqueue(0, txt);
          }
        }

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

        window.display();
    }

    return 0;
}
