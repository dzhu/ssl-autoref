#include "events.h"

#include <cinttypes>

#include "base_ref.h"
#include "geomalgo.h"
#include "util.h"

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
    case SSL_Referee::NORMAL_FIRST_HALF_PRE:
      return SSL_Referee::NORMAL_FIRST_HALF;
    case SSL_Referee::NORMAL_FIRST_HALF:
      return SSL_Referee::NORMAL_HALF_TIME;
    case SSL_Referee::NORMAL_HALF_TIME:
      return SSL_Referee::NORMAL_SECOND_HALF_PRE;
    case SSL_Referee::NORMAL_SECOND_HALF_PRE:
      return SSL_Referee::NORMAL_SECOND_HALF;
    case SSL_Referee::NORMAL_SECOND_HALF:
      return SSL_Referee::POST_GAME;
    default:
      return SSL_Referee::POST_GAME;
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

const char RefboxUpdateEvent::ID;

void RefboxUpdateEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  const SSL_Referee &msg = ref->getRefboxMessage();

  if (vars.cmd != msg.command() || vars.stage != msg.stage()) {
    disagree_cnt++;
  }
  else {
    disagree_cnt = 0;
  }

  if (disagree_cnt < 120 && msg.command() == last_msg.command() && msg.stage() == last_msg.stage()) {
    return;
  }

  disagree_cnt = 0;
  last_msg = msg;
  fired = true;
  vars.cmd = msg.command();
  vars.stage = msg.stage();

  switch (msg.command()) {
    case SSL_Referee::HALT:
    case SSL_Referee::TIMEOUT_YELLOW:
    case SSL_Referee::TIMEOUT_BLUE:
      vars.state = REF_BREAK;
      break;

    case SSL_Referee::FORCE_START:
      vars.state = REF_RUN;
      break;

    case SSL_Referee::STOP:
    case SSL_Referee::GOAL_YELLOW:
    case SSL_Referee::GOAL_BLUE:
    case SSL_Referee::BALL_PLACEMENT_YELLOW:
    case SSL_Referee::BALL_PLACEMENT_BLUE:
      vars.state = REF_WAIT_STOP;
      break;

    case SSL_Referee::DIRECT_FREE_YELLOW:
    case SSL_Referee::DIRECT_FREE_BLUE:
    case SSL_Referee::INDIRECT_FREE_YELLOW:
    case SSL_Referee::INDIRECT_FREE_BLUE:
    case SSL_Referee::PREPARE_KICKOFF_YELLOW:
    case SSL_Referee::PREPARE_KICKOFF_BLUE:
    case SSL_Referee::PREPARE_PENALTY_YELLOW:
    case SSL_Referee::PREPARE_PENALTY_BLUE:
    case SSL_Referee::NORMAL_START:
      vars.reset_loc = w.ball.loc;
      vars.state = REF_WAIT_KICK;
      break;
  }

  switch (msg.command()) {
    case SSL_Referee::DIRECT_FREE_YELLOW:
    case SSL_Referee::INDIRECT_FREE_YELLOW:
    case SSL_Referee::PREPARE_KICKOFF_YELLOW:
    case SSL_Referee::PREPARE_PENALTY_YELLOW:
      vars.kicker.set(TeamYellow, MaxRobotIds);
      break;

    case SSL_Referee::PREPARE_KICKOFF_BLUE:
    case SSL_Referee::INDIRECT_FREE_BLUE:
    case SSL_Referee::DIRECT_FREE_BLUE:
    case SSL_Referee::PREPARE_PENALTY_BLUE:
      vars.kicker.set(TeamBlue, MaxRobotIds);
      break;

    default:
      break;
  }

  fired = true;
  setDescription("Got refbox command: %s", SSL_Referee::Command_Name(msg.command()).c_str());
  last_msg = msg;
}

const char RobotsStartedEvent::ID;

void RobotsStartedEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_WAIT_START) {
    return;
  }

  int seen = 0;
  for (const auto &r : w.robots) {
    if (r.visible() && r.vel.length() > 30) {
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
    vars.next_cmd = teamCommand(INDIRECT_FREE, vars.toucher.team);
    vars.reset = true;
    vars.reset_loc = BoundToField(w.ball.loc, 100, true);
    vars.state = REF_WAIT_STOP;
    setDescription("Ball kicked too fast by %s team", TeamName(vars.toucher.team));

    autoref_msg_valid = true;
    setReplayTimes(vars.touch_time, w.time);
    setFoulMessage(ssl::SSL_Autoref::RuleInfringement::BALL_SPEED, vars.toucher);
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

    if (stuck_count > 12) {
      stuck_count = 0;
      fired = true;
      vars.cmd = SSL_Referee::STOP;
      vars.state = REF_WAIT_STOP;
      vars.next_cmd = SSL_Referee::FORCE_START;
      vars.reset = true;
      vars.reset_loc = legalPosition(last_ball_loc);
      setDescription("Ball got stuck during play");

      autoref_msg_valid = true;
      setReplayTimes(w.time - 3, w.time);
      autoref_msg.set_lack_of_progress(true);
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
  int MAX_EXTRAPOLATE_FRAMES = 5;
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

      frames = min(lost_cnt, MAX_EXTRAPOLATE_FRAMES);
      ball_loc = p0 + v0 * FramePeriod * (frames + ball_history.size() - 1);
    }
  }
  else {
    ball_loc = w.ball.loc;
  }

  bool f = !IsInField(ball_loc + 0 * (ball_loc - last_ball_loc), -BallRadius, false);

  if (f) {
    cnt++;
  }
  else {
    cnt = 0;
  }
  fired = cnt > 1;

  if (fired) {
    if (vars.toucher.team != TeamBlue && vars.toucher.team != TeamYellow) {
      vars.toucher.team = RandomTeam();
    }

    vars.reset = true;
    vars.state = REF_WAIT_STOP;
    vars.cmd = SSL_Referee::STOP;
    vars.kicker.team = FlipTeam(vars.toucher.team);

    vector2f out_loc = OutOfBoundsLoc(last_ball_loc, ball_loc - last_ball_loc);
    bool own_half = (vars.toucher.team == TeamBlue) == (vars.blue_side * out_loc.x > 0);
    bool past_goal_line = fabs(out_loc.x) - fabs(out_loc.y) > FieldLengthH - FieldWidthH;
    bool crossed_midline = vars.touch_loc.x * out_loc.x < 0;

    // check for icing
    if (past_goal_line && crossed_midline && !own_half) {
      vars.next_cmd = teamCommand(INDIRECT_FREE, vars.kicker.team);
      vars.reset_loc = vars.touch_loc;
      setDescription("Icing (by %s %X)", TeamName(vars.toucher.team), vars.toucher.id);

      autoref_msg_valid = true;
      setReplayTimes(vars.touch_time, w.time);
      setFoulMessage(ssl::SSL_Autoref::RuleInfringement::CARPETING, vars.toucher);
    }
    // if not icing, then throw-in, corner kick, or goal kick
    else {
      vector2f pos;
      if (past_goal_line) {
        pos.set(sign(out_loc.x) * (FieldLengthH - (own_half ? 100 : 500)), sign(out_loc.y) * (FieldWidthH - 100));
        vars.next_cmd = teamCommand(DIRECT_FREE, vars.kicker.team);

        if (own_half) {
          setDescription("Corner kick %s -- (touched by %s %X)",
                         TeamName(FlipTeam(vars.toucher.team)),
                         TeamName(vars.toucher.team),
                         vars.toucher.id);
        }
        else {
          setDescription("Goal kick %s -- (touched by %s %X)",
                         TeamName(FlipTeam(vars.toucher.team)),
                         TeamName(vars.toucher.team),
                         vars.toucher.id);
        }
      }
      else {
        pos.set(out_loc.x, sign(out_loc.y) * (FieldWidthH - 100));
        vars.next_cmd = teamCommand(INDIRECT_FREE, vars.kicker.team);
        setDescription("Throw-in (touched by %s %X)", TeamName(vars.toucher.team), vars.toucher.id);
      }
      vars.reset_loc = pos;

      autoref_msg_valid = true;
      setReplayTimes(vars.touch_time, w.time);
      auto ball_out = autoref_msg.mutable_ball_out_of_field();
      ball_out->set_last_touch(vars.toucher.team == TeamBlue ? ssl::SSL_Autoref_Team_BLUE
                                                             : ssl::SSL_Autoref_Team_YELLOW);
      auto out_pos = ball_out->mutable_position();
      out_pos->set_x(vars.reset_loc.x);
      out_pos->set_y(vars.reset_loc.y);
    }
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
      vars.toucher.team = res.team;
      vars.toucher.id = res.robot_id;
      vars.touch_loc = w.ball.loc;  // TODO compute and use past touch location in detectors
      vars.touch_time = w.time;
      // setDescription("Ball touched by %s team", TeamName(vars.toucher.team));
      break;
    }
  }

  if (fired && vars.state == REF_RUN) {
    // check if the robot touched the ball while in one of the defense areas
    for (const auto &r : w.robots) {
      if (r.team != res.team || r.robot_id != res.robot_id) {
        continue;
      }

      bool own_side_positive_x = (vars.blue_side > 0) == (r.team == TeamBlue);
      double own_dist = DistToDefenseArea(r.loc, own_side_positive_x);
      auto &refbox = ref->getRefboxMessage();
      uint32_t goalie_id = (res.team == TeamBlue ? refbox.blue() : refbox.yellow()).goalie();

      // check own defense area
      if (res.robot_id != goalie_id) {
        if (own_dist < -MaxRobotRadius) {
          vars.state = REF_WAIT_STOP;
          vars.cmd = SSL_Referee::STOP;
          vars.next_cmd = teamCommand(PREPARE_PENALTY, res.team);

          setDescription("Multiple defenders (entire) (by %s %X)", TeamName(vars.toucher.team), vars.toucher.id);

          autoref_msg_valid = true;
          setReplayTimes(vars.touch_time - 1, w.time);
          setFoulMessage(ssl::SSL_Autoref::RuleInfringement::DEFENDER_DEFENSE_AREA_FULL, vars.toucher);
        }
        else if (own_dist < MaxRobotRadius) {
          vars.state = REF_WAIT_STOP;
          vars.cmd = SSL_Referee::STOP;
          vars.next_cmd = SSL_Referee::FORCE_START;

          setDescription("Multiple defenders (partial) (by %s %X)", TeamName(vars.toucher.team), vars.toucher.id);
          // TODO send card

          autoref_msg_valid = true;
          setReplayTimes(vars.touch_time - 1, w.time);
          setFoulMessage(ssl::SSL_Autoref::RuleInfringement::DEFENDER_DEFENSE_AREA_PARTIAL, vars.toucher);
        }
      }
      // check opponent defense area
      if (DistToDefenseArea(r.loc, !own_side_positive_x) < MaxRobotRadius) {
        vars.state = REF_WAIT_STOP;
        vars.kicker.team = FlipTeam(res.team);
        vars.cmd = SSL_Referee::STOP;
        vars.next_cmd = teamCommand(INDIRECT_FREE, vars.kicker.team);

        setDescription("Attacker in defense area");

        autoref_msg_valid = true;
        setReplayTimes(vars.touch_time - 1, w.time);
        setFoulMessage(ssl::SSL_Autoref::RuleInfringement::ATTACKER_DEFENSE_AREA, vars.toucher);
      }
    }
  }
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
      && vars.stage != SSL_Referee::NORMAL_SECOND_HALF_PRE
      && vars.stage != SSL_Referee::NORMAL_SECOND_HALF) {
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
    vars.toucher.clear();
    vars.cmd = vars.next_cmd;
    vars.next_cmd = SSL_Referee::HALT;
    setDescription("Starting kick: %s", SSL_Referee::Command_Name(vars.cmd).c_str());

    // reset the desired kick location to the current location, in case it didn't get moved before timing out
    vars.reset_loc = w.ball.loc;
    switch (vars.cmd) {
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
          vars.blue_side = GuessBlueSide(w);
        }

        vars.state = REF_WAIT_KICK;
        vars.kick_deadline = w.time + KickDeadline;
        break;

      case SSL_Referee::FORCE_START:
        vars.state = REF_RUN;
        break;

      case SSL_Referee::PREPARE_KICKOFF_BLUE:
      case SSL_Referee::PREPARE_KICKOFF_YELLOW:
        t0 = t0_bots = t0_ball = w.time;
        vars.next_cmd = SSL_Referee::NORMAL_START;
        vars.state = REF_WAIT_STOP;
        break;

      default:
        break;
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
    vars.blue_side = GuessBlueSide(w);

    RobotID offender = checkDefenseAreaDistanceInfraction(w);
    if (offender.isValid()) {
      vars.state = REF_WAIT_STOP;
      vars.cmd = SSL_Referee::STOP;
      vars.kicker.team = FlipTeam(vars.kicker.team);
      vars.next_cmd = teamCommand(INDIRECT_FREE, vars.kicker.team);

      setDescription("Too close to defense area at kick (%s %X)", TeamName(offender.team), offender.id);

      autoref_msg_valid = true;
      setReplayTimes(w.time - 2, w.time);
      setFoulMessage(ssl::SSL_Autoref::RuleInfringement::DEFENSE_AREA_DISTANCE, offender);
    }
    else {
      vars.state = REF_RUN;
      setDescription("Kick taken");
    }
  }
}

