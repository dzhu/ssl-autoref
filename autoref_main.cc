#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

#include <sys/select.h>

#include "autoref.h"

#include "constants.h"
#include "messages_robocup_ssl_wrapper.pb.h"
#include "rconclient.h"
#include "referee.pb.h"
#include "udp.h"

int main(int argc, char *argv[])
{
  puts("Autoref starting...");

  UDP vision_net;
  if (!vision_net.open(VisionGroup, VisionPort, true)) {
    puts("SSL-Vision port open failed!");
    exit(1);
  }

  UDP ref_net;
  if (!ref_net.open(RefGroup, RefPort, false)) {
    puts("Referee port open failed!");
    exit(1);
  }

  RemoteClient rcon;
  if (!rcon.open("localhost", 10007)) {
    puts("Remote client port open failed!");
    exit(1);
  }

  EventAutoref autoref;

  // Address ref_addr(RefGroup, RefPort);

  SSL_WrapperPacket vision_msg;
  SSL_Referee ref_msg;

  fd_set read_fds;
  int n_fds = std::max(vision_net.getFd(), ref_net.getFd()) + 1;

  while (true) {
    FD_ZERO(&read_fds);
    FD_SET(vision_net.getFd(), &read_fds);
    FD_SET(ref_net.getFd(), &read_fds);
    select(n_fds, &read_fds, nullptr, nullptr, nullptr);

    if (vision_net.recv(vision_msg)) {
      if (vision_msg.has_detection()) {
        autoref.updateVision(vision_msg.detection());

        //// currently, we're not sending actual referee messages
        // if (autoref.isMessageReady()) {
        //   ref_net.send(autoref.makeMessage(), ref_addr);
        // }

        if (autoref.isRemoteReady()) {
          rcon.sendRequest(autoref.makeRemote());
        }
      }
      if (vision_msg.has_geometry()) {
        autoref.updateGeometry(vision_msg.geometry());
      }
    }
    if (ref_net.recv(ref_msg)) {
      autoref.updateReferee(ref_msg);
    }
  }
}
