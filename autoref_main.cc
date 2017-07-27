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
#include "ssl_referee.pb.h"
#include "udp.h"

enum OptionIndex
{
  UNKNOWN,
  HELP,
  VERBOSE,
  FULL,
  REMOTE,
};

const option::Descriptor options[] = {
  {UNKNOWN, 0, "", "", option::Arg::None, "An automatic referee for the SSL."},             //
  {HELP, 0, "h", "help", option::Arg::None, "-h, --help: print help"},                      //
  {VERBOSE, 0, "v", "verbose", option::Arg::None, "-v: verbose"},                           //
  {FULL, 0, "f", "full", option::Arg::None, "--full: run using full game logic"},           //
  {REMOTE, 0, "r", "remote", option::Arg::None, "--remote: send refbox control messages"},  //
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

  UDP autoref_net;
  if (!autoref_net.open("", 0, false)) {
    puts("Referee port open failed!");
    exit(1);
  }

  bool use_rcon = args[REMOTE] != nullptr;
  RemoteClient rcon;
  bool rcon_opened = false;
  if (use_rcon) {
    if (rcon.open("localhost", 10007)) {
      puts("Remote client opened!");
      rcon_opened = true;
    }
    else {
      puts("Remote client port open failed!");
    }
  }

  bool verbose = (args[VERBOSE] != nullptr);
  BaseAutoref *autoref;
  if (args[FULL]) {
    puts("Starting full autoref.");
    autoref = new Autoref(verbose);
  }
  else {
    puts("Starting evaluation autoref.");
    autoref = new EvaluationAutoref(verbose);
  }

  Address autoref_addr(AutorefGroup, AutorefPort);

  SSL_WrapperPacket vision_msg;
  SSL_Referee ref_msg;

  fd_set read_fds;
  int n_fds = std::max(vision_net.getFd(), ref_net.getFd()) + 1;

  bool got_vision = false, got_ref = false;

  puts("\nWaiting for network packets...");

  while (true) {
    FD_ZERO(&read_fds);
    FD_SET(vision_net.getFd(), &read_fds);
    FD_SET(ref_net.getFd(), &read_fds);
    select(n_fds, &read_fds, nullptr, nullptr, nullptr);

    if (FD_ISSET(vision_net.getFd(), &read_fds) && vision_net.recv(vision_msg)) {
      if (!got_vision) {
        got_vision = true;
        puts("Got vision packet!");
      }

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
        if (autoref->isMessageReady()) {
          {
            const auto &a = autoref->getUpdate();
            printf("has timestamp: %d\n", a.has_game_timestamp());
            if (a.has_game_timestamp()) {
              printf("timestamp has: %d %d\n",
                     a.game_timestamp().has_game_stage(),
                     a.game_timestamp().has_stage_time_left());
            }
          }

          autoref_net.send(autoref->getUpdate(), autoref_addr);
        }
      }
      if (vision_msg.has_geometry()) {
        autoref->updateGeometry(vision_msg.geometry());
      }
    }
    if (FD_ISSET(ref_net.getFd(), &read_fds) && ref_net.recv(ref_msg)) {
      if (!got_ref) {
        got_ref = true;
        puts("Got ref packet!");
      }
      autoref->updateReferee(ref_msg);
    }
  }
}
