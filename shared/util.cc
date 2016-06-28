#include <cstdarg>

#include "geomalgo.h"
#include "util.h"

SSL_Referee::Command team_command(TeamCommand cmd, Team team)
{
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
    return (signy == 1) ? vector2f(objectLoc.x, FieldWidthH) : vector2f(objectLoc.x, -FieldWidthH);
  }
  if (objectDir.y == 0) {
    return (signx == 1) ? vector2f(FieldLengthH, objectLoc.y) : vector2f(-FieldLengthH, objectLoc.y);
  }

  vector2f dir = objectDir.norm();
  double xtime, ytime;
  if ((signx == 1) && (signy == 1)) {
    double theirSideDist = FieldLengthH - objectLoc.x;
    double leftSideDist = FieldWidthH - objectLoc.y;
    xtime = theirSideDist / dir.x;
    ytime = leftSideDist / dir.y;
    return (xtime < ytime) ? vector2f(FieldLengthH, objectLoc.y + xtime * dir.y)
                           : vector2f(objectLoc.x + ytime * dir.x, FieldWidthH);
  }
  if ((signx == 1) && (signy == -1)) {
    double theirSideDist = FieldLengthH - objectLoc.x;
    double rightSideDist = fabs(-FieldWidthH - objectLoc.y);
    xtime = fabs(theirSideDist / dir.x);
    ytime = fabs(rightSideDist / dir.y);
    return (xtime < ytime) ? vector2f(FieldLengthH, objectLoc.y + xtime * dir.y)
                           : vector2f(objectLoc.x + ytime * dir.x, -FieldWidthH);
  }
  if ((signx == -1) && (signy == 1)) {
    double ourSideDist = fabs(-FieldLengthH - objectLoc.x);
    double rightSideDist = FieldWidthH - objectLoc.y;
    xtime = fabs(ourSideDist / dir.x);
    ytime = fabs(rightSideDist / dir.y);
    return (xtime < ytime) ? vector2f(-FieldLengthH, objectLoc.y + xtime * dir.y)
                           : vector2f(objectLoc.x + ytime * dir.x, FieldWidthH);
  }
  double ourSideDist = fabs(-FieldLengthH - objectLoc.x);
  double leftSideDist = fabs(-FieldWidthH - objectLoc.y);
  xtime = fabs(ourSideDist / dir.x);
  ytime = fabs(leftSideDist / dir.y);
  return (xtime < ytime) ? vector2f(-FieldLengthH, objectLoc.y + xtime * dir.y)
                         : vector2f(objectLoc.x + ytime * dir.x, -FieldWidthH);
}

vector2f ClosestDefenseAreaP(const vector2f &loc, bool ours, double dist)
{
  // Check if the location is outside the goal line.
  if (ours && loc.x < -FieldLengthH) {
    return vector2f(-FieldLengthH - dist, loc.y);
  }
  if (!ours && loc.x > FieldLengthH) {
    return vector2f(FieldLengthH + dist, loc.y);
  }

  const float goal_x = ours ? -FieldLengthH : FieldLengthH;
  const float dy = fabs(loc.y) - static_cast<float>(DefenseStretchH);
  if (dy > 0) {
    // On the circular part
    vector2f defenseCircCent(goal_x, DefenseStretchH * sign(loc.y));
    vector2f circLocV = (defenseCircCent - loc);
    vector2f locDir = circLocV.norm(DefenseRadius + dist);
    return vector2f(defenseCircCent - locDir);
  }

  // Measure x-distance
  return vector2f(goal_x - sign(goal_x) * (DefenseRadius + dist), loc.y);
}

bool IsInField(vector2f loc, float margin, bool avoid_defense)
{
  // check outside field boundaries
  if (fabs(loc.x) > FieldLengthH - margin) {
    return false;
  }
  if (fabs(loc.y) > FieldWidthH - margin) {
    return false;
  }

  // check defense area
  if (avoid_defense) {
    // static const vector2f cen(-FieldLengthH,0);
    // if(sqdist(loc,cen) < sq(DefenseRadius + margin)) return(false);

    float x = loc.x, y = fabs(loc.y);
    vector2f p1(-FieldLengthH, DefenseStretchH), loc2 = loc;
    loc2.y = fabs(loc2.y);
    if (y > DefenseStretchH) {
      if (dist(p1, loc2) < DefenseRadius + margin) {
        return false;
      }
    }
    else {
      if (fabs(p1.x - x) < DefenseRadius + margin) {
        return false;
      }
    }
  }

  return true;
}

vector2f BoundToField(vector2f loc, float margin, bool avoid_defense)
{
  loc.x = abs_bound(loc.x, FieldLengthH - margin);
  loc.y = abs_bound(loc.y, FieldWidthH - margin);

  if (avoid_defense) {
    const float clearance = DefenseRadius + margin;
    if (fabs(loc.y) > DefenseStretchH) {
      const vector2f goalQCircleCen_AbsY(-FieldLengthH, DefenseStretchH);
      const vector2f loc_AbsY(loc.x, fabs(loc.y));
      const float distToDefense = (goalQCircleCen_AbsY - loc_AbsY).length();
      if (distToDefense < clearance) {
        const vector2f goalQCircleCen = vector2f(-FieldLengthH, sign(loc.y) * DefenseStretchH);
        loc = goalQCircleCen + (loc - goalQCircleCen) * clearance / distToDefense;
      }
    }
    else {
      if (fabs(-FieldLengthH - loc.x) < clearance) {
        loc.x = -FieldLengthH + clearance;
      }
    }
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

int8_t GuessBlueSide(const World &w)
{
  double blue_right_weight = 0;
  for (const auto &r : w.robots) {
    if (r.visible()) {
      if (DistToDefenseArea(r.loc, true) < -MaxRobotRadius) {
        blue_right_weight += 5 * FieldLengthH * ((r.team == TeamBlue) ? 1 : -1);
      }
      else if (DistToDefenseArea(r.loc, false) < -MaxRobotRadius) {
        blue_right_weight += -5 * FieldLengthH * ((r.team == TeamBlue) ? 1 : -1);
      }
      else {
        blue_right_weight += r.loc.x * ((r.team == TeamBlue) ? 1 : -1);
      }
    }
  }
  return static_cast<int8_t>(sign_nz(blue_right_weight));
}
