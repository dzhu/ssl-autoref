#include <wx/wx.h>

#include "messages_robocup_ssl_geometry.pb.h"
#include "referee_call.pb.h"

#include "world.h"

#include "../events.h"

class wxFieldPanel : public wxPanel
{
  wxSizer *sizer;
  World w;

  SSL_GeometryData geo;
  AutorefVariables vars;

public:
  wxFieldPanel() : wxPanel()
  {
  }

  wxFieldPanel(wxWindow *parent, wxWindowID id);
  void addUpdate(wxCommandEvent &event);
  void addGeometryUpdate(wxCommandEvent &event);
  void addRefereeCall(wxCommandEvent &event);
  void addAutorefUpdate(wxCommandEvent &event);

  void paintEvent(wxPaintEvent &evt);
  void paintNow();
  void render(wxDC &dc);

  DECLARE_DYNAMIC_CLASS(wxFieldPanel);

  DECLARE_EVENT_TABLE();
};
