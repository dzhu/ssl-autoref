#include <wx/wx.h>
#include <deque>

#include "referee_call.pb.h"

class wxAutorefHistoryPanel : public wxPanel
{
  int max_items;
  std::deque<RefereeCall> history;
  wxSizer *sizer;

public:
  wxAutorefHistoryPanel() : wxPanel(), max_items(0), sizer(nullptr)
  {
  }

  wxAutorefHistoryPanel(wxWindow *parent, wxWindowID id, int _max_items)
      : wxPanel(parent, id), sizer(new wxBoxSizer(wxVERTICAL)), history(5), max_items(_max_items)
  {
    SetSizer(sizer);
  }

  void addUpdate(wxCommandEvent &event);

  DECLARE_DYNAMIC_CLASS(wxEventsHistory);
};
