#pragma once

#include <deque>

#include "constants.h"
#include "util.h"
#include "world.h"

#include "messages_robocup_ssl_wrapper.pb.h"

class Tracker
{
  bool cameras_seen[MaxCameras];
  int num_cameras, num_cameras_seen;

  bool ready;

  World world;

  vector2f last_ball;

  void makeWorld();

  unsigned int frames;

public:
  struct Observation
  {
    bool valid;
    double time;
    float conf;
    vector2f loc;
    float angle;
  };

  struct ObjectTracker
  {
    static const int VEL_SAMPLES = 5;

    int affinity;
    Observation obs[MaxCameras];
    std::deque<Observation> history;

    ObjectTracker() : affinity(-1){};
    int mergeObservations();
    vector2f fitVelocity();
  };

  ObjectTracker robots[NumTeams][MaxRobotIds];
  ObjectTracker ball;

  Tracker() : frames(0), ready(false), last_ball(0, 0), num_cameras(0), num_cameras_seen(0)
  {
    for (bool &s : cameras_seen) {
      s = false;
    }
  }

  // returns whether a new referee message is available
  void updateVision(const SSL_DetectionFrame &d);

  bool isReady()
  {
    return ready;
  }
  bool getWorld(World &w)
  {
    if (ready) {
      w = world;
    }
    return ready;
  }
};
