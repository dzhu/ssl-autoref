#include "tracker.h"

int Tracker::ObjectTracker::mergeObservations()
{
  if (affinity < 0 || !obs[affinity].valid) {
    affinity = -1;
    for (int o = 0; o < MaxCameras; o++) {
      if (obs[o].valid) {
        affinity = o;
        break;
      }
    }
  }
  if (affinity != -1) {
    history.push_back(obs[affinity]);
    if (history.size() > VEL_SAMPLES) {
      history.pop_front();
    }
  }
  return affinity;
}

vector2f Tracker::ObjectTracker::fitVelocity()
{
  // TODO dedup
  if (history.size() < VEL_SAMPLES
      || history[0].time - history[history.size() - 1].time > 2 * VEL_SAMPLES * FramePeriod) {
    return vector2f(0, 0);
  }

  int samples = VEL_SAMPLES;
  double t0 = history[0].time;
  double sxx = 0, sxy = 0, sx = 0, sy = 0;
  double txx = 0, txy = 0, tx = 0, ty = 0;
  for (const auto &h : history) {
    double t = h.time - t0;
    double x = h.loc.x;
    double y = h.loc.y;

    sxx += t * t;
    sxy += t * x;
    sx += t;
    sy += x;

    txx += t * t;
    txy += t * y;
    tx += t;
    ty += y;
  }
  double vx = (samples * sxy - sx * sy) / (samples * sxx - sx * sx);
  double vy = (samples * txy - tx * ty) / (samples * txx - tx * tx);

  return vector2f(vx, vy);
}

void Tracker::updateVision(const SSL_DetectionFrame &d)
{
  int camera = d.camera_id();

  // if (num_cameras <= 0) {
  //   num_cameras = 4;
  // }

  // count how many cameras are sending, at first
  if (num_cameras <= 0) {
    if (!cameras_seen[camera]) {
      num_cameras_seen++;
      cameras_seen[camera] = true;
    }

    if (frames++ >= 100) {
      num_cameras = num_cameras_seen;

      for (bool &s : cameras_seen) {
        s = false;
      }
      num_cameras_seen = 0;
    }
    else {
      return;
    }
  }

  double time = d.t_capture();

  // static uint64_t times[4] = {0};
  // uint64_t tm = GetTimeMicros();
  // if (times[camera] > 0) {
  //   printf("%d %d\n", camera, tm - times[camera]);
  // }
  // times[camera] = tm;

  WorldRobot wr;
  for (const auto &r : d.robots_blue()) {
    Observation &obs = robots[TeamBlue][r.robot_id()].obs[camera];
    obs.valid = true;
    obs.time = time;
    obs.conf = r.confidence();
    obs.loc.set(r.x(), r.y());
    obs.angle = r.orientation();
  }
  for (const auto &r : d.robots_yellow()) {
    Observation &obs = robots[TeamYellow][r.robot_id()].obs[camera];
    obs.valid = true;
    obs.time = time;
    obs.conf = r.confidence();
    obs.loc.set(r.x(), r.y());
    obs.angle = r.orientation();
  }

  // take the ball from this camera that is closest to the last position of the ball
  if (d.balls().size() > 0) {
    float min_dist = HUGE_VALF;
    SSL_DetectionBall closest;
    for (const auto &b : d.balls()) {
      if (dist(last_ball, vector2f(b.x(), b.y())) < min_dist) {
        closest = b;
      }
    }

    Observation &obs = ball.obs[camera];
    obs.valid = true;
    obs.time = time;
    obs.conf = closest.confidence();
    obs.loc.set(closest.x(), closest.y());
  }

  if (!cameras_seen[camera]) {
    num_cameras_seen++;
    cameras_seen[camera] = true;
  }

  ready = (num_cameras_seen == num_cameras);
  if (ready) {
    // condense all observations and convert to World object
    makeWorld();

    // forget all previous observations
    for (auto &team_robots : robots) {
      for (auto &robot : team_robots) {
        for (auto &obs : robot.obs) {
          obs.valid = false;
        }
      }
    }

    for (auto &obs : ball.obs) {
      obs.valid = false;
    }

    for (bool &s : cameras_seen) {
      s = false;
    }
    num_cameras_seen = 0;
  }
}

void Tracker::makeWorld()
{
  world.reset();

  bool debug = false;

  world.time = GetTimeMicros() / 1e6;
  for (int team = 0; team < NumTeams; team++) {
    for (int id = 0; id < MaxRobotIds; id++) {
      ObjectTracker &robot = robots[team][id];
      if (robot.mergeObservations() >= 0) {
        const Observation &obs = robot.obs[robot.affinity];
        WorldRobot wr;
        wr.conf = obs.conf;
        wr.loc = obs.loc;
        wr.angle = obs.angle;
        wr.team = static_cast<Team>(team);
        wr.robot_id = id;
        wr.vel = robot.fitVelocity();
        world.robots.push_back(wr);

        if (debug) {
          printf("[%c %x %d <%.0f,%.0f> <%.0f,%.0f>] ", "by"[team], id, robot.affinity, V2COMP(wr.loc), V2COMP(wr.vel));
        }
      }
    }
  }

  if (ball.mergeObservations() >= 0) {
    const Observation &obs = ball.obs[ball.affinity];
    WorldBall &wb = world.ball;
    wb.conf = obs.conf;
    wb.loc = obs.loc;
    wb.vel = ball.fitVelocity();

    if (debug) {
      printf("[ball %d <%.0f,%.0f> <%.0f,%.0f>] ", ball.affinity, V2COMP(wb.loc), V2COMP(wb.vel));
    }
  }
  if (debug) {
    puts("");
  }
}
