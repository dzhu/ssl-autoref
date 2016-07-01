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

// TODO set last id field

#include "rconclient.h"

bool RemoteClient::recvFully(void *buffer, std::size_t length)
{
  char *ptr = static_cast<char *>(buffer);
  while (length != 0) {
    ssize_t ret = recv(sock, ptr, length, 0);
    if (ret < 0) {
      std::cerr << std::strerror(errno) << '\n';
      return false;
    }
    if (!ret) {
      std::cerr << "Socket closed by remote peer.\n";
      return false;
    }
    ptr += ret;
    length -= ret;
  }
  return true;
}

bool RemoteClient::sendFully(const void *buffer, std::size_t length)
{
  const char *ptr = static_cast<const char *>(buffer);
  while (length != 0) {
    ssize_t ret = send(sock, ptr, length, 0);
    if (ret < 0) {
      std::cerr << std::strerror(errno) << '\n';
      return false;
    }
    ptr += ret;
    length -= ret;
  }
  return true;
}

SSL_RefereeRemoteControlRequest RemoteClient::createMessage()
{
  SSL_RefereeRemoteControlRequest request;
  request.set_message_id(nextMessageID++);
  return request;
}

bool RemoteClient::sendRequest(const SSL_RefereeRemoteControlRequest &request)
{
  // send request
  {
    const std::string &message = request.SerializeAsString();
    uint32_t messageLength = htonl(static_cast<uint32_t>(message.size()));
    // std::cout << "Send " << (sizeof(messageLength) + message.size()) << " bytes: ";
    // std::cout.flush();
    if (request.has_command()) {
      std::cout << "Sending command: " << SSL_Referee::Command_Name(request.command()) << ".\n";
    }
    if (request.has_stage()) {
      std::cout << "Sending stage: " << SSL_Referee::Stage_Name(request.stage()) << ".\n";
    }
    bool ret = true;
    ret &= sendFully(&messageLength, sizeof(messageLength));
    ret &= sendFully(message.data(), message.size());
    if (!ret) {
      return false;
    }
    // std::cout << "OK\n";
  }

  // wait for response
  SSL_RefereeRemoteControlReply reply;
  {
    // std::cout << "Receive reply: ";
    // std::cout.flush();
    uint32_t replyLength;
    recvFully(&replyLength, sizeof(replyLength));
    replyLength = ntohl(replyLength);
    if (replyLength > MAX_REPLY_LENGTH) {
      std::cerr << "Got reply length " << replyLength << " which is greater than limit " << MAX_REPLY_LENGTH << ".\n";
    }
    std::vector<char> buffer(replyLength);
    recvFully(&buffer[0], replyLength);
    // std::cout << (4 + replyLength) << " bytes OK.\n";
    reply.ParseFromArray(&buffer[0], replyLength);
  }
  if (reply.message_id() != request.message_id()) {
    std::cerr << "Reply message ID " << reply.message_id() << " does not match request message ID "
              << request.message_id() << ".\n";
  }
  std::cout << "Command result is: " << SSL_RefereeRemoteControlReply::Outcome_Name(reply.outcome()) << ".\n";

  return true;
}

bool RemoteClient::open(const char *hostname, int port)
{
  // Parse target address.
  struct addrinfo *refboxAddresses;
  {
    struct addrinfo hints;
    hints.ai_flags = 0;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    int err = getaddrinfo(hostname, nullptr, &hints, &refboxAddresses);
    if (err != 0) {
      std::cerr << gai_strerror(err) << '\n';
    }
  }

  // Try to connect to the returned addresses until one of them succeeds.
  sock = -1;
  for (const addrinfo *i = refboxAddresses; i; i = i->ai_next) {
    char host_buf[256], port_buf[32];
    (reinterpret_cast<struct sockaddr_in *>(i->ai_addr))->sin_port = htons(port);
    int err = getnameinfo(i->ai_addr,
                          i->ai_addrlen,
                          host_buf,
                          sizeof(host_buf),
                          port_buf,
                          sizeof(port_buf),
                          NI_NUMERICHOST | NI_NUMERICSERV);
    if (err != 0) {
      std::cerr << gai_strerror(err) << '\n';
      return false;
    }
    std::cout << "Trying host " << host_buf << " port " << port_buf << ": ";
    std::cout.flush();
    sock = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
    if (sock < 0) {
      std::cout << std::strerror(errno) << '\n';
      continue;
    }
    if (connect(sock, i->ai_addr, i->ai_addrlen) < 0) {
      std::cout << std::strerror(errno) << '\n';
      close(sock);
      sock = -1;
      continue;
    }
    std::cout << "OK\n";
    return true;
  }
  return false;
}

bool RemoteClient::sendCard(SSL_RefereeRemoteControlRequest::CardInfo::CardType color,
                            SSL_RefereeRemoteControlRequest::CardInfo::CardTeam team)
{
  SSL_RefereeRemoteControlRequest request = createMessage();
  request.mutable_card()->set_type(color);
  request.mutable_card()->set_team(team);
  return sendRequest(request);
}

bool RemoteClient::sendStage(SSL_Referee::Stage stage)
{
  SSL_RefereeRemoteControlRequest request = createMessage();
  request.set_stage(stage);
  return sendRequest(request);
}

bool RemoteClient::sendCommand(SSL_Referee::Command command)
{
  SSL_RefereeRemoteControlRequest request = createMessage();
  request.set_command(command);
  return sendRequest(request);
}