RobotID KickTakenEvent::checkDefenseAreaDistanceInfraction(const World &w) const
{
  bool positive_x = (vars.blue_side > 0) != (vars.kicker.team == TeamBlue);
  for (const auto &robot : w.robots) {
    if (robot.team != vars.kicker.team) {
      continue;
    }

    if (DistToDefenseArea(robot.loc, positive_x) < MaxRobotRadius + 200) {
      return RobotID(robot.team, robot.robot_id);
    }
  }
  return RobotID();
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

    autoref_msg_valid = true;
    autoref_msg.set_lack_of_progress(true);
    setReplayTimes(w.time - 3, w.time);
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
  int MAX_EXTRAPOLATE_FRAMES = 5;
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

      frames = min(lost_cnt, MAX_EXTRAPOLATE_FRAMES);
      ball_loc = p0 + v0 * (frames + ball_history.size() - 1) * FramePeriod;

      vector2f b(ball_loc.x, fabs(ball_loc.y));
      vector2f h(ball_history[0].v.x, fabs(ball_history[0].v.y));
      // check if the hallucinated trajectory crosses a goal wall
      if (segment_intersects(
            b, h, vector2f(FieldLengthH, GoalWidthH), vector2f(FieldLengthH + GoalDepth, GoalWidthH))) {
        return;
      }
    }
  }
  else {
    ball_loc = w.ball.loc;
  }

  fired = ((fabs(ball_loc.y) < GoalWidthH) && (fabs(ball_loc.x) > FieldLengthH)
           && (fabs(ball_loc.x) < FieldLengthH + GoalDepth));

  if (fired) {
    Team scoring_team = (vars.blue_side * ball_loc.x > 0) ? TeamYellow : TeamBlue;
    vars.team[scoring_team].score++;

    vars.cmd = SSL_Referee::STOP;
    vars.next_cmd = teamCommand(GOAL, scoring_team);
    vars.kicker.team = FlipTeam(scoring_team);
    vars.state = REF_DELAY_GOAL;
    vars.reset = true;
    vars.reset_loc.set(0, 0);
    setDescription("Goal scored by %s team", TeamName(scoring_team));

    autoref_msg_valid = true;
    auto goal = autoref_msg.mutable_goal();
    goal->set_scoring_team(scoring_team == TeamBlue ? ssl::SSL_Autoref::BLUE : ssl::SSL_Autoref::YELLOW);
    setReplayTimes(vars.touch_time, w.time);
  }
}

