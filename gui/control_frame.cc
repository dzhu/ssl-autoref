#include "control_frame.h"
#include "event_types.h"
#include "gui_utils.h"

BEGIN_EVENT_TABLE(AutorefControlWxFrame, wxFrame)
EVT_COMMAND(wxID_ANY, wxEVT_CONSTRUCT_AUTOREF_UPDATE, AutorefControlWxFrame::setAutoref)
END_EVENT_TABLE()

AutorefControlWxFrame::AutorefControlWxFrame(const wxString &title) : wxFrame(nullptr, wxID_ANY, title)
{
  wxIcon icon(_T("icon.gif"), wxBITMAP_TYPE_GIF);
  SetIcon(icon);

  SetSizer(new wxBoxSizer(wxVERTICAL));
}

void AutorefControlWxFrame::setAutoref(wxCommandEvent &evt)
{
  autoref = (BaseAutoref *)evt.GetClientData();

  events.clear();
  autoref->forEachEvent([this](AutorefEvent *e) { events.push_back(e); });

  std::sort(events.begin(), events.end(), [](AutorefEvent *e1, AutorefEvent *e2) {
    return std::string(e1->name()) < std::string(e2->name());
  });

  for (AutorefEvent *ev : events) {
    wxCheckBox *c = new wxCheckBox(this, wxID_ANY, ev->name());
    c->SetValue(true);
    Bind(wxEVT_CHECKBOX, [c, ev](wxCommandEvent &evt) { ev->setEnabled(c->IsChecked()); }, c->GetId());
    GetSizer()->Add(c);
  }

  Layout();
}
