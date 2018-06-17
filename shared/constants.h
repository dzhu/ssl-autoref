#pragma once

#include <math.h>
#include <stdint.h>

#include "messages_robocup_ssl_geometry.pb.h"

class Constants
{
public:
  // time-related values
  static double TimeInHalf;
  static double TimeInHalftime;
  static double KickDeadline;

  static double FrameRate;
  static double FramePeriod;
  static unsigned int FrameRateInt;

  // distance-related values (common)
  static float MaxRobotRadius;
  static float BallRadius;
  static int DribblerOffset;

  // misc
  static float MaxKickSpeed;
  static int MaxTeamRobots;
  static int MaxRobots;

  // field geometry (by division)
  static float FieldLengthH;
  static float FieldWidthH;
  static float DefenseLength;
  static float DefenseWidthH;
  static float GoalDepth;
  static float GoalWidthH;

  // init functions
  static void initCommon();
  static void initDivisionA();
  static void initDivisionB();
  static void updateGeometry(const SSL_GeometryData &g);
};

enum Team : int
{
  TeamBlue = 0,
  TeamYellow = 1,
  TeamNone = 2,
};
static const int NumTeams = static_cast<int>(TeamNone);

static const int MaxRobotIds = 16;
static const int MaxCameras = 8;

struct RobotID
{
  Team team;
  uint8_t id;

  RobotID() : team(TeamNone), id(MaxRobotIds)
  {
  }
  RobotID(Team t, uint8_t i) : team(t), id(i)
  {
  }

  void set(Team t, uint8_t i)
  {
    team = t;
    id = i;
  }

  void clear()
  {
    set(TeamNone, MaxRobotIds);
  }

  bool isValid() const
  {
    return (team == TeamBlue || team == TeamYellow) && (id >= 0 && id < MaxRobotIds);
  }

  bool operator==(RobotID other) const
  {
    return team == other.team && id == other.id;
  }

  bool operator!=(RobotID other) const
  {
    return !(*this == other);
  }
};