const char DelayDoneEvent::ID;

void DelayDoneEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_DELAY_GOAL) {
    cnt = 0;
    return;
  }

  fired = cnt++ > 10;
  if (fired) {
    vars.state = REF_WAIT_STOP;
    vars.cmd = teamCommand(GOAL, FlipTeam(vars.kicker.team));
    vars.next_cmd = teamCommand(PREPARE_KICKOFF, vars.kicker.team);
    vars.reset = true;
    vars.reset_loc.set(0, 0);
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

        autoref_msg_valid = true;
        // TODO compute start time
        setReplayTimes(w.time - 3, w.time);
        setFoulMessage(ssl::SSL_Autoref::RuleInfringement::DRIBBLING, RobotID(r.team, r.robot_id));
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

    default:
      break;
  }
  setDescription("Stage ended: %s", SSL_Referee::Stage_Name(vars.stage).c_str());
  vars.stage = NextStage(vars.stage);
}

const char TooManyRobotsEvent::ID;

void TooManyRobotsEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_RUN && vars.state != REF_WAIT_STOP) {
    return;
  }

  const SSL_Referee &msg = ref->getRefboxMessage();
  int max_blue = MaxTeamRobots - msg.blue().yellow_card_times_size() - msg.blue().red_cards();
  int max_yellow = MaxTeamRobots - msg.yellow().yellow_card_times_size() - msg.yellow().red_cards();

  int n_blue = 0, n_yellow = 0;
  for (const auto &robot : w.robots) {
    if (robot.team == TeamBlue) {
      n_blue++;
    }
    else {
      n_yellow++;
    }
  }

  // printf("br %d by %d  yr %d yy %d\n",
  //        msg.blue().red_cards(),
  //        msg.blue().yellow_card_times_size(),
  //        msg.yellow().red_cards(),
  //        msg.yellow().yellow_card_times_size());
  // printf("b %d/%d  y %d/%d\n", n_blue, max_blue, n_yellow, max_yellow);

  Team offending_team = TeamNone;

  if (n_blue > max_blue) {
    blue_frames++;

    if (blue_frames > 300) {
      offending_team = TeamBlue;
      blue_frames = 0;
      fired = true;
      setDescription("Blue team has %d robots (max %d)", n_blue, max_blue);
    }
  }
  else if (n_yellow > max_yellow) {
    yellow_frames++;
    if (yellow_frames > 300) {
      offending_team = TeamYellow;
      yellow_frames = 0;
      fired = true;
      setDescription("Yellow team has %d robots (max %d)", n_yellow, max_yellow);
    }
  }

  if (fired) {
    vars.cmd = SSL_Referee::STOP;
    vars.next_cmd = SSL_Referee::FORCE_START;

    autoref_msg_valid = true;
    setReplayTimes(w.time - 3, w.time);
    setFoulMessage(ssl::SSL_Autoref::RuleInfringement::NUMBER_OF_PLAYERS, offending_team);
  }
}

