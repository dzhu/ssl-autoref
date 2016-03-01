#include "events.h"
#include "autoref.h"

#undef XZ
#undef X
#define XZ(s) #s
#define X(s) #s
const char *ref_state_names[] = {
#include "ref_states.def"
};

SSL_Referee::Stage NextStage(SSL_Referee::Stage s)
{
  switch (s) {
    case SSL_Referee::NORMAL_FIRST_HALF_PRE: return SSL_Referee::NORMAL_FIRST_HALF;
    case SSL_Referee::NORMAL_FIRST_HALF: return SSL_Referee::NORMAL_HALF_TIME;
    case SSL_Referee::NORMAL_HALF_TIME: return SSL_Referee::NORMAL_SECOND_HALF_PRE;
    case SSL_Referee::NORMAL_SECOND_HALF_PRE: return SSL_Referee::NORMAL_SECOND_HALF;
    case SSL_Referee::NORMAL_SECOND_HALF: return SSL_Referee::POST_GAME;
    default: return SSL_Referee::POST_GAME;
  }
}

const AutorefVariables &AutorefEvent::refVars() const
{
  return ref->vars;
}

const char InitEvent::ID;

void InitEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_INIT) {
    return;
  }

  fired = true;

  vars.cmd = SSL_Referee::STOP;
  vars.stage = SSL_Referee::NORMAL_FIRST_HALF_PRE;
  vars.state = REF_WAIT_START;
  vars.reset = true;
  vars.reset_loc.set(0, 0);
  setDescription("Autoref started", w.time);
}

const char RobotsStartedEvent::ID;

void RobotsStartedEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_WAIT_START) {
    return;
  }

  int seen = 0;
  for (const auto &r : w.robots) {
    if (r.visible() && r.vel.length() > 50) {
      seen |= (1 << r.team);
    }
  }

  if (seen == 3) {
    cnt++;
  }

  fired = (cnt > FrameRateInt / 2);
  if (fired) {
    vars.next_cmd = SSL_Referee::PREPARE_KICKOFF_BLUE;
    vars.reset = true;
    vars.reset_loc.set(0, 0);
    vars.state = REF_WAIT_STOP;
    setDescription("Robots started moving");
  }
}

const char BallSpeedEvent::ID;

void BallSpeedEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_RUN) {
    return;
  }
  if (vars.stage != SSL_Referee::NORMAL_FIRST_HALF && vars.stage != SSL_Referee::NORMAL_SECOND_HALF) {
    return;
  }

  if (!w.ball.visible()) {
    return;
  }

  if (last_time == 0) {
    last_time = w.time;
    last_loc = w.ball.loc;
    return;
  }

  double speed = dist(w.ball.loc, last_loc) / (w.time - last_time);
  if (speed > MaxKickSpeed * 1.02) {
    cnt++;
  }
  else {
    cnt = 0;
  }

  last_time = w.time;
  last_loc = w.ball.loc;

  fired = cnt > 2;

  if (fired) {
    vars.cmd = SSL_Referee::STOP;
    vars.next_cmd = (vars.touch_team == TeamBlue) ? SSL_Referee::INDIRECT_FREE_YELLOW : SSL_Referee::INDIRECT_FREE_BLUE;
    vars.reset = true;
    vars.reset_loc = BoundToField(w.ball.loc, 100, true);
    vars.state = REF_WAIT_STOP;
    setDescription("Ball kicked too fast by %s team", TeamName(vars.touch_team));
  }
}

const char BallStuckEvent::ID;

vector2f BallStuckEvent::legalPosition(vector2f loc)
{
  if (fabs(loc.x) > FieldLengthH) {
    loc.x = sign(loc.x) * FieldLengthH;
  }

  if (DistToDefenseArea(loc, true) < 700) {
    return ClosestDefenseAreaP(loc, true, 700);
  }
  if (DistToDefenseArea(loc, false) < 700) {
    return ClosestDefenseAreaP(loc, false, 700);
  }
  return loc;
}

void BallStuckEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_RUN) {
    stuck_count = 0;
    wait_frames = FrameRateInt;
    last_ball_loc = w.ball.loc;
    return;
  }

  if (--wait_frames < 0) {
    wait_frames = FrameRateInt;
    if (dist(w.ball.loc, last_ball_loc) < 250) {
      stuck_count++;
    }
    else {
      stuck_count = 0;
    }
    last_ball_loc = w.ball.loc;

    if (stuck_count > 10) {
      fired = true;
      vars.cmd = SSL_Referee::STOP;
      vars.state = REF_WAIT_STOP;
      vars.next_cmd = SSL_Referee::FORCE_START;
      vars.reset = true;
      vars.reset_loc = legalPosition(last_ball_loc);
      setDescription("Ball got stuck during play");
    }
  }
}

const char BallExitEvent::ID;

void BallExitEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_RUN) {
    return;
  }
  if (vars.stage != SSL_Referee::NORMAL_FIRST_HALF && vars.stage != SSL_Referee::NORMAL_SECOND_HALF) {
    return;
  }

  vector2f ball_loc;

  bool EXTRAPOLATE_RAW = true;
  int frames = 0;
  if (EXTRAPOLATE_RAW) {
    if (w.ball.visible()) {
      lost_cnt = 0;
      ball_loc = w.ball.loc;
      ball_history.add(tvec(w.time, ball_loc));
    }
    else {
      lost_cnt++;

      vector2f p0, v0;
      linvel(ball_history, p0, v0);

      frames = min(lost_cnt, 30);
      ball_loc = p0 + v0 * (frames + ball_history.size() - 1) * FramePeriod;
    }
  }
  else {
    ball_loc = w.ball.loc;
  }

  bool f = !IsInField(ball_loc + 0 * (ball_loc - last_ball_loc), -BallRadius, false);

  // printf("%.3f %d %d %.3f %.3f\n", w.time, frames, f, V2COMP(ball_loc));
  if (f) {
    cnt++;
  }
  else {
    cnt = 0;
  }
  fired = cnt > 1;

  if (fired) {
    if (vars.touch_team != TeamBlue && vars.touch_team != TeamYellow) {
      // printf("\nwarning: choosing touch_team randomly (was %d)", vars.touch_team);
      vars.touch_team = random_team();
    }

    vector2f out_loc = OutOfBoundsLoc(last_ball_loc, ball_loc - last_ball_loc);
    vector2f pos;
    // past goal line
    if (fabs(out_loc.x) - fabs(out_loc.y) > FieldLengthH - FieldWidthH) {
      bool goal_kick = (vars.touch_team == TeamBlue) ^ (vars.blue_side * out_loc.x > 0);
      pos.set(sign(out_loc.x) * (FieldLengthH - (goal_kick ? 500 : 100)), sign(out_loc.y) * (FieldWidthH - 100));
    }
    // past touch line
    else {
      pos.set(out_loc.x, sign(out_loc.y) * (FieldWidthH - 100));
    }

    vars.cmd = SSL_Referee::STOP;
    vars.kick_team = FlipTeam(vars.touch_team);
    vars.next_cmd = (vars.kick_team == TeamBlue) ? SSL_Referee::INDIRECT_FREE_BLUE : SSL_Referee::INDIRECT_FREE_YELLOW;
    vars.state = REF_WAIT_STOP;
    vars.reset = true;
    vars.reset_loc = pos;
    setDescription("Ball exited at <%.0f,%.0f>, touched by %s team", V2COMP(out_loc), TeamName(vars.touch_team));
  }

  if (IsInField(ball_loc, -BallRadius, false)) {
    last_ball_loc = ball_loc;
  }
}

const char BallTouchedEvent::ID;

void BallTouchedEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  CollideResult res;
  for (auto &proc : procs) {
    if (proc->proc(w, res)) {
      fired = true;
      vars.touch_team = res.team;
      setDescription("Ball touched by %s team", TeamName(vars.touch_team));
    }
  }
  return;
}

const char KickReadyEvent::ID;

void KickReadyEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (t0_bots == 0) {
    t0_bots = w.time;
  }
  if (t0_ball == 0) {
    t0_ball = w.time;
  }

  if (vars.stage != SSL_Referee::NORMAL_FIRST_HALF_PRE && vars.stage != SSL_Referee::NORMAL_FIRST_HALF
      && vars.stage != SSL_Referee::NORMAL_SECOND_HALF_PRE && vars.stage != SSL_Referee::NORMAL_SECOND_HALF) {
    return;
  }
  if (vars.state != REF_WAIT_STOP) {
    t0 = t0_bots = t0_ball = w.time;
    return;
  }

  bool bots_slow = true;
  for (const auto &r : w.robots) {
    if (r.visible() && r.vel.length() > 400) {
      bots_slow = false;
    }
  }
  if (!bots_slow) {
    t0_bots = w.time;
  }

  bool ball_ready = !w.ball.visible() || (w.ball.vel.length() < 100 && dist(w.ball.loc, vars.reset_loc) < 500);
  if (!ball_ready) {
    t0_ball = w.time;
  }

  bool timeout = (w.time - t0 > 15);
  fired = timeout || ((w.time - t0_bots > 1) && (w.time - t0_ball > 1));

  if (fired) {
    vars.touch_team = TeamNone;
    vars.cmd = vars.next_cmd;
    setDescription("Starting kick: %s", SSL_Referee::Command_Name(vars.cmd).c_str());

    // reset the desired kick location to the current location, in case it didn't get moved before timing out
    vars.reset_loc = w.ball.loc;
    switch (vars.next_cmd) {
      case SSL_Referee::INDIRECT_FREE_BLUE:
      case SSL_Referee::INDIRECT_FREE_YELLOW:
        vars.state = REF_WAIT_KICK;
        vars.kick_deadline = w.time + KickDeadline;
        break;

      case SSL_Referee::NORMAL_START:
        // do necessary things if this the start of the period
        if (vars.stage == SSL_Referee::NORMAL_FIRST_HALF_PRE || vars.stage == SSL_Referee::NORMAL_SECOND_HALF_PRE) {
          vars.stage = NextStage(vars.stage);
          vars.stage_end = w.time + TimeInHalf;

          // determine which team is on each side
          int blue_left_weight = 0;
          for (const auto &r : w.robots) {
            if (r.visible()) {
              blue_left_weight += ((r.team == TeamBlue) == (r.loc.x < 0)) ? 1 : -1;
            }
          }
          vars.blue_side = -sign_nz(blue_left_weight);
        }

        vars.state = REF_WAIT_KICK;
        vars.kick_deadline = w.time + KickDeadline;
        break;

      case SSL_Referee::FORCE_START: vars.state = REF_RUN; break;

      case SSL_Referee::PREPARE_KICKOFF_BLUE:
      case SSL_Referee::PREPARE_KICKOFF_YELLOW:
        t0 = t0_bots = t0_ball = w.time;
        vars.next_cmd = SSL_Referee::NORMAL_START;
        vars.state = REF_WAIT_STOP;
        break;

      default: break;
    }
  }
}

const char KickTakenEvent::ID;

void KickTakenEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_WAIT_KICK) {
    return;
  }

  if (!w.ball.visible()) {
    return;
  }

  if (w.ball.vel.length() > 400 || dist(w.ball.loc, vars.reset_loc) > 50) {
    fired = true;
  }

  if (fired) {
    vars.state = REF_RUN;
    setDescription("Kick taken");
  }
}

const char KickExpiredEvent::ID;

void KickExpiredEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state == REF_WAIT_KICK && w.time > vars.kick_deadline) {
    fired = true;
    vars.cmd = SSL_Referee::STOP;
    vars.reset = true;
    vars.state = REF_WAIT_STOP;
    vars.next_cmd = SSL_Referee::FORCE_START;
    setDescription("Team took too long to take a kick");
  }
}

const char GoalScoredEvent::ID;

void GoalScoredEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_RUN) {
    return;
  }
  if (vars.stage != SSL_Referee::NORMAL_FIRST_HALF && vars.stage != SSL_Referee::NORMAL_SECOND_HALF) {
    return;
  }

  vector2f ball_loc;

  // TODO dedup this and BallExitEvent
  bool EXTRAPOLATE_RAW = true;
  int frames = 0;
  if (EXTRAPOLATE_RAW) {
    // if we actually see the ball, record that
    if (w.ball.visible()) {
      lost_cnt = 0;
      ball_loc = w.ball.loc;
      ball_history.add(tvec(w.time, ball_loc));
    }
    // otherwise, project the last several positions forward
    else {
      lost_cnt++;

      vector2f p0, v0;
      linvel(ball_history, p0, v0);

      frames = min(lost_cnt, 5);
      ball_loc = p0 + v0 * (frames + ball_history.size() - 1) * FramePeriod;

      vector2f b(ball_loc.x, fabs(ball_loc.y));
      vector2f h(ball_history[0].v.x, fabs(ball_history[0].v.y));
      // check if the hallucinated trajectory crosses a goal wall
      if (segment_intersects(b, h, vector2f(FieldLengthH, GoalWidthH), vector2f(FieldLengthH + GoalDepth, GoalWidthH))) {
        return;
      }
    }
  }
  else {
    ball_loc = w.ball.loc;
  }

  fired = ((fabs(ball_loc.y) < GoalWidthH) && (fabs(ball_loc.x) > FieldLengthH) && (fabs(ball_loc.x) < FieldLengthH + GoalDepth));

  if (fired) {
    uint8_t scoring_team = (vars.blue_side * ball_loc.x > 0) ? TeamYellow : TeamBlue;
    vars.team[scoring_team].score++;

    vars.cmd = (scoring_team == TeamBlue) ? SSL_Referee::GOAL_BLUE : SSL_Referee::GOAL_YELLOW;
    vars.next_cmd = (scoring_team == TeamBlue) ? SSL_Referee::PREPARE_KICKOFF_YELLOW : SSL_Referee::PREPARE_KICKOFF_BLUE;
    vars.state = REF_WAIT_STOP;
    vars.reset = true;
    vars.reset_loc.set(0, 0);
    setDescription("Goal scored by %s team", TeamName(scoring_team));
  }
}

const char LongDribbleEvent::ID;

void LongDribbleEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  const WorldBall &ball = w.ball;

  // TODO do something smarter, since dribbling can prevent the ball from being
  // seen
  if (!ball.visible()) {
    return;
  }

  for (const auto &r : w.robots) {
    if (!r.visible()) {
      continue;
    }

    vector2f ball_local = (ball.loc - r.loc).rotate(-r.angle);
    DribbleRecord &d = dribble[r.team][r.robot_id];

    double InX = DribblerOffset;

    double NearX = DribblerOffset + BallRadius + 5;
    double NearY = 50;

    double MidX = 120;
    double MidY = 90;

    double FarX = 150;
    double FarY = 110;

    // does it seem to be on this robot's dribbler currently?
    // TODO do some longer-term reasoning about this
    if (fabs(ball_local.y) < NearY && ball_local.x < NearX && ball_local.x > InX) {
      if (!d.last) {
        d.start_loc = r.loc;
      }
      d.last = true;

      if (dist(r.loc, d.start_loc) > 1000) {
        fired = true;
        setDescription("Ball dribbled too far by robot %s-%X", TeamName(r.team), r.robot_id);
      }
    }
    else {
      int c = d.off_cnt++;
      if (c > 10) {
        d.last = false;
      }
    }
  }
}

const char StageTimeEndedEvent::ID;

void StageTimeEndedEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.stage_end <= 0 || w.time < vars.stage_end) {
    return;
  }

  fired = true;

  switch (vars.stage) {
    case SSL_Referee::NORMAL_FIRST_HALF:
      vars.cmd = SSL_Referee::HALT;
      vars.stage_end = w.time + TimeInHalftime;
      vars.state = REF_BREAK;
      break;

    case SSL_Referee::NORMAL_HALF_TIME:
      vars.cmd = SSL_Referee::STOP;
      vars.next_cmd = SSL_Referee::PREPARE_KICKOFF_YELLOW;
      vars.reset = true;
      vars.reset_loc.set(0, 0);
      vars.state = REF_WAIT_STOP;
      break;

    case SSL_Referee::NORMAL_SECOND_HALF:
      vars.cmd = SSL_Referee::HALT;
      vars.state = REF_BREAK;
      break;

    default: break;
  }
  setDescription("Stage ended: %s", SSL_Referee::Stage_Name(vars.stage).c_str());
  vars.stage = NextStage(vars.stage);
}
