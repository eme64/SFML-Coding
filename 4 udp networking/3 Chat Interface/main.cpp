#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <cstring>

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

- request / send files -> eg. map
- chat

 --- completed:

 - package sequence wrap around -> infinite messages!
 - replaced sfml packages with my own packaging
*/

typedef uint16_t NPSeq;
const NPSeq NPSeq_HALF = 32768;
const NPSeq NPSeq_START_AT = 32768 + 32750;
/*
uint16_t - 32768
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
// ------------------------------------------------------- Input
struct InputHandler
{
  static std::queue<char> textInput;

  static void push(char c);

  static void clearText();

  static bool updateString(std::string &s);
};

std::queue<char> InputHandler::textInput;

void InputHandler::push(char c)
{
  InputHandler::textInput.push(c);
}

void InputHandler::clearText()
{
  InputHandler::textInput = std::queue<char>();
}

bool InputHandler::updateString(std::string &s)
{
  // returns true if enter hit. rest remains in queue
  while(!InputHandler::textInput.empty())
  {
    char c = InputHandler::textInput.front();
    InputHandler::textInput.pop();

    switch(c)
    {
      case 8: // Return
        if(s.size()>0)
        {
          s.pop_back();
        }
        break;
      case 13: // Enter

        return true;
        break;
      default:
        s += c;
        break;
    }
  }
  return false;
}
// ------------------------------------------------------- Networking
// just send a string
const uint16_t NET_HEADER_ECHO = 100;

const uint16_t NET_HEADER_PING = 200;
const uint16_t NET_HEADER_PONG = 201;

// announce the recieve port of client
const uint16_t NET_HEADER_CLIENT_ANNOUNCE = 2000;
// server id ping -> as first message over client channel
const uint16_t NET_HEADER_SERVERCLIENT_IDPING = 3000;

// the NPacket
const uint16_t NET_HEADER_NPacket = 4000;

// ---------------- UDP Packets:
struct NUDPWritePacket
{
  std::vector<char> data;
  size_t index = 0; // where to write next, may be beyond memory

  bool send(sf::UdpSocket &socket, sf::IpAddress ip, unsigned short port)
  {
    return (socket.send(data.data(), index, ip, port) == sf::Socket::Done);
  }
};

NUDPWritePacket& operator << (NUDPWritePacket& p, const char& c){
  p.data.resize(p.index+1);
  p.data[p.index] = c;
  p.index++;

  return p;
}

NUDPWritePacket& operator << (NUDPWritePacket& p, const uint16_t& c){
  char b1 = c & 0xFF;
  char b2 = c >> 8;

  p.data.resize(p.index+2);
  p.data[p.index] = b1;
  p.data[p.index+1] = b2;
  p.index+=2;

  return p;
}

NUDPWritePacket& operator << (NUDPWritePacket& p, const std::string& c){

  uint16_t size = c.size();

  p << size;

  p.data.resize(p.index + size);
  std::memcpy(&(p.data[p.index]), c.data(), c.size());
  p.index+=size;

  return p;
}


struct NUDPReadPacket
{
  static const size_t rcv_buffer_size = sf::UdpSocket::MaxDatagramSize;
  static char rcv_buffer[NUDPReadPacket::rcv_buffer_size];

  std::vector<char> data;
  size_t size = 0;
  size_t index = 0; // where to read next

  bool receive(sf::UdpSocket &socket, sf::IpAddress &ip, unsigned short& port);
  bool endOfPacket();
};
char NUDPReadPacket::rcv_buffer[NUDPReadPacket::rcv_buffer_size];
bool NUDPReadPacket::receive(sf::UdpSocket &socket, sf::IpAddress &ip, unsigned short& port)
{
  // returns true if successful
  auto status = socket.receive(
    NUDPReadPacket::rcv_buffer, NUDPReadPacket::rcv_buffer_size,
    size, ip, port
  );

  if(status == sf::Socket::Done)
  {

    // success:
    data.reserve(size);
    std::memcpy(data.data(), rcv_buffer, size);

    return true;
  }else{
    // failiure:

    return false;
  }
}

bool NUDPReadPacket::endOfPacket()
{
  return (index >= size);
}


NUDPReadPacket& operator >> (NUDPReadPacket& p, char& c){
  c = p.data[p.index];
  p.index++;

  return p;
}

NUDPReadPacket& operator >> (NUDPReadPacket& p, uint16_t& c){
  uint16_t b1 = p.data[p.index];
  uint16_t b2 = p.data[p.index+1];
  p.index+=2;

  c = ((uint16_t(b1)& 0xFF) | ((uint16_t(b2) & 0xFF) << 8));
  return p;
}

NUDPReadPacket& operator >> (NUDPReadPacket& p, std::string& c){

  uint16_t size;
  p >> size;

  std::string cnew( &(p.data[p.index]), size);

  c = cnew;

  p.index+=size;

  return p;
}

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

const uint16_t NPacket_TAG_CHAT = 1;

inline bool sequence_greater_than( NPSeq s1, NPSeq s2 )
{
  return ( ( s1 > s2 ) && ( s1 - s2 <= NPSeq_HALF ) ) ||
  ( ( s1 < s2 ) && ( s2 - s1  > NPSeq_HALF ) );
}

struct NPacket
{
  NPSeq seq;
  uint16_t tag;
  std::string str;

  // book keeping:
  sf::Int32 time_sent; // for timeout and resend
};

struct NPacketRcvHandler
{
  NPSeq highest_seq_num_rcvd = NPSeq_START_AT;

  uint16_t packet_counter = 0;

  std::map<NPSeq, NPSeq> map_missing;


  void createAckMessage(NUDPWritePacket &p);
  bool hasNews; // if the ack situation changed since last createAckMessage

  void incoming(NUDPReadPacket &p, NPacketSendHandler &np_send);
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

  int enqueue(uint16_t tag, const std::string &txt);
  void update(NPacketRcvHandler &np_rcv, sf::UdpSocket &socket, const sf::IpAddress &ip, const unsigned short &port);
  void readAcks(NUDPReadPacket &p);

  std::string missingToString();
};

int NPacketSendHandler::enqueue(uint16_t tag, const std::string &txt)
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
    NUDPWritePacket p;
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
    while(queue.size() > 0 && map_sent.size() < num_wait_ack_limit && p.index < 50000)
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
    if(!p.send(socket,ip, port))//socket.send(p, ip, port) != sf::Socket::Done)
    {
        std::cout << "Error on send ! (NPacket)" << std::endl;
    }
  }
}

void NPacketSendHandler::readAcks(NUDPReadPacket &p)
{
  NPSeq highest;
  uint16_t num_missing;

  p >> highest >> num_missing;

  std::vector<NPSeq> missingVec;
  missingVec.reserve(num_missing+1);
  std::vector<NPSeq> missingWrapVec;
  missingWrapVec.reserve(num_missing);

  for(uint16_t i = 0; i<num_missing; i++)
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

void NPacketRcvHandler::createAckMessage(NUDPWritePacket &p)
{
  hasNews = false;

  p << highest_seq_num_rcvd;
  p << (uint16_t)(map_missing.size());

  // send all that are missing:
  std::map<NPSeq, NPSeq>::iterator it;
	for (it=map_missing.begin(); it!=map_missing.end(); ++it)
	{
		p << (it->second);
  }
}

void NPacketRcvHandler::incoming(NUDPReadPacket &p, NPacketSendHandler &np_send)
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
  uint16_t id;
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

  void sendPacket(NUDPWritePacket &p, sf::UdpSocket &socket)
  {
    if(!p.send(socket,remoteIp, remotePort)) //(socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void sendIdPing(sf::UdpSocket &socket)
  {
    NUDPWritePacket p;

    p << NET_HEADER_SERVERCLIENT_IDPING << id;
    if(!p.send(socket,remoteIp, remotePort))//if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void sendPing(sf::UdpSocket &socket)
  {
    NUDPWritePacket p;
    p << NET_HEADER_PING;
    if(!p.send(socket,remoteIp, remotePort))//if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void sendPong(sf::UdpSocket &socket)
  {
    NUDPWritePacket p;
    p << NET_HEADER_PONG;
    if(!p.send(socket,remoteIp, remotePort))//if (socket.send(p, remoteIp, remotePort) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void rcvPacket(NUDPReadPacket &p, sf::UdpSocket &socket)
  {
    last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();

    uint16_t header;
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
      NUDPReadPacket p;
      sf::IpAddress remoteIp;
      unsigned short remotePort;

      bool success = p.receive(socket, remoteIp, remotePort);
      //auto status = socket.receive(p, remoteIp, remotePort);

      repeat = false;
      if(success)
      {
        repeat = true;

        // find client:
        NServerClient* c = getClient(remoteIp, remotePort);
        if(c == NULL)
        {
          // create only if this was a port announcement:
          uint16_t header;
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

  uint16_t id_on_server;

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
      NUDPReadPacket p;
      sf::IpAddress remoteIp;
      unsigned short remotePort;

      bool success = p.receive(socket, remoteIp, remotePort);
      //auto status = socket.receive(p, remoteIp, remotePort);

      repeat = false;
      if(success)
      {
        last_rcv = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();


        repeat = true;
        uint16_t header;
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

  /*
  void send(uint16_t header, const std::string &txt)
  {
    NUDPWritePacket p;
    p << header << txt;

    if( !p.send(socket, recipient, destination_port))//if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }
  */

  void announcePort()
  {
    NUDPWritePacket p;
    p << NET_HEADER_CLIENT_ANNOUNCE << socket_port;

    if( !p.send(socket, recipient, destination_port))//if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }

    number_announcements++;
    last_announce = PROGRAM_CLOCK.getElapsedTime().asMilliseconds();
  }

  void sendPing()
  {
    NUDPWritePacket p;
    p << NET_HEADER_PING;

    if( !p.send(socket, recipient, destination_port))//if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }

  void sendPong()
  {
    NUDPWritePacket p;
    p << NET_HEADER_PONG;

    if( !p.send(socket, recipient, destination_port))//if (socket.send(p, recipient, destination_port) != sf::Socket::Done)
    {
        std::cout << "Error on send !" << std::endl;
    }
  }
};