const char RobotSpeedEvent::ID;

void RobotSpeedEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_WAIT_STOP) {
    return;
  }

  for (const auto &robot : w.robots) {
    if (robot.vel.length() > GameOffRobotSpeedLimit) {
      violation_frames[static_cast<int>(robot.team)]++;
    }
  }

  for (int team = 0; team < NumTeams; team++) {
    if (violation_frames[team] > 2 * FrameRate) {
      fired = true;
      violation_frames[team] = 0;

      setDescription("%s team moved too fast during game off", TeamName(static_cast<Team>(team), true));
      break;

      autoref_msg_valid = true;
      setReplayTimes(w.time - 3, w.time);
      setFoulMessage(ssl::SSL_Autoref::RuleInfringement::STOP_SPEED, static_cast<Team>(team));
    }
  }
}

const char StopDistanceEvent::ID;

void StopDistanceEvent::_process(const World &w, bool ball_z_valid, float ball_z)
{
  if (vars.state != REF_WAIT_STOP) {
    return;
  }

  float dist = GameOffRobotDistanceLimit + MaxRobotRadius;

  for (const auto &robot : w.robots) {
    if ((robot.loc - w.ball.loc).length() < dist) {
      violation_frames[static_cast<int>(robot.team)]++;
    }
  }

  for (int team = 0; team < NumTeams; team++) {
    if (violation_frames[team] > 10 * FrameRate) {
      fired = true;
      violation_frames[team] = 0;

      setDescription("%s team was too close to the ball during game off", TeamName(static_cast<Team>(team), true));
      break;

      autoref_msg_valid = true;
      setReplayTimes(w.time - 3, w.time);
      setFoulMessage(ssl::SSL_Autoref::RuleInfringement::STOP_BALL_DISTANCE, static_cast<Team>(team));
    }
  }
}
