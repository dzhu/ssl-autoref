#include "constants.h"

double Constants::TimeInHalf;

double Constants::TimeInHalftime;
double Constants::KickDeadline;

double Constants::FrameRate;
double Constants::FramePeriod;
unsigned int Constants::FrameRateInt;

// distance-related values (common)
float Constants::MaxRobotRadius;
float Constants::BallRadius;
int Constants::DribblerOffset;

// misc
float Constants::MaxKickSpeed;
int Constants::MaxTeamRobots;
int Constants::MaxRobots;

// field geometry (by division)
float Constants::FieldLengthH;
float Constants::FieldWidthH;
float Constants::DefenseLength;
float Constants::DefenseWidthH;
float Constants::GoalDepth;
float Constants::GoalWidthH;

void Constants::initCommon()
{
  MaxKickSpeed = 6500;
  TimeInHalf = 500;
  TimeInHalftime = 500;
  KickDeadline = 10;

  FrameRate = 61.51;
  FramePeriod = 1 / FrameRate;
  FrameRateInt = rint(FrameRate + 0.5);

  MaxRobotRadius = 90;
  BallRadius = 21;
  DribblerOffset = 79;
}

void Constants::initDivisionA()
{
  initCommon();
  MaxTeamRobots = 8;
  MaxRobots = MaxTeamRobots * NumTeams;
  DefenseLength = 1200;
  DefenseWidthH = 1200;
}

void Constants::initDivisionB()
{
  initCommon();
  MaxTeamRobots = 6;
  MaxRobots = MaxTeamRobots * NumTeams;
  DefenseLength = 1000;
  DefenseWidthH = 1000;
}

void Constants::updateGeometry(SSL_GeometryData &g)
{
  auto f = g.field();

  FieldLengthH = f.field_length() / 2.;
  FieldWidthH = f.field_width() / 2.;
  GoalDepth = f.goal_depth();
  GoalWidthH = f.goal_width() / 2.;
}
