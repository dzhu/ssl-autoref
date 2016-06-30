#pragma once

#include <wx/gbsizer.h>
#include <wx/wx.h>

#include "referee.pb.h"
#include "referee_call.pb.h"

class wxRefereeHistoryItem : public wxPanel
{
  wxSizer *sizer;
  RefereeCall update;
  bool emph;

  wxStaticText *command_label, *command_text, *next_label, *next_text;
  wxStaticText *description_label, *description_text, *stage_time_label, *stage_time_text;
  ~wxRefereeHistoryItem();

  wxTimer *animate_timer;

  uint64_t create_time;

public:
  wxRefereeHistoryItem() : wxPanel(), sizer(nullptr), emph(false)
  {
  }

  wxRefereeHistoryItem(wxWindow *parent, wxWindowID id, RefereeCall _update);

  void setEmph(bool e);

  void OnTimer(wxTimerEvent &event);

  DECLARE_EVENT_TABLE()
  DECLARE_DYNAMIC_CLASS(wxRefereeHistoryItem)
};
