#include <wx/gbsizer.h>

#include "control_frame.h"
#include "event_types.h"
#include "gui_utils.h"

BEGIN_EVENT_TABLE(AutorefControlWxFrame, wxFrame)
EVT_COMMAND(wxID_ANY, wxEVT_CONSTRUCT_AUTOREF_UPDATE, AutorefControlWxFrame::setAutoref)
END_EVENT_TABLE()

using gbp = wxGBPosition;

AutorefControlWxFrame::AutorefControlWxFrame(const wxString &title) : wxFrame(nullptr, wxID_ANY, title)
{
  wxIcon icon(_T("icon.gif"), wxBITMAP_TYPE_GIF);
  SetIcon(icon);
}

void AutorefControlWxFrame::setAutoref(wxCommandEvent &evt)
{
  if (autoref != nullptr) {
    return;
  }

  autoref = (BaseAutoref *)evt.GetClientData();
  wxGridBagSizer *full_sizer = new wxGridBagSizer(0, 0);
  SetSizer(full_sizer);
  full_sizer->Add(20, 20, gbp(0, 0));
  full_sizer->Add(20, 20, gbp(2, 2));

  wxGridBagSizer *sizer = new wxGridBagSizer(0, 0);
  full_sizer->Add(sizer, gbp(1, 1), wxDefaultSpan, wxEXPAND);

  {
    wxSizer *rules_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Enabled rules");

    events.clear();
    autoref->forEachEvent([this](AutorefEvent *e) { events.push_back(e); });

    std::sort(events.begin(), events.end(), [](AutorefEvent *e1, AutorefEvent *e2) {
      return std::string(e1->name()) < std::string(e2->name());
    });

    for (AutorefEvent *ev : events) {
      wxCheckBox *c = new wxCheckBox(this, wxID_ANY, ev->name());
      c->SetValue(true);
      Bind(wxEVT_CHECKBOX, [c, ev](wxCommandEvent &evt) { ev->setEnabled(c->IsChecked()); }, c->GetId());
      rules_sizer->Add(c);
    }

    sizer->Add(rules_sizer, gbp(0, 0));
  }

  // {
  //   wxSizer *settings_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Settings");
  //   wxCheckBox *c = new wxCheckBox(this, wxID_ANY, "h");
  //   settings_sizer->Add(c);
  //   sizer->Add(settings_sizer, gbp(1, 0), wxDefaultSpan, wxEXPAND);
  // }

  Layout();
  Fit();
}
