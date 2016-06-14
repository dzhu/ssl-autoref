#include <cstdio>
#include <ctime>

#include "autoref.h"
#include "events.h"

Autoref::Autoref(bool verbose_) : verbose(verbose_)
{
  addEvent<InitEvent>();
  addEvent<RobotsStartedEvent>();
  addEvent<KickReadyEvent>();  // must be before anything that can transition
                               // from game-on to game-off, so it doesn't miss
                               // any transitions and can always reset properly
  addEvent<BallSpeedEvent>();
  addEvent<BallStuckEvent>();
  addEvent<GoalScoredEvent>();
  addEvent<BallExitEvent>();
  addEvent<DelayDoneEvent>();
  addEvent<KickTakenEvent>();
  addEvent<KickExpiredEvent>();
  addEvent<BallTouchedEvent>();
  addEvent<LongDribbleEvent>();
  addEvent<StageTimeEndedEvent>();
}

bool Autoref::doEvents(const World &w, bool ball_z_valid, float ball_z)
{
  static int n = 0;
  bool ret = false;

  bool any_fired = true;

  SSL_Referee::Stage last_stage = vars.stage;
  SSL_Referee::Command last_command = vars.cmd;

  while (any_fired) {
    any_fired = false;
    for (auto it = events.begin(); it < events.end(); ++it) {
      AutorefEvent *ev = *it;
      ev->process(w, ball_z_valid, ball_z);

      if (ev->firingNew()) {
        AutorefVariables new_vars = ev->getUpdate();

        char time_buf[256];
        time_t tt = w.time;
        tm *st = localtime(&tt);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", st);

        // print detailed internal information about firing event
        if (verbose) {
          printf("\n%s.%03d event fired: %s\n", time_buf, static_cast<int>(1000 * (w.time - tt)), ev->name());

#define PRINT_DIFF(format, field)                          \
  if (new_vars.field != vars.field) {                      \
    printf("-- " #field ": " format "\n", new_vars.field); \
  }
#define PRINT_DIFF_PROTO_STR(field, type)                                            \
  if (new_vars.field != vars.field) {                                                \
    printf("-- " #field ": %s\n", SSL_Referee::type##_Name(new_vars.field).c_str()); \
  }

          PRINT_DIFF_PROTO_STR(cmd, Command);
          PRINT_DIFF_PROTO_STR(next_cmd, Command);
          PRINT_DIFF_PROTO_STR(stage, Stage);

          if (new_vars.reset) {
            printf("-- reset loc: <%.2f,%.2f>\n", V2COMP(new_vars.reset_loc));
          }
          if (new_vars.state != vars.state) {
            printf("-- state: %s\n", ref_state_names[new_vars.state]);
          }

          PRINT_DIFF("%.3f", stage_end);
          PRINT_DIFF("%.3f", kick_deadline);
          PRINT_DIFF("%d", kick_team);
          PRINT_DIFF("%X", kicker_id);
          PRINT_DIFF("%d", touch_team);
          PRINT_DIFF("%d", blue_side);
        }

        // print readable updates
        if (ev->getDescription().size() > 0) {
          printf("\n%s \x1b[32;1m%s\x1b[m\n", time_buf, ev->getDescription().c_str());
        }
        if (new_vars.reset) {
          printf("\n%s \x1b[33;1mPlease move the ball to <%.0f,%.0f>!\x1b[m\n", time_buf, V2COMP(new_vars.reset_loc));
        }

        vars = new_vars;
        vars.reset = false;

        any_fired = true;
        break;
      }
    }
  }

  new_stage = (vars.stage != last_stage);
  new_cmd = (vars.cmd != last_command);

  fflush(stdout);
  return ret;
}
