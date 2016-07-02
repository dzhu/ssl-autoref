#pragma once

#include <deque>

#include "constants.h"
#include "util.h"
#include "world.h"

#include "messages_robocup_ssl_wrapper.pb.h"

class Tracker
{
public:
  struct Observation
  {
    bool valid;
    int last_valid;
    double time;
    float conf;
    vector2f loc;
    float angle;

    Observation() : valid(false), last_valid(0), time(0), conf(0), loc(0, 0), angle(0) {}
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

private:
  bool cameras_seen[MaxCameras];
  int num_cameras, num_cameras_seen;

  bool ready;

  World world;

  void makeWorld();

  unsigned int frames;

  // Observation last_ball;

public:
  ObjectTracker robots[NumTeams][MaxRobotIds];
  ObjectTracker ball;

  Tracker() : frames(0), ready(false), num_cameras(0), num_cameras_seen(0)
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
