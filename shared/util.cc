#include <cstdarg>
#include <random>

#include "geomalgo.h"
#include "util.h"

// bool isStageGameOn(SSL_Referee::Stage stage)
// {
//   switch(stage)
// }

SSL_Referee::Command teamCommand(TeamCommand cmd, Team team)
{
  if (team != TeamBlue && team != TeamYellow) {
    team = RandomTeam();
  }

#define X(s) \
  case s:    \
    return (team == TeamBlue) ? SSL_Referee::s##_BLUE : SSL_Referee::s##_YELLOW

  switch (cmd) {
    X(PREPARE_KICKOFF);
    X(PREPARE_PENALTY);
    X(DIRECT_FREE);
    X(INDIRECT_FREE);
    X(TIMEOUT);
    X(GOAL);
    X(BALL_PLACEMENT);
  }

#undef X
}

Team commandTeam(SSL_Referee::Command command)
{
  switch (command) {
    case SSL_Referee::TIMEOUT_BLUE:
    case SSL_Referee::GOAL_BLUE:
    case SSL_Referee::BALL_PLACEMENT_BLUE:
    case SSL_Referee::DIRECT_FREE_BLUE:
    case SSL_Referee::INDIRECT_FREE_BLUE:
    case SSL_Referee::PREPARE_KICKOFF_BLUE:
    case SSL_Referee::PREPARE_PENALTY_BLUE:
      return TeamBlue;

    case SSL_Referee::TIMEOUT_YELLOW:
    case SSL_Referee::GOAL_YELLOW:
    case SSL_Referee::BALL_PLACEMENT_YELLOW:
    case SSL_Referee::DIRECT_FREE_YELLOW:
    case SSL_Referee::INDIRECT_FREE_YELLOW:
    case SSL_Referee::PREPARE_KICKOFF_YELLOW:
    case SSL_Referee::PREPARE_PENALTY_YELLOW:
      return TeamYellow;

    case SSL_Referee::NORMAL_START:
    case SSL_Referee::STOP:
    case SSL_Referee::FORCE_START:
    case SSL_Referee::HALT:
      return TeamNone;
  }
}

std::string commandDisplayName(SSL_Referee::Command command)
{
  switch (command) {
    case SSL_Referee::TIMEOUT_BLUE:
      return "TIMEOUT BLUE";
    case SSL_Referee::GOAL_BLUE:
      return "GOAL BLUE";
    case SSL_Referee::BALL_PLACEMENT_BLUE:
      return "PLACEMENT BLUE";
    case SSL_Referee::DIRECT_FREE_BLUE:
      return "DIRECT BLUE";
    case SSL_Referee::INDIRECT_FREE_BLUE:
      return "INDIRECT BLUE";
    case SSL_Referee::PREPARE_KICKOFF_BLUE:
      return "KICKOFF BLUE";
    case SSL_Referee::PREPARE_PENALTY_BLUE:
      return "PENALTY BLUE";

    case SSL_Referee::TIMEOUT_YELLOW:
      return "TIMEOUT YELLOW";
    case SSL_Referee::GOAL_YELLOW:
      return "GOAL YELLOW";
    case SSL_Referee::BALL_PLACEMENT_YELLOW:
      return "PLACEMENT YELLOW";
    case SSL_Referee::DIRECT_FREE_YELLOW:
      return "DIRECT YELLOW";
    case SSL_Referee::INDIRECT_FREE_YELLOW:
      return "INDIRECT YELLOW";
    case SSL_Referee::PREPARE_KICKOFF_YELLOW:
      return "KICKOFF YELLOW";
    case SSL_Referee::PREPARE_PENALTY_YELLOW:
      return "PENALTY YELLOW";

    case SSL_Referee::NORMAL_START:
      return "START";
    case SSL_Referee::STOP:
      return "STOP";
    case SSL_Referee::FORCE_START:
      return "FORCE START";
    case SSL_Referee::HALT:
      return "HALT";
  }
}

std::string stageDisplayName(SSL_Referee::Stage stage)
{
  switch (stage) {
    case SSL_Referee::NORMAL_FIRST_HALF_PRE:
      return "PRE FIRST HALF";
    case SSL_Referee::NORMAL_FIRST_HALF:
      return "FIRST HALF";
    case SSL_Referee::NORMAL_HALF_TIME:
      return "HALF TIME";
    case SSL_Referee::NORMAL_SECOND_HALF_PRE:
      return "PRE SECOND HALF";
    case SSL_Referee::NORMAL_SECOND_HALF:
      return "SECOND HALF";
    default:
      return "POST GAME";
  }
}

const char *TeamName(Team team, bool capital)
{
  return team == TeamBlue ? (capital ? "Blue" : "blue") : (capital ? "Yellow" : "yellow");
}

std::string StringFormat(const char *format, va_list al)
{
  va_list al2;
  va_copy(al2, al);

  int len = vsnprintf(nullptr, 0, format, al) + 1;

  char buf[len];

  vsnprintf(buf, len, format, al2);

  va_end(al);
  va_end(al2);

  return std::string(buf);
}

