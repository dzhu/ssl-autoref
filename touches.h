#pragma once

#include "constants.h"
#include "shared/geomalgo.h"
#include "shared/runqueue.h"
#include "util.h"
#include "world.h"

struct CollideResult
{
public:
  int team;
  int robot_id;
  double time;

  CollideResult() : team(0), robot_id(0), time(0)
  {
  }
  CollideResult(int t, int r, double tm) : team(t), robot_id(r), time(tm)
  {
  }
};

class TouchProcessor
{
public:
  virtual bool proc(const World &w, CollideResult &res) = 0;
  virtual const char *name() = 0;
};

class LineCheckProcessor : public TouchProcessor
{
  RunningQueue<World, 100> history;
  int last;

  bool line3(World a, World b, World c)
  {
    return cosine(a.ball.loc - b.ball.loc, c.ball.loc - b.ball.loc) < -.95;
    // double f = (b.timestamp - a.timestamp) / (c.timestamp - a.timestamp);
    // vector2f v = (1 - f) * a.ball.loc - f * c.ball.loc;
    // return dist(v, b.ball.loc) < 10;
  }

public:
  LineCheckProcessor() : last(0)
  {
    history.init();
  }

  bool proc(const World &w, CollideResult &res);
  const char *name()
  {
    return "LineCheckProcessor";
  }
};

class RobotDistProcessor : public TouchProcessor
{
  RunningQueue<tvec, 6> robot_history[NumTeams][MaxRobotIds];

  int last;

public:
  RobotDistProcessor() : last(0)
  {
    for (int t = 0; t < NumTeams; t++)
      for (int r = 0; r < MaxRobotIds; r++) robot_history[t][r].init();
  }

  bool proc(const World &w, CollideResult &res);
  const char *name()
  {
    return "RobotDistProcessor";
  }
};

template <const int n>
void linvel(RunningQueue<tvec, n> &vals, vector2f &p0, vector2f &v0, int samples = 0)
{
  if (samples == 0) samples = n;

  double t0 = vals[-samples + 1].t;
  double sxx = 0, sxy = 0, sx = 0, sy = 0;
  double txx = 0, txy = 0, tx = 0, ty = 0;
  for (int i = -samples + 1; i <= 0; i++) {
    double t = vals[i].t - t0;
    double x = vals[i].v.x;
    double y = vals[i].v.y;

    sxx += t * t;
    sxy += t * x;
    sx += t;
    sy += x;

    txx += t * t;
    txy += t * y;
    tx += t;
    ty += y;
  }
  double vx = (samples * sxy - sx * sy) / (samples * sxx - sx * sx);
  double vy = (samples * txy - tx * ty) / (samples * txx - tx * tx);
  double cx = (sy - sx * vx) / samples;
  double cy = (ty - tx * vy) / samples;

  p0.set(cx, cy);
  v0.set(vx, vy);
}

class BackTrackProcessor : public TouchProcessor
{
  static const int VEL_SAMPLES = 4;
  static const int COMP_SAMPLES = 4;
  static const int HIST_LEN = VEL_SAMPLES + COMP_SAMPLES;
  float t0;
  RunningQueue<tvec, HIST_LEN> robot_history[NumTeams][MaxRobotIds];

  int last;

public:
  BackTrackProcessor() : last(0), t0(0)
  {
    for (int t = 0; t < NumTeams; t++) {
      for (int r = 0; r < MaxRobotIds; r++) {
        robot_history[t][r].init();
      }
    }
  }

  bool proc(const World &w, CollideResult &res);
  const char *name()
  {
    return "BackTrackProcessor";
  }
};
