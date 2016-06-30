#pragma once

#include <math.h>

static const double TimeInHalf = 600;
static const double TimeInHalftime = 300;
static const double KickDeadline = 10;

static const int MaxCameras = 4;

enum Team : int
{
  TeamBlue = 0,
  TeamYellow = 1,
  TeamNone = 2,
};
static const int NumTeams = static_cast<int>(TeamNone);

static const int MaxTeamRobots = 6;
static const int MaxRobots = NumTeams * MaxTeamRobots;
static const int MaxRobotIds = 16;

static const float MaxRobotRadius = 90;
static const float BallRadius = 21;
static const int DribblerOffset = 79;

static const double FrameRate = 61.51;
static const double FramePeriod = 1.0 / FrameRate;
static const unsigned int FrameRateInt = rint(FrameRate + 0.5);

static const float GoalDepth = 180;
static const float GoalWidthH = 500;
static const float FieldLengthH = 4500;
static const float FieldWidthH = 3000;

static const float DefenseStretchH = 250;
static const float DefenseRadius = 1000;

static const float MaxKickSpeed = 8000;
