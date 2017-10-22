#pragma once

#include <cstdint>
#include <vector>

#include "constants.h"
#include "gvector.h"

struct WorldRobot
{
  float conf;

  RobotID robot_id;

  float angle;
  vector2f loc, vel;

  bool visible() const
  {
    return conf > .1;
  }

  WorldRobot() : conf(0), angle(0), loc(0, 0), vel(0, 0)
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

  WorldBall() : loc(0, 0), vel(0, 0), conf(0)
  {
  }
};

struct World
{
  double time;

  std::vector<WorldRobot> robots;
  WorldBall ball;

  void reset()
  {
    time = 0;
    robots.clear();
    ball.conf = 0;
  }
};
