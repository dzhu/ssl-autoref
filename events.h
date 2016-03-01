#pragma once

#include <chrono>
#include <random>
#include <vector>

#include <stdarg.h>

#include "referee.pb.h"

#include "constants.h"
#include "runqueue.h"
#include "touches.h"
#include "util.h"
#include "world.h"

using namespace std;

class EventAutoref;

#define XZ(s) s = 0
#define X(s) s
enum RefGameState
{
#include "ref_states.def"
};

extern const char *ref_state_names[];

struct AutorefVariables
{
  RefGameState state;
  SSL_Referee::Stage stage;
  SSL_Referee::Command cmd, next_cmd;

  bool reset;
  vector2f reset_loc;
  double stage_end;
  double kick_deadline;

  uint8_t kick_team;
  uint8_t kicker_id;

  uint8_t touch_team;

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
        reset_loc(0, 0),
        stage_end(0),
        kick_deadline(0),
        touch_team(TeamNone),
        blue_side(0),
        games_played(0)
  {
  }
};

class AutorefEvent
{
protected:
  EventAutoref *ref;
  bool fired, fired_last;

  AutorefVariables vars;

  virtual void _process(const World &w, bool ball_z_valid, float ball_z) = 0;

  const AutorefVariables &refVars() const;

  string description;

  void setDescription(const char *format, ...)
  {
    va_list al;
    va_start(al, format);
    description = StringFormat(format, al);
    va_end(al);
  }

public:
  void process(const World &w, bool ball_z_valid, float ball_z)
  {
    fired_last = fired;
    fired = false;
    vars = refVars();
    _process(w, ball_z_valid, ball_z);
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

  AutorefEvent(EventAutoref *_ref) : ref(_ref), fired(false), fired_last(false)
  {
  }
};

// each subclass must define a static const char ID for use in the templated
// retrieval by EventAutoref -- the value doesn't matter, only that the
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

  InitEvent(EventAutoref *_ref) : AutorefEvent(_ref)
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

  RobotsStartedEvent(EventAutoref *_ref) : AutorefEvent(_ref), cnt(0)
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
    return "BallSpeedEvent";
  }

  BallSpeedEvent(EventAutoref *_ref) : AutorefEvent(_ref), cnt(0), last_loc(0, 0), last_time(0)
  {
  }
};

class BallStuckEvent : public AutorefEvent
{
  int stuck_count;
  int wait_frames;

  vector2f last_ball_loc;

  vector2f legalPosition(vector2f loc);

public:
  static const char ID = 0;
  void _process(const World &w, bool ball_z_valid, float ball_z);
  const char *name() const
  {
    return "BallStuckEvent";
  }

  BallStuckEvent(EventAutoref *_ref) : AutorefEvent(_ref), stuck_count(0), wait_frames(0), last_ball_loc(0, 0)
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
    return "BallExitEvent";
  }

  unsigned int random_team()
  {
    return binary_dist(generator) ? TeamYellow : TeamBlue;
  }

  BallExitEvent(EventAutoref *_ref)
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
    return "BallTouchedEvent";
  }

  BallTouchedEvent(EventAutoref *_ref) : AutorefEvent(_ref)
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

  KickReadyEvent(EventAutoref *_ref) : AutorefEvent(_ref), t0_bots(0), t0_ball(0)
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
    return "KickTakenEvent";
  }

  KickTakenEvent(EventAutoref *_ref) : AutorefEvent(_ref)
  {
  }
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

  KickExpiredEvent(EventAutoref *_ref) : AutorefEvent(_ref)
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
    return "GoalScoredEvent";
  }

  GoalScoredEvent(EventAutoref *_ref) : AutorefEvent(_ref), last_ball_loc(0, 0), cnt(0), lost_cnt(0), stop_cnt(0)
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
    return "LongDribbleEvent";
  }

  LongDribbleEvent(EventAutoref *_ref) : AutorefEvent(_ref)
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

  StageTimeEndedEvent(EventAutoref *_ref) : AutorefEvent(_ref)
  {
  }
};
