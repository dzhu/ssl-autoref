#include <cstdio>
#include <ctime>

#include "autoref.h"
#include "events.h"

BaseAutoref::BaseAutoref()
{
  have_geometry = new_refbox = false;
  game_on = false;
  new_stage = new_cmd = false;
  cmd_counter = 0;
}

void BaseAutoref::updateGeometry(const SSL_GeometryData &g)
{
  if (!have_geometry) {
    puts("got geometry!");
  }
  have_geometry = true;
  geometry.CopyFrom(g);
}

void BaseAutoref::updateVision(const SSL_DetectionFrame &d)
{
  // puts("vision");
  tracker.updateVision(d);
  World w;
  if (tracker.getWorld(w)) {
    doEvents(w);
  }
}

void BaseAutoref::updateReferee(const SSL_Referee &r)
{
  new_refbox = true;
  refbox_message = r;
}

bool BaseAutoref::isMessageReady()
{
  return tracker.isReady();
}

bool BaseAutoref::isRemoteReady()
{
  return new_cmd || new_stage;
}

void convertTeamInfo(const AutorefVariables::TeamInfo &in, SSL_Referee::TeamInfo &out)
{
  out.set_name("");  // TODO
  out.set_score(in.score);
  out.set_red_cards(0);
  out.set_yellow_cards(0);
  out.set_timeouts(in.timeout_n);
  out.set_timeout_time(in.timeout_t);
  out.set_goalie(0);  // TODO
}

SSL_Referee BaseAutoref::makeMessage()
{
  SSL_Referee msg;
  uint64_t time = GetTimeMicros();
  msg.set_packet_timestamp(time);

  msg.set_stage(vars.stage);
  if (vars.stage == SSL_Referee::NORMAL_FIRST_HALF || vars.stage == SSL_Referee::NORMAL_SECOND_HALF
      || vars.stage == SSL_Referee::NORMAL_HALF_TIME) {
    msg.set_stage_time_left(static_cast<uint64_t>(1000000 * vars.stage_end) - time);
  }
  else {
    msg.set_stage_time_left(0);
  }

  msg.set_command(vars.cmd);
  msg.set_command_counter(cmd_counter);
  msg.set_command_timestamp(cmd_timestamp);

  convertTeamInfo(vars.team[TeamBlue], *(msg.mutable_blue()));
  convertTeamInfo(vars.team[TeamYellow], *(msg.mutable_yellow()));

  return msg;
}

SSL_RefereeRemoteControlRequest BaseAutoref::makeRemote()
{
  SSL_RefereeRemoteControlRequest msg;
  msg.set_message_id(0);
  msg.set_last_command_counter(refbox_message.command_counter());

  if (new_cmd) {
    msg.set_command(vars.cmd);
  }
  else if (new_stage) {
    msg.set_stage(vars.stage);
  }
  return msg;
}
