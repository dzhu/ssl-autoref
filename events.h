#pragma once

#include <chrono>
#include <random>
#include <vector>

#include <cstdarg>

#include "constants.h"
#include "runqueue.h"
#include "touches.h"
#include "util.h"
#include "world.h"

#include "ssl_autoref.pb.h"
#include "ssl_referee.pb.h"

using namespace std;

class BaseAutoref;

enum RefGameState
{
#define XZ(s) s = 0
#define X(s) s
#include "ref_states.def"
};

extern const char *ref_state_names[];

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

  bool isValid()
  {
    return (team == TeamBlue || team == TeamYellow) && (id >= 0 && id < MaxRobotIds);
  }

  bool operator==(RobotID other)
  {
    return team == other.team && id == other.id;
  }
};

struct AutorefVariables
{
  RefGameState state;
  SSL_Referee::Stage stage;
  SSL_Referee::Command cmd, next_cmd;

  bool reset;
  vector2f reset_loc;
  double stage_end;
  double kick_deadline;

  RobotID kicker;
  RobotID toucher;
  double touch_time;

  vector2f touch_loc;

  int8_t blue_side;

  uint32_t games_played;
  struct TeamInfo
  {
    int timeout_n;
    double timeout_t;
    uint32_t score;
    uint32_t wins;

    TeamInfo() : timeout_n(4), timeout_t(300), score(0), wins(0)
    {
    }
  };
  TeamInfo team[2];

  AutorefVariables()
      : state(REF_INIT),
        stage(SSL_Referee::NORMAL_FIRST_HALF_PRE),
        cmd(SSL_Referee::HALT),
        next_cmd(SSL_Referee::HALT),
        reset(false),
        stage_end(0),
        kick_deadline(0),
        touch_time(0),
        blue_side(0),
        games_played(0)
  {
  }
};

class AutorefEvent
{
  bool enabled;

protected:
  BaseAutoref *ref;
  bool fired, fired_last;
  AutorefVariables vars;
  string description;
  ssl::SSL_Autoref autoref_msg;
  bool autoref_msg_valid;

  bool isEnabled()
  {
    return enabled;
  }

  virtual void _process(const World &w, bool ball_z_valid, float ball_z) = 0;

  const AutorefVariables &refVars() const;

  void setDescription(const char *format, ...)
  {
    va_list al;
    va_start(al, format);
    description = StringFormat(format, al);
    va_end(al);
  }

  void setReplayTimes(double t0, double t1)
  {
    auto replay = autoref_msg.mutable_replay();

    if (t0 < t1 - 5) {
      printf("Replay from %f to %f too long...", t0, t1);
      t0 = t1 - 5;
    }

    replay->set_start_timestamp(1e6 * t0);
    replay->set_end_timestamp(1e6 * t1);
  }

  void setFoulMessage(ssl::SSL_Autoref::RuleInfringement::FoulType type, Team offending_team)
  {
    auto foul = autoref_msg.mutable_foul();
    foul->set_foul_type(type);
    foul->set_offending_team(offending_team == TeamBlue ? ssl::SSL_Autoref::BLUE : ssl::SSL_Autoref::YELLOW);
  }

  void addFoulOffender(int id)
  {
    auto foul = autoref_msg.mutable_foul();
    foul->add_offending_robots(id);
  }

  void setFoulMessage(ssl::SSL_Autoref::RuleInfringement::FoulType type, RobotID offending_robot)
  {
    setFoulMessage(type, offending_robot.team);
    addFoulOffender(offending_robot.id);
  }

  void setDesignatedPoint(vector2f designated_point)
  {
    autoref_msg.mutable_foul()->mutable_designated_point()->set_x(designated_point.x);
    autoref_msg.mutable_foul()->mutable_designated_point()->set_y(designated_point.y);
  }

public:
  void process(const World &w, bool ball_z_valid, float ball_z)
  {
    fired_last = fired;
    fired = false;

    if (!enabled) {
      return;
    }

    vars = refVars();
    autoref_msg.Clear();
    autoref_msg_valid = false;
    _process(w, ball_z_valid, ball_z);
  }

  void setEnabled(bool e)
  {
    enabled = e;
  }

  virtual const char *name() const = 0;

  const AutorefVariables &getUpdate() const
  {
    return vars;
  }
  bool firing() const
  {
    return fired;
  }
  bool firingNew() const
  {
    return fired && !fired_last;
  }

  string getDescription() const
  {
    return description;
  }

  bool getMessage(ssl::SSL_Autoref &msg) const
  {
    if (autoref_msg_valid) {
      msg = autoref_msg;
    }
    return autoref_msg_valid;
  }

  AutorefEvent(BaseAutoref *_ref) : ref(_ref), fired(false), fired_last(false), enabled(true)
  {
  }
};

// each subclass must define a static const char ID for use in the templated
// retrieval by BaseAutoref -- the value doesn't matter, only that the
// variable exists

class InitEvent : public AutorefEvent
{
public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "InitEvent";
  }

  InitEvent(BaseAutoref *_ref) : AutorefEvent(_ref)
  {
  }
};

class RefboxUpdateEvent : public AutorefEvent
{
  SSL_Referee last_msg;

  int disagree_cnt;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "receive updates from refbox";
  }

  RefboxUpdateEvent(BaseAutoref *_ref) : AutorefEvent(_ref), disagree_cnt(0)
  {
  }
};

