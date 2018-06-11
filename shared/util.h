#pragma once

#include <algorithm>
#include <string>

#include "ssl_referee.pb.h"

#include "constants.h"
#include "gvector.h"
#include "world.h"

enum TeamCommand
{
  PREPARE_KICKOFF,
  PREPARE_PENALTY,
  DIRECT_FREE,
  INDIRECT_FREE,
  TIMEOUT,
  GOAL,
  BALL_PLACEMENT,
};

// bool isStageGameOn(SSL_Referee::Stage stage);

SSL_Referee::Command teamCommand(TeamCommand cmd, Team team);

struct tvec
{
  double t;
  vector2f v;

  tvec() : t(0), v(0, 0)
  {
  }
  tvec(double t_, vector2f v_) : t(t_), v(v_)
  {
  }

  bool between(tvec start, tvec end)
  {
    double c = (t - start.t) / (end.t - start.t);
    vector2f v0 = (1 - c) * start.v + c * end.v;

    double mult = .1;
    return dist(v, v0) < mult * dist(start.v, end.v);
  }
};

Team commandTeam(SSL_Referee::Command command);

std::string commandDisplayName(SSL_Referee::Command command);
std::string stageDisplayName(SSL_Referee::Stage stage);

template <class num>
inline num sign(num x)
{
  return (x >= 0) ? (x > 0) : -1;
}

template <class num>
inline num sign_nz(num x)
{
  return (x >= 0) ? 1 : -1;
}

template <class num1, class num2>
inline num1 abs_bound(num1 x, num2 range)
// bound absolute value x in [-range,range]
{
  if (x < -range) {
    x = -range;
  }
  if (x > range) {
    x = range;
  }
  return x;
}

template <class num1, class num2>
inline num1 bound(num1 x, num2 low, num2 high)
{
  if (x < low) {
    x = low;
  }
  if (x > high) {
    x = high;
  }
  return x;
}

const char *TeamName(Team team, bool capital = false);

std::string StringFormat(const char *format, va_list al);

vector2f OutOfBoundsLoc(const vector2f &objectLoc, const vector2f &objectDir);

vector2f ClosestDefenseAreaP(const vector2f &loc, bool ours, double dist);

bool IsInField(vector2f loc, float margin, bool avoid_defense);

vector2f BoundToField(vector2f loc, float margin, bool avoid_defense);

Team FlipTeam(Team team);

uint64_t GetTimeMicros();

Team RandomTeam();
