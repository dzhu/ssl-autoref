#include <vector>

#include <wx/wx.h>

#include "../base_ref.h"
#include "../events.h"

class AutorefControlWxFrame : public wxFrame
{
  BaseAutoref *autoref;
  std::vector<AutorefEvent *> events;

public:
  AutorefControlWxFrame(const wxString &title);
  void setAutoref(wxCommandEvent &evt);

private:
  void OnQuit(wxCommandEvent &event)
  {
    Close(true);
  }

  // wxGameInfoPanel *info_panel;
  // wxAutorefHistoryPanel *history_panel;
  // wxFieldPanel *field_panel;
  // void addRefereeCall(wxCommandEvent &event)
  // {
  //   history_panel->addUpdate(event);
  // }

  // void addGameUpdate(wxCommandEvent &event)
  // {
  //   info_panel->addUpdate(event);
  // }

  // void addWorldUpdate(wxCommandEvent &event)
  // {
  //   field_panel->addUpdate(event);
  //   field_panel->Refresh();
  // }

  // void addGeometryUpdate(wxCommandEvent &event)
  // {
  //   field_panel->addGeometryUpdate(event);
  // }

  // void addAutorefUpdate(wxCommandEvent &event)
  // {
  //   field_panel->addAutorefUpdate(event);
  // }

  DECLARE_EVENT_TABLE()
};