class RobotsStartedEvent : public AutorefEvent
{
  int cnt;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "RobotsStartedEvent";
  }

  RobotsStartedEvent(BaseAutoref *_ref) : AutorefEvent(_ref), cnt(0)
  {
  }
};

class BallSpeedEvent : public AutorefEvent
{
  int cnt;

  vector2f last_loc;
  double last_time;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "ball goes too fast";
  }

  BallSpeedEvent(BaseAutoref *_ref) : AutorefEvent(_ref), cnt(0), last_loc(0, 0), last_time(0)
  {
  }
};

class BallStuckEvent : public AutorefEvent
{
  int stuck_count;
  int wait_frames;

  vector2f last_ball_loc;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "ball gets stuck in play";
  }

  BallStuckEvent(BaseAutoref *_ref) : AutorefEvent(_ref), stuck_count(0), wait_frames(0), last_ball_loc(0, 0)
  {
  }
};

class DelayDoneEvent : public AutorefEvent
{
  int cnt;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "DelayDoneEvent";
  }

  DelayDoneEvent(BaseAutoref *_ref) : AutorefEvent(_ref), cnt(0)
  {
  }
};

class BallExitEvent : public AutorefEvent
{
  RunningQueue<tvec, 5> ball_history;
  int lost_cnt;
  int stop_cnt;

  int cnt;
  vector2f last_ball_loc;

  std::default_random_engine generator;
  std::uniform_int_distribution<unsigned int> binary_dist;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "ball exits the field";
  }

  BallExitEvent(BaseAutoref *_ref)
      : AutorefEvent(_ref),
        last_ball_loc(0, 0),
        cnt(0),
        lost_cnt(0),
        stop_cnt(0),
        generator(std::chrono::system_clock::now().time_since_epoch().count()),
        binary_dist(0, 1)
  {
    ball_history.init();
  }
};

class BallTouchedEvent : public AutorefEvent
{
  vector<TouchProcessor *> procs;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "ball is touched";
  }

  BallTouchedEvent(BaseAutoref *_ref) : AutorefEvent(_ref)
  {
    // TODO add old accel-based processor
    procs.push_back(new BackTrackProcessor());
    procs.push_back(new RobotDistProcessor());
  }
};

class KickReadyEvent : public AutorefEvent
{
  double t0;
  double t0_bots;
  double t0_ball;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "KickReadyEvent";
  }

  KickReadyEvent(BaseAutoref *_ref) : AutorefEvent(_ref), t0_bots(0), t0_ball(0)
  {
  }
};

class KickTakenEvent : public AutorefEvent
{
public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "a kick is taken";
  }

  KickTakenEvent(BaseAutoref *_ref) : AutorefEvent(_ref)
  {
  }

  RobotID checkDefenseAreaDistanceInfraction(const World &w) const;
};

class KickExpiredEvent : public AutorefEvent
{
public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "KickExpiredEvent";
  }

  KickExpiredEvent(BaseAutoref *_ref) : AutorefEvent(_ref)
  {
  }
};

class GoalScoredEvent : public AutorefEvent
{
  RunningQueue<tvec, 5> ball_history;
  int lost_cnt;
  int stop_cnt;

  int cnt;
  vector2f last_ball_loc;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "a goal is scored";
  }

  GoalScoredEvent(BaseAutoref *_ref) : AutorefEvent(_ref), last_ball_loc(0, 0), cnt(0), lost_cnt(0), stop_cnt(0)
  {
    ball_history.init();
  }
};

class LongDribbleEvent : public AutorefEvent
{
  struct DribbleRecord
  {
    vector2f start_loc;
    int off_cnt;
    bool last, last_active;

    DribbleRecord() : start_loc(0, 0), off_cnt(0), last(false)
    {
    }
  };

  DribbleRecord dribble[NumTeams][MaxRobotIds];

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "a robot dribbles the ball too far";
  }

  LongDribbleEvent(BaseAutoref *_ref) : AutorefEvent(_ref)
  {
  }
};

class StageTimeEndedEvent : public AutorefEvent
{
public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "StageTimeEndedEvent";
  }

  StageTimeEndedEvent(BaseAutoref *_ref) : AutorefEvent(_ref)
  {
  }
};

class TooManyRobotsEvent : public AutorefEvent
{
  int blue_frames, yellow_frames;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "a team has too many robots";
  }

  TooManyRobotsEvent(BaseAutoref *_ref) : AutorefEvent(_ref), blue_frames(0), yellow_frames(0)
  {
  }
};

class RobotSpeedEvent : public AutorefEvent
{
  int violation_frames[NumTeams];

  constexpr static float GameOffRobotSpeedLimit = 1600;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "a robot moves too fast during game off";
  }

  RobotSpeedEvent(BaseAutoref *_ref) : AutorefEvent(_ref)
  {
    violation_frames[0] = violation_frames[1] = 0;
  }
};

class StopDistanceEvent : public AutorefEvent
{
  int violation_frames[NumTeams];

  constexpr static float GameOffRobotDistanceLimit = 450;

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "a robot is too close to the ball during game off";
  }

  StopDistanceEvent(BaseAutoref *_ref) : AutorefEvent(_ref)
  {
    violation_frames[0] = violation_frames[1] = 0;
  }
};
