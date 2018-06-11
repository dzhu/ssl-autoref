#include "tracker.h"

static constexpr int AFFINITY_PERSIST = 30;

int Tracker::ObjectTracker::mergeObservations()
{
  int last_affinity = affinity;
  if (affinity < 0 || !obs[affinity].valid) {
    affinity = -1;
    for (int o = 0; o < MaxCameras; o++) {
      if (obs[o].valid) {
        affinity = o;
        break;
      }
    }
  }

  if (affinity < 0 && last_affinity >= 0 && obs[last_affinity].last_valid < AFFINITY_PERSIST) {
    affinity = last_affinity;
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
      || history[0].time - history[history.size() - 1].time > 2 * VEL_SAMPLES * Constants::FramePeriod) {
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
  static const bool debug = false;

  int camera = d.camera_id();

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
  last_capture_time = time;

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
    // obs.last_valid = 0;
    obs.time = time;
    obs.conf = r.confidence();
    obs.loc.set(r.x(), r.y());
    obs.angle = r.orientation();
  }
  for (const auto &r : d.robots_yellow()) {
    Observation &obs = robots[TeamYellow][r.robot_id()].obs[camera];
    obs.valid = true;
    // obs.last_valid = 0;
    obs.time = time;
    obs.conf = r.confidence();
    obs.loc.set(r.x(), r.y());
    obs.angle = r.orientation();
  }

  // take the ball from this camera that is closest to the last position of the
  // ball and not too far away
  float max_dist = HUGE_VALF;
  vector2f last_ball_loc(0, 0);
  if (ball.affinity >= 0) {
    const Observation &last_ball = ball.obs[ball.affinity];
    if (last_ball.last_valid < 30) {
      max_dist = 10000 * Constants::FramePeriod * last_ball.last_valid * 100;
      last_ball_loc = last_ball.loc;
    }
    if (debug) {
      printf("ball affinity: %d  last_valid: %d  last ball loc: <%7.3f,%7.3f>  max_dist %.3f\n",
             ball.affinity,
             last_ball.last_valid,
             V2COMP(last_ball.loc),
             max_dist);
    }
  }
  else {
    if (debug) {
      puts("no ball affinity");
    }
  }
  if (d.balls().size() > 0) {
    bool found = false;
    float min_dist = HUGE_VALF;
    SSL_DetectionBall closest;
    for (const auto &b : d.balls()) {
      float d = dist(last_ball_loc, vector2f(b.x(), b.y()));
      if (d < min_dist && d < max_dist) {
        closest = b;
        min_dist = d;
        found = true;
      }
    }

    if (found) {
      Observation &obs = ball.obs[camera];
      obs.valid = true;
      // obs.last_valid = 0;
      obs.time = time;
      obs.conf = closest.confidence();
      obs.loc.set(closest.x(), closest.y());

      if (debug) {
        printf("camera %d seen ball <%7.3f,%7.3f>  dist %7.3f\n", camera, V2COMP(obs.loc), min_dist);
      }
    }
  }

  if (!cameras_seen[camera]) {
    num_cameras_seen++;
    cameras_seen[camera] = true;
  }

  ready = (num_cameras_seen == num_cameras);
  if (ready) {
    if (debug) {
      puts("\nready\n");
    }
    // condense all observations and convert to World object
    makeWorld();

    // forget all previous observations
    for (auto &team_robots : robots) {
      for (auto &robot : team_robots) {
        for (auto &obs : robot.obs) {
          if (obs.valid) {
            obs.valid = false;
            obs.last_valid = 1;
          }
          else {
            obs.last_valid++;
          }
        }
      }
    }

    for (auto &obs : ball.obs) {
      if (obs.valid) {
        obs.valid = false;
        obs.last_valid = 1;
      }
      else {
        obs.last_valid++;
      }
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

  world.time = last_capture_time;  // GetTimeMicros() / 1e6;
  for (int team = 0; team < NumTeams; team++) {
    for (int id = 0; id < MaxRobotIds; id++) {
      ObjectTracker &robot = robots[team][id];
      if (robot.mergeObservations() >= 0) {
        const Observation &obs = robot.obs[robot.affinity];
        WorldRobot wr;
        wr.conf = (1 || obs.last_valid < 2) * obs.conf;
        wr.loc = obs.loc;
        wr.angle = obs.angle;
        wr.robot_id.set(static_cast<Team>(team), id);
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
    wb.conf = (1 || obs.last_valid < 2) * obs.conf;
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
