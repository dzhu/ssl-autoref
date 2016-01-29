#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

#include "autoref.h"

#include "constants.h"
#include "messages_robocup_ssl_wrapper.pb.h"
#include "referee.pb.h"
#include "udp.h"

int main(int argc, char *argv[])
{
  puts("Autoref starting...");

  UDP vision_net;
  if (!vision_net.open(VisionGroup, VisionPort, true)) {
    puts("vision open failed!");
    exit(1);
  }

  UDP ref_net;
  if (!ref_net.open(RefGroup, RefPort, false)) {
    puts("ref open failed!");
    exit(1);
  }

  EventAutoref autoref;

  Address ref_addr(RefGroup, RefPort);
  SSL_WrapperPacket vision_msg;
  while (true) {
    if (vision_net.recv(vision_msg)) {
      if (vision_msg.has_detection()) {
        autoref.updateVision(vision_msg.detection());

        if (autoref.isReady()) {
          ref_net.send(autoref.makeRefereeMessage(), ref_addr);
        }
      }
      if (vision_msg.has_geometry()) {
        autoref.updateGeometry(vision_msg.geometry());
      }
    }
  }
}
