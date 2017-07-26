#pragma once

#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "google/protobuf/message.h"
using namespace google::protobuf;

static const int MaxDataGramSize = 65536;

static const int PORT_OFFSET =
#include "PORT_OFFSET"
  ;

static const char *VisionGroup = "224.5.23.2";
static const int VisionPort = 10005 + PORT_OFFSET;

static const char *RefGroup = "224.5.23.1";
static const int RefPort = 10003 + PORT_OFFSET;

static const char *AutorefGroup = "224.5.23.3";
static const int AutorefPort = 10008 + PORT_OFFSET;

class Address
{
  sockaddr addr;
  socklen_t addr_len;

public:
  Address()
  {
    memset(&addr, 0, sizeof(addr));
    addr_len = 0;
  }
  Address(const char *hostname, int port)
  {
    setHost(hostname, port);
  }
  Address(const Address &src)
  {
    copy(src);
  }
  ~Address()
  {
    reset();
  }

  bool setHost(const char *hostname, int port);
  void setAny(int port = 0);

  bool operator==(const Address &a) const
  {
    return addr_len == a.addr_len && memcmp(&addr, &a.addr, addr_len) == 0;
  }
  void copy(const Address &src)
  {
    memcpy(&addr, &src.addr, src.addr_len);
    addr_len = src.addr_len;
  }
  void reset()
  {
    memset(&addr, 0, sizeof(addr));
    addr_len = 0;
  }
  void clear()
  {
    reset();
  }

  in_addr_t getInAddr() const;

  friend class UDP;
};

class UDP
{
  char buf[MaxDataGramSize];
  int fd;

public:
  unsigned sent_packets;
  unsigned sent_bytes;
  unsigned recv_packets;
  unsigned recv_bytes;

public:
  UDP()
  {
    fd = -1;
    close();
  }
  ~UDP()
  {
    close();
  }

  bool open(const char *server_host, int port, bool blocking);
  bool addMulticast(const Address &multiaddr, const Address &interface);
  void close();
  bool isOpen() const
  {
    return fd >= 0;
  }

  bool send(const void *data, int length, const Address &dest);
  bool send(const Message &packet, const Address &dest);

  int recv(Address &src);
  bool recv(Message &packet);

  bool wait(int timeout_ms = -1) const;
  bool havePendingData() const;

  int getFd() const
  {
    return fd;
  }
};
