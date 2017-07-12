#include "udp.h"

//====================================================================//
//  Net::Address: Network address class
//====================================================================//

bool Address::setHost(const char *hostname, int port)
{
  addrinfo *res = nullptr;
  getaddrinfo(hostname, nullptr, nullptr, &res);
  if (res == nullptr) {
    return false;
  }

  memset(&addr, 0, sizeof(addr));
  addr_len = res->ai_addrlen;
  memcpy(&addr, res->ai_addr, addr_len);
  freeaddrinfo(res);

  // set port for internet sockets
  sockaddr_in *sockname = reinterpret_cast<sockaddr_in *>(&addr);
  if (sockname->sin_family == AF_INET) {
    sockname->sin_port = htons(port);
  }
  else {
    // TODO: any way to set port in general?
  }

  return true;
}

void Address::setAny(int port)
{
  memset(&addr, 0, sizeof(addr));
  sockaddr_in *s = reinterpret_cast<sockaddr_in *>(&addr);
  s->sin_addr.s_addr = htonl(INADDR_ANY);
  s->sin_port = htons(port);
  addr_len = sizeof(sockaddr_in);
}

in_addr_t Address::getInAddr() const
{
  const sockaddr_in *s = reinterpret_cast<const sockaddr_in *>(&addr);
  return s->sin_addr.s_addr;
}

//====================================================================//
//  Net::UDP: Simple raw UDP messaging
//====================================================================//

bool UDP::open(const char *server_host, int port, bool blocking)
{
  // open the socket
  if (fd >= 0) {
    ::close(fd);
  }
  fd = socket(PF_INET, SOCK_DGRAM, 0);

  // set socket as non-blocking
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    flags = 0;
  }
  fcntl(fd, F_SETFL, flags | (blocking ? 0 : O_NONBLOCK));

  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse), sizeof(reuse)) != 0) {
    fprintf(stderr, "ERROR WHEN SETTING SO_REUSEADDR ON UDP SOCKET\n");
    fflush(stderr);
  }

  socklen_t len = 0;
  int yes = 1;
  getsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast<char *>(&yes), &len);

  // allow packets to be received on this host
  if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast<const char *>(&yes), sizeof(yes)) != 0) {
    fprintf(stderr, "ERROR WHEN SETTING IP_MULTICAST_LOOP ON UDP SOCKET\n");
    fflush(stderr);
  }

  // bind socket to port if nonzero
  if (port != 0) {
    sockaddr_in sockname;
    sockname.sin_family = AF_INET;
    sockname.sin_addr.s_addr = htonl(INADDR_ANY);
    sockname.sin_port = htons(port);
    if (bind(fd, reinterpret_cast<struct sockaddr *>(&sockname), sizeof(sockname)) != 0) {
      fprintf(stderr, "ERROR ON BINDING ON UDP SOCKET: %s\n", strerror(errno));
      fflush(stderr);
      return false;
    }
  }

  if (strlen(server_host) == 0) {
    return true;
  }

  // add UDP multicast groups
  Address multiaddr, interface;
  multiaddr.setHost(server_host, port);
  interface.setAny();

  return addMulticast(multiaddr, interface);
}

bool UDP::addMulticast(const Address &multiaddr, const Address &interface)
{
  struct ip_mreq imreq;
  imreq.imr_multiaddr.s_addr = multiaddr.getInAddr();
  imreq.imr_interface.s_addr = interface.getInAddr();

  int ret = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imreq, sizeof(imreq));

  return ret == 0;
}

void UDP::close()
{
  if (fd >= 0) {
    ::close(fd);
  }
  fd = -1;

  sent_packets = 0;
  sent_bytes = 0;
  recv_packets = 0;
  recv_bytes = 0;
}

bool UDP::send(const void *data, int length, const Address &dest)
{
  int len = sendto(fd, data, length, 0, &dest.addr, dest.addr_len);

  if (len > 0) {
    sent_packets++;
    sent_bytes += len;
  }

  return len == length;
}

bool UDP::send(const Message &packet, const Address &dest)
{
  string s = packet.SerializeAsString();
  return send(s.data(), s.size(), dest);
}

int UDP::recv(Address &src)
{
  src.addr_len = sizeof(src.addr);
  int len = recvfrom(fd, buf, MaxDataGramSize, 0, &src.addr, &src.addr_len);

  if (len > 0) {
    recv_packets++;
    recv_bytes += len;
  }

  return len;
}

bool UDP::recv(Message &packet)
{
  Address addr;
  int n = recv(addr);
  if (n <= 0) {
    return false;
  }
  return packet.ParsePartialFromArray(buf, n);
}

bool UDP::havePendingData() const
{
  return wait(0);
}

bool UDP::wait(int timeout_ms) const
{
  static const bool debug = false;
  static bool pendingData = false;
  pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;

  bool success = (poll(&pfd, 1, timeout_ms) > 0);

  if (!success) {
    // Poll now claims that there is no pending data.
    // What did havePendingData get from Poll most recently?
    if (debug) {
      printf("wait failed, havePendingData=%s\n", (pendingData ? "true" : "false"));
    }
  }
  pendingData = success;
  return success;
}
