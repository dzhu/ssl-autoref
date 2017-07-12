#pragma once

#include <cstdint>

#include <algorithm>
#include <deque>
#include <map>

#include <google/protobuf/text_format.h>

#include "messages_robocup_ssl_wrapper.pb.h"
#include "rcon.pb.h"
#include "ssl_referee.pb.h"
#include "ssl_autoref.pb.h"

#include "constants.h"
#include "events.h"
#include "touches.h"
#include "tracker.h"

using namespace google::protobuf;

class BaseAutoref
{
protected:
  bool have_geometry, new_refbox;
  SSL_GeometryData geometry;
  SSL_Referee refbox_message;

  bool game_on;

  friend class AutorefEvent;
  std::vector<AutorefEvent *> events;
  std::map<const char *, AutorefEvent *> event_map;

  vector2f ball_reset_loc;

  uint32_t cmd_counter;
  uint64_t cmd_timestamp;

  AutorefVariables vars;

  ssl::SSL_Autoref message;
  bool message_ready;

  template <typename E>
  void addEvent()
  {
    E *ev = new E(this);
    event_map[&E::ID] = ev;
    events.push_back(ev);
  }

  bool new_stage, new_cmd;

  bool state_updated;

  virtual bool doEvents(const World &w, bool ball_z_valid = false, float ball_z = 0) = 0;

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
    return (E *)event_map.at(&E::ID);
  }

  const SSL_Referee &getRefboxMessage()
  {
    return refbox_message;
  }

  BaseAutoref();
  bool isMessageReady();
  bool isRemoteReady();

  SSL_Referee makeMessage();
  SSL_RefereeRemoteControlRequest makeRemote();
  const ssl::SSL_Autoref &getUpdate();

  void updateGeometry(const SSL_GeometryData &g);
  void updateVision(const SSL_DetectionFrame &d);
  void updateReferee(const SSL_Referee &r);

  AutorefVariables getState()
  {
    return vars;
  }

  // after a call to updateVision, this should return true if something has
  // happened
  bool isStateUpdated()
  {
    return state_updated;
  }

  template<typename Fun>
  void forEachEvent(Fun f){
    std::for_each(events.begin(), events.end(), f);
  }
};
