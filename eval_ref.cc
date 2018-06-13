#include <cstdio>
#include <ctime>

#include "eval_ref.h"
#include "events.h"

#include <fstream>
#include <iostream>

void logMessage(Message &drawing, ostream &out)
{
  uint32_t sz = drawing.ByteSize();
  printf("logging %d bytes\n", sz);
  out.write(reinterpret_cast<const char *>(&sz), 4);
  drawing.SerializeToOstream(&out);
  out.flush();
}

EvaluationAutoref::EvaluationAutoref(bool verbose_) : BaseAutoref(), verbose(verbose_)
{
  vars.state = REF_RUN;
  vars.stage = SSL_Referee::NORMAL_FIRST_HALF;

  addEvent<RefboxUpdateEvent>();
  addEvent<KickTakenEvent>();
  addEvent<BallSpeedEvent>();
  addEvent<GoalScoredEvent>();
  addEvent<BallExitEvent>();
  addEvent<BallTouchedEvent>();
  addEvent<LongDribbleEvent>();
  addEvent<TooManyRobotsEvent>();
  addEvent<RobotSpeedEvent>();
  addEvent<StopDistanceEvent>();
  addEvent<BallStuckEvent>();

  log = new ofstream("autoref.dlog");
}

bool EvaluationAutoref::doEvents(const World &w, bool ball_z_valid, float ball_z)
{
  static int n = 0;
  bool ret = false;

  SSL_Referee::Stage last_stage = vars.stage;
  SSL_Referee::Command last_command = vars.cmd;

  message_ready = false;
  state_updated = false;

  game_event.Clear();

  // printf("-- state: %s\n", ref_state_names[vars.state]);
  for (auto it = events.begin(); it < events.end(); ++it) {
    AutorefEvent *ev = *it;
    ev->process(w, ball_z_valid, ball_z);

    if (ev->firingNew()) {
      state_updated = true;

      AutorefVariables new_vars = ev->getUpdate();

      auto drawings = ev->getDrawings();
      // for (auto d : drawings) {
      //   printf("drawing: %.3f -> %.3f  %d %d %d\n",
      //          d.timestamp(),
      //          d.end_timestamp(),
      //          d.line_size(),
      //          d.circle_size(),
      //          d.rectangle_size());
      //   logMessage(d, *log);
      // }

      const SSL_Referee &ref = getRefboxMessage();

      char time_buf[256];
      uint64_t t0 = ref.packet_timestamp();
      time_t tt = t0 / 1e6;
      if (tt == 0) {
        tt = w.time;
      }
      tm *st = localtime(&tt);
      strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", st);

      if (ev->getMessage(game_event)) {
        message_ready = true;
      }

      // if (ev->getDescription().size() > 0) {
      //   update.set_description(ev->getDescription());
      // }
      // if (new_vars.cmd != vars.cmd) {
      //   update.set_command(new_vars.cmd);

      //   if (new_vars.cmd == SSL_Referee::STOP || new_vars.cmd == SSL_Referee::PREPARE_KICKOFF_YELLOW
      //       || new_vars.cmd == SSL_Referee::PREPARE_KICKOFF_BLUE
      //       || new_vars.cmd == SSL_Referee::PREPARE_PENALTY_YELLOW
      //       || new_vars.cmd == SSL_Referee::PREPARE_PENALTY_BLUE) {
      //     update.set_next_command(new_vars.next_cmd);
      //   }
      // }

      if (verbose) {
        printf("\n%ld.%06ld %s event fired: %s\n", t0 / 1000000, t0 % 1000000, time_buf, ev->name());

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
        PRINT_DIFF("%d", kicker.team);
        PRINT_DIFF("%X", kicker.id);
        PRINT_DIFF("%d", toucher.id);
        PRINT_DIFF("%d", blue_side);
      }

      // print readable updates
      if (ev->getDescription().size() > 0) {
        printf(
          "\n%ld.%06ld %s \x1b[32;1m%s\x1b[m\n", t0 / 1000000, t0 % 1000000, time_buf, ev->getDescription().c_str());
      }
      // else{
      //   printf("\n%s \x1b[32;1mEvent fired: %s\x1b[m\n", time_buf, ev->name());
      // }
      if (new_vars.reset) {
        int color = 33;
        bool is_bold = false;
        printf("\n%s \x1b[%d;%dmPlease move the ball to <%.0f,%.0f>!\x1b[m\n",
               time_buf,
               is_bold,
               color,
               V2COMP(new_vars.reset_loc));
      }

      vars = new_vars;
      vars.reset = false;
    }
  }

  new_stage = (vars.stage != last_stage);
  new_cmd = (vars.cmd != last_command) && (vars.cmd != refbox_message.command());

  fflush(stdout);
  return ret;
}
