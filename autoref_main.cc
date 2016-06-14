#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

#include <sys/select.h>

#include "autoref.h"
#include "base_ref.h"
#include "eval_ref.h"

#include "constants.h"
#include "messages_robocup_ssl_wrapper.pb.h"
#include "optionparser.h"
#include "rconclient.h"
#include "referee.pb.h"
#include "udp.h"

enum OptionIndex
{
  UNKNOWN,
  HELP,
  VERBOSE,
  EVAL,
};

const option::Descriptor options[] = {
  {UNKNOWN, 0, "", "", option::Arg::None, "An automatic referee for the SSL."},    //
  {HELP, 0, "h", "help", option::Arg::None, "-h, --help: print help"},             //
  {VERBOSE, 0, "v", "verbose", option::Arg::None, "-v: verbose"},                  //
  {EVAL, 0, "", "eval", option::Arg::None, "--eval: run using evaluation logic"},  //
  {0, 0, nullptr, nullptr, nullptr, nullptr},
};

int main(int argc, char *argv[])
{
  argc -= (argc > 0);
  argv += (argc > 0);  // skip program name argv[0] if present
  option::Stats stats(options, argc, argv);
  std::vector<option::Option> args(stats.options_max);
  std::vector<option::Option> buffer(stats.buffer_max);
  option::Parser parse(options, argc, argv, &args[0], &buffer[0]);

  if (parse.error() || args[HELP] != nullptr || args[UNKNOWN] != nullptr) {
    option::printUsage(std::cout, options);
    return 0;
  }

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
  bool rcon_opened = true;
  if (rcon.open("localhost", 10007)) {
    puts("Remote client opened!");
  }
  else {
    rcon_opened = false;
    puts("Remote client port open failed!");
  }

  bool verbose = (args[VERBOSE] != nullptr);
  BaseAutoref *autoref;
  if (args[EVAL]) {
    puts("Starting evaluation autoref.");
    autoref = new EvaluationAutoref(verbose);
  }
  else {
    puts("Starting full autoref.");
    autoref = new Autoref(verbose);
  }

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
        autoref->updateVision(vision_msg.detection());

        //// currently, we're not sending actual referee messages
        // if (autoref->isMessageReady()) {
        //   ref_net.send(autoref->makeMessage(), ref_addr);
        // }

        if (autoref->isRemoteReady()) {
          if (rcon_opened) {
            rcon.sendRequest(autoref->makeRemote());
          }
        }
      }
      if (vision_msg.has_geometry()) {
        autoref->updateGeometry(vision_msg.geometry());
      }
    }
    if (ref_net.recv(ref_msg)) {
      autoref->updateReferee(ref_msg);
    }
  }
}
