#pragma once
#include <cstdint>

#include <deque>
#include <map>

#include <google/protobuf/text_format.h>

#include "messages_robocup_ssl_wrapper.pb.h"
#include "rcon.pb.h"
#include "referee.pb.h"

#include "constants.h"
#include "events.h"
#include "touches.h"
#include "tracker.h"

using namespace std;
using namespace google::protobuf;

class EventAutoref
{
  bool have_geometry, have_refbox;
  SSL_GeometryData geometry;
  SSL_Referee refbox_message;

  bool game_on;

  friend class AutorefEvent;
  std::vector<AutorefEvent *> events;
  std::map<const char *, AutorefEvent *> event_map;

  vector2f ball_reset_loc;

  uint32_t cmd_counter;
  uint64_t cmd_timestamp;

  bool doEvents(const World &w, bool ball_z_valid = false, float ball_z = 0);

  AutorefVariables vars;

  template <typename E>
  void addEvent()
  {
    E *ev = new E(this);
    event_map[&E::ID] = ev;
    events.push_back(ev);
  }

  bool new_stage, new_cmd;

public:
  Tracker tracker;

  // timestamp from the last received world state
  double last_time;

  // when waiting for robots to stop moving, check positions each time
  // this many frames pass
  int CheckInterval;

  // when waiting for robots to stop moving, if we have waited this many
  // seconds, then just assume they're ready
  int StopMovingTimeout;

  // declare that a kick has happened if it moves this distance
  double FreeKickDistanceThreshold;

  // Number of games to play, after which autoref sets Halt
  // if NumGames <=0 then games continue indefinitely
  int NumGames;

  // Warp ball, or drag it?
  bool WarpBall;

  int NumBlueRobots, NumYellowRobots;

  template <typename E>
  E *getEvent() const
  {
    return (E *)event_map[&E::ID];
  }

  EventAutoref();
  bool isMessageReady();
  bool isRemoteReady();

  SSL_Referee makeMessage();
  SSL_RefereeRemoteControlRequest makeRemote();

  void updateGeometry(const SSL_GeometryData &g);
  void updateVision(const SSL_DetectionFrame &d);
  void updateReferee(const SSL_Referee &r);
};
