#pragma once

#include <stdint.h>

#include <vector>

#include "gvector.h"

using namespace std;

struct WorldRobot
{
  float conf;

  uint8_t team, robot_id;

  float angle;
  vector2f loc, vel;

  bool visible() const
  {
    return conf > .1;
  }

  WorldRobot() : conf(0), team(0), robot_id(0), angle(0), loc(0, 0), vel(0, 0)
  {
  }
};

struct WorldBall
{
  float conf;

  vector2f loc, vel;
  bool visible() const
  {
    return conf > .1;
  }
};

struct World
{
  double time;

  vector<WorldRobot> robots;
  WorldBall ball;

  void reset()
  {
    time = 0;
    robots.clear();
    ball.conf = 0;
  }
};
