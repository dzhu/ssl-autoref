#include "touches.h"

bool AccelProcessor::proc(const World &w, CollideResult &res)
{
  history.add(w);

  double t = w.time;
  if (w.ball.conf < .1) {
    return false;
  }

  if (history.size() < 3) {
    return false;
  }

  vector2f delta = history[-1].ball.loc - (history[0].ball.loc + history[-2].ball.loc) / 2;
  double accel = delta.length() * Constants::FrameRate * Constants::FrameRate;

  if (accel < 2000) {
    return false;
  }

  vector2f ball_pt = history[-1].ball.loc;
  double closest_dist = HUGE_VALF;
  WorldRobot closest_robot;
  for (const auto &r : w.robots) {
    if (dist(r.loc, ball_pt) < closest_dist) {
      closest_dist = dist(r.loc, ball_pt);
      closest_robot = r;
    }
  }

  bool near = closest_dist < Constants::MaxRobotRadius + Constants::BallRadius + 10;
  if (!near) {
    return false;
  }

  res.robot_id = closest_robot.robot_id;
  res.time = history[-1].time;

  return true;
}

bool LineCheckProcessor::proc(const World &w, CollideResult &res)
{
  last++;
  history.add(w);

  double t = w.time;
  if (w.ball.conf < .1) {
    return false;
  }

  if (history.size() < 6) {
    return false;
  }

  // check whether first three and last three make good different fits
  if (!(line3(history[-5], history[-4], history[-3]) && line3(history[-2], history[-1], history[0])
        && !line3(history[-5], history[-3], history[0]))) {
    return false;
  }

  // look for nearby robot
  vector2f ball_pt
    = intersection(history[-5].ball.loc, history[-3].ball.loc, history[-2].ball.loc, history[0].ball.loc);

  double closest_dist = HUGE_VALF;
  WorldRobot closest_robot;
  const World &old = history[-3];

  if (old.robots.empty()) {
    return false;
  }

  for (const auto &r : old.robots) {
    if (dist(r.loc, ball_pt) < closest_dist) {
      closest_dist = dist(r.loc, ball_pt);
      closest_robot = r;
    }
  }

  res.robot_id = closest_robot.robot_id;

  // make sure robot comes near ball at some point (TODO use interframe locations)
  bool near = false;
  near = (closest_dist < Constants::MaxRobotRadius + Constants::BallRadius + 3);
  // TODO fix this
  // for (int i = -5; i <= 0; i++) {
  //   if (dist(history[i].robots[closest_ind].loc, history[i].ball.loc) < Constants::MaxRobotRadius +
  //   Constants::BallRadius + 30) {
  //     near = true;
  //     break;
  //   }
  // }

  if (!near || last < 3) {
    return false;
  }

  // TODO something more accurate?
  res.time = history[-2].time;
  last = 0;
  return true;
}

bool RobotDistProcessor::proc(const World &w, CollideResult &res)
{
  last++;
  double t = w.time;
  const WorldBall &ball = w.ball;

  if (!ball.visible()) {
    return false;
  }

  bool found = false;

  for (const auto &r : w.robots) {
    if (!r.visible()) {
      continue;
    }
    vector2f v = ball.loc - r.loc;

    RunningQueue<tvec, 6> &hist = robot_history[r.robot_id.team][r.robot_id.id];
    hist.add(tvec(t, v));

    if (hist.size() < 6) {
      continue;
    }

    // fprintf(stderr, "---- %d\n", r.robot_id);
    // for(int i = -5; i <= 0; i++)
    //   fprintf(stderr, "%f %.0f,%.0f\n", hist[i].t, V2COMP(hist[i].v));

    if (t - hist[-5].t > 15 * Constants::FramePeriod) {
      continue;
    }

    // check that the last three and three before that are in straight lines
    if (!hist[-4].between(hist[-5], hist[-3]) || !hist[-1].between(hist[0], hist[-2])) {
      continue;
    }

    // check that they're not all in a straight line
    if (cosine(hist[-5].v - hist[-3].v, hist[0].v - hist[-2].v) < -.99) {
      continue;
    }

    vector2f inter = intersection(hist[-5].v, hist[-3].v, hist[-2].v, hist[0].v);

    double t0 = point_on_segment_t(hist[-5].v, hist[-3].v, inter);
    double t1 = point_on_segment_t(hist[0].v, hist[-2].v, inter);
    if (t0 < .9 || t1 < .9) {
      continue;
    }

    if (inter.length() < Constants::MaxRobotRadius + Constants::BallRadius + 30 && inter.length() < hist[-5].v.length()
        && inter.length() < hist[0].v.length()) {
      res.robot_id = r.robot_id;
      res.time = t - 2 * Constants::FramePeriod;
      found = true;
    }
  }

  if (found && last > 3) {
    last = 0;
    return true;
  }

  return false;
}

bool BackTrackProcessor::proc(const World &w, CollideResult &res)
{
  last++;
  double t = w.time;
  const WorldBall &ball = w.ball;

  if (t0 == 0) {
    t0 = t;
  }

  if (ball.conf < .1) {
    return false;
  }

  bool found = false;
  for (const auto &r : w.robots) {
    if (!r.visible()) {
      continue;
    }
    vector2f v = ball.loc - r.loc;

    RunningQueue<tvec, HIST_LEN> &hist = robot_history[r.robot_id.team][r.robot_id.id];
    hist.add(tvec(t - t0, v));

    // not enough samples yet; give up
    if (hist.size() < HIST_LEN) {
      continue;
    }

    // recent samples are too sparse; give up
    if (t - (t0 + hist[-HIST_LEN + 1].t) > 3 * HIST_LEN * Constants::FramePeriod) {
      continue;
    }

    // check whether ball passed "through" the robot before the last few
    // samples (probably was chipped over)
    bool through = false;
    for (int i = -HIST_LEN + 1; i < -VEL_SAMPLES + 1; i++) {
      double seg_dist = distance_to_segment(hist[i].v, hist[i + 1].v, vector2f(0, 0));
      if (seg_dist < Constants::MaxRobotRadius - Constants::BallRadius) {
        through = true;
        break;
      }
    }
    if (through) {
      continue;
    }

    // estimate velocity of ball from last VEL_SAMPLES samples and extrapolate
    // backward before that
    vector2f p0, v0;
    linvel(hist, p0, v0, VEL_SAMPLES);

    // printf("%.3f || p %.0f,%.0f || v %.3f,%.3f\n", t, V2COMP(p0), V2COMP(v0));

    for (int i = -2; i < 0; i++) {
      vector2f old_pos(p0 + i * Constants::FramePeriod * v0);
      if (old_pos.length() < Constants::MaxRobotRadius + Constants::BallRadius - 10) {
        res.robot_id = r.robot_id;
        res.time = t - 2 * Constants::FramePeriod;
        found = true;
      }
    }
  }

  if (found && last > 3) {
    last = 0;
    return true;
  }

  return false;
}

// float AccelProcessor::BallNearDist  = Constants::MaxRobotRadius + Constants::BallRadius + 70.0;
// float AccelProcessor::BallCloseDist = Constants::MaxRobotRadius + Constants::BallRadius + 40.0;
// float AccelProcessor::BallLooseDist = 500.0;
// float AccelProcessor::BallTouchAcceleration = 50000.0;

// AccelProcessor::AccelProcessor(){

// }
