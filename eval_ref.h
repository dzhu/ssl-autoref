#pragma once
#include <cstdint>

#include <deque>
#include <map>

#include <google/protobuf/text_format.h>

#include "messages_robocup_ssl_wrapper.pb.h"
#include "rcon.pb.h"
#include "referee.pb.h"

#include "base_ref.h"

#include "constants.h"
#include "events.h"
#include "touches.h"
#include "tracker.h"

using namespace google::protobuf;

class EvaluationAutoref : public BaseAutoref
{
  bool doEvents(const World &w, bool ball_z_valid = false, float ball_z = 0);
  bool verbose;

public:
  EvaluationAutoref(bool verbose_);
};