//-------------------------------------------------------- CHAT
struct EChat
{
  std::list<std::string> lines;
  size_t num_lines_max = 20;
  std::string inputLine;

  size_t blinker = 0;

  bool isActive = true; // true -> catches input, draw more opaque

  void draw(float x, float y, sf::RenderWindow &window)
  {
    blinker++;
    std::string blinker_txt = "";
    if(blinker > 30)
    {
      blinker = 0;
    }else if(blinker > 10)
    {
      blinker_txt = "|";
    }
    DrawText(x+5,y-20, ">" + inputLine + blinker_txt, 12, window, sf::Color(255,255,255));

    float y_pos = y-40;
    for(std::string l:lines)
    {
      DrawText(x+5,y_pos, l, 12, window, sf::Color(200,200,200));
      y_pos -= 15;
    }
  }

  void push(std::string line)
  {
    lines.push_front(line);
    while(lines.size() > num_lines_max)
    {
      lines.pop_back();
    }
  }

  std::string update()
  {
    // returns string if enter pressed
    std::string ret = "";
    if(isActive)
    {
      if(InputHandler::updateString(inputLine))
      {
        //push(inputLine);
        ret = inputLine;
        inputLine = "";
      }
    }
    return ret;
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

    std::string textInput;

    // --------------------------- EChat
    EChat chat;

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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && window.hasFocus()){
          if(!NET_SERVER_MODE)
          {
            std::string txt = "hello " + std::to_string(rand());
            NET_CLIENT->np_send.enqueue(0, txt);
          }
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

        std::string cl = chat.update();
        if(cl != "")
        {
            chat.push(cl);
        }
        chat.draw(0,600, window);

        window.display();
    }

    return 0;
}