vector2f OutOfBoundsLoc(const vector2f &objectLoc, const vector2f &objectDir)
{
  int signx = sign(objectDir.x);
  int signy = sign(objectDir.y);
  if (objectDir.x == 0 && objectDir.y == 0) {
    return objectLoc;
  }
  if (objectDir.x == 0) {
    return (signy == 1) ? vector2f(objectLoc.x, Constants::FieldWidthH)
                        : vector2f(objectLoc.x, -Constants::FieldWidthH);
  }
  if (objectDir.y == 0) {
    return (signx == 1) ? vector2f(Constants::FieldLengthH, objectLoc.y)
                        : vector2f(-Constants::FieldLengthH, objectLoc.y);
  }

  vector2f dir = objectDir.norm();
  double xtime, ytime;
  if ((signx == 1) && (signy == 1)) {
    double theirSideDist = Constants::FieldLengthH - objectLoc.x;
    double leftSideDist = Constants::FieldWidthH - objectLoc.y;
    xtime = theirSideDist / dir.x;
    ytime = leftSideDist / dir.y;
    return (xtime < ytime) ? vector2f(Constants::FieldLengthH, objectLoc.y + xtime * dir.y)
                           : vector2f(objectLoc.x + ytime * dir.x, Constants::FieldWidthH);
  }
  if ((signx == 1) && (signy == -1)) {
    double theirSideDist = Constants::FieldLengthH - objectLoc.x;
    double rightSideDist = fabs(-Constants::FieldWidthH - objectLoc.y);
    xtime = fabs(theirSideDist / dir.x);
    ytime = fabs(rightSideDist / dir.y);
    return (xtime < ytime) ? vector2f(Constants::FieldLengthH, objectLoc.y + xtime * dir.y)
                           : vector2f(objectLoc.x + ytime * dir.x, -Constants::FieldWidthH);
  }
  if ((signx == -1) && (signy == 1)) {
    double ourSideDist = fabs(-Constants::FieldLengthH - objectLoc.x);
    double rightSideDist = Constants::FieldWidthH - objectLoc.y;
    xtime = fabs(ourSideDist / dir.x);
    ytime = fabs(rightSideDist / dir.y);
    return (xtime < ytime) ? vector2f(-Constants::FieldLengthH, objectLoc.y + xtime * dir.y)
                           : vector2f(objectLoc.x + ytime * dir.x, Constants::FieldWidthH);
  }
  double ourSideDist = fabs(-Constants::FieldLengthH - objectLoc.x);
  double leftSideDist = fabs(-Constants::FieldWidthH - objectLoc.y);
  xtime = fabs(ourSideDist / dir.x);
  ytime = fabs(leftSideDist / dir.y);
  return (xtime < ytime) ? vector2f(-Constants::FieldLengthH, objectLoc.y + xtime * dir.y)
                         : vector2f(objectLoc.x + ytime * dir.x, -Constants::FieldWidthH);
}

vector2f ClosestDefenseAreaP(const vector2f &loc, bool positive_x, double dist)
{
  auto sgn = positive_x ? 1 : -1;
  auto x = sgn * loc.x, y = std::abs(loc.y);

  vector2f ret = loc;

  if (x > Constants::FieldLengthH - Constants::DefenseLength
      && x + y > Constants::FieldLengthH - Constants::DefenseLength + Constants::DefenseWidthH) {
    ret.set(x, Constants::DefenseWidthH + dist);
  }
  else if (y < Constants::DefenseWidthH
           && x + y <= Constants::FieldLengthH - Constants::DefenseLength + Constants::DefenseWidthH) {
    ret.set(Constants::FieldLengthH - Constants::DefenseLength - dist, y);
  }
  else {
    vector2f corner(Constants::FieldLengthH - Constants::DefenseLength, Constants::DefenseWidthH);
    ret.set(corner + (loc - corner).norm(dist));
  }

  if (loc.y < 0) {
    ret.y *= -1;
  }
  return ret * sgn;
}

bool IsInField(vector2f loc, float margin, bool avoid_defense)
{
  auto x = std::abs(loc.x), y = std::abs(loc.y);

  // check outside field boundaries
  if (x > Constants::FieldLengthH - margin) {
    return false;
  }
  if (y > Constants::FieldWidthH - margin) {
    return false;
  }

  // check defense area
  if (avoid_defense) {
    if (x > Constants::FieldLengthH - Constants::DefenseLength - margin && y < Constants::DefenseWidthH + margin) {
      return false;
    }
  }

  return true;
}

vector2f BoundToField(vector2f loc, float margin, bool avoid_defense)
{
  loc.x = abs_bound(loc.x, Constants::FieldLengthH - margin);
  loc.y = abs_bound(loc.y, Constants::FieldWidthH - margin);

  if (avoid_defense) {
    // TODO
  }
  return loc;
}

Team FlipTeam(Team team)
{
  switch (team) {
    case TeamBlue:
      return TeamYellow;
    case TeamYellow:
      return TeamBlue;
    default:
      return TeamNone;
  }
}

uint64_t GetTimeMicros()
{
  timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);
  return tv.tv_sec * 1000000 + tv.tv_nsec / 1000;
}

Team RandomTeam()
{
  static std::default_random_engine generator(std::random_device{}());
  static std::uniform_int_distribution<unsigned int> binary_dist(0, 1);
  return binary_dist(generator) ? TeamYellow : TeamBlue;
}
