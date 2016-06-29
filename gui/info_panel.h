#include <deque>

#include <wx/gbsizer.h>
#include <wx/wx.h>

#include "referee_update.pb.h"

class wxGameInfoPanel : public wxPanel
{
  wxSizer *sizer;
  wxStaticText *blue_name_text, *yellow_name_text;
  wxStaticText *blue_score_text, *yellow_score_text;
  wxStaticText *stage_text, *time_text, *command_text;

public:
  wxGameInfoPanel() : wxPanel()
  {
  }

  wxGameInfoPanel(wxWindow *parent, wxWindowID id);
  void addUpdate(wxCommandEvent &event);

  DECLARE_DYNAMIC_CLASS(wxGameInfoPanel);
};
