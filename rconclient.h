#include <netdb.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "rcon.pb.h"

#define MAX_REPLY_LENGTH 4096

class RemoteClient
{
  int sock;

  bool recvFully(void *buffer, std::size_t length);
  bool sendFully(const void *buffer, std::size_t length);

  uint32_t nextMessageID;

  SSL_RefereeRemoteControlRequest createMessage();

public:
  RemoteClient() : sock(0){};
  bool open(const char *hostname, int port);

  void sendRequest(const SSL_RefereeRemoteControlRequest &request);

  void sendCard(SSL_RefereeRemoteControlRequest::CardInfo::CardType color,
                SSL_RefereeRemoteControlRequest::CardInfo::CardTeam team);
  void sendStage(SSL_Referee::Stage stage);
  void sendCommand(SSL_Referee::Command command);
};
