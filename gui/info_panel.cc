#include "referee.pb.h"

#include "gui_utils.h"
#include "style.h"
#include "util.h"

#include "info_panel.h"

IMPLEMENT_DYNAMIC_CLASS(wxGameInfoPanel, wxPanel);

using gbp = wxGBPosition;

wxGameInfoPanel::wxGameInfoPanel(wxWindow *parent, wxWindowID id)
    : wxPanel(parent, id), sizer(new wxBoxSizer(wxVERTICAL))
{
  SetSizer(sizer);

  wxFont label_font(wxFontInfo(25).Family(wxFONTFAMILY_DEFAULT).Italic());
  wxFont text_font(wxFontInfo(25).Family(wxFONTFAMILY_DEFAULT));
  wxFont name_font(wxFontInfo(40).Family(wxFONTFAMILY_DEFAULT));
  wxFont score_font(wxFontInfo(80).Family(wxFONTFAMILY_DEFAULT));

  {
    wxPanel *score_panel = new wxPanel(this, wxID_ANY);
    auto score_sizer = new wxGridBagSizer(0, 0);
    score_panel->SetSizer(score_sizer);

    blue_name_text = makeText(this, "Blue", name_font, blue_team_colour);
    blue_score_text = makeText(this, "0", score_font);
    yellow_name_text = makeText(this, "Yellow", name_font, yellow_team_colour);
    yellow_score_text = makeText(this, "0", score_font);

    score_sizer->Add(blue_name_text, gbp(0, 1), wxDefaultSpan, wxALIGN_CENTER_HORIZONTAL);
    score_sizer->Add(yellow_name_text, gbp(0, 3), wxDefaultSpan, wxALIGN_CENTER_HORIZONTAL);
    score_sizer->Add(blue_score_text, gbp(1, 1), wxDefaultSpan, wxALIGN_CENTER_HORIZONTAL);
    score_sizer->Add(yellow_score_text, gbp(1, 3), wxDefaultSpan, wxALIGN_CENTER_HORIZONTAL);

    score_sizer->Add(0, 0, gbp(0, 0));
    score_sizer->Add(0, 0, gbp(0, 2));
    score_sizer->Add(0, 0, gbp(0, 4));

    score_sizer->AddGrowableCol(0, 2);
    score_sizer->AddGrowableCol(1, 3);
    score_sizer->AddGrowableCol(2, 1);
    score_sizer->AddGrowableCol(3, 3);
    score_sizer->AddGrowableCol(4, 2);

    sizer->Add(score_sizer, 0, wxEXPAND);
  }

  {
    wxPanel *ref_panel = new wxPanel(this, wxID_ANY);
    auto ref_sizer = new wxGridBagSizer(0, 0);
    ref_panel->SetSizer(ref_sizer);

    wxStaticText *stage_label = makeText(this, "Stage:", label_font, wxColour(0, 0, 0));
    wxStaticText *time_label = makeText(this, "Stage time left:", label_font, wxColour(0, 0, 0));
    wxStaticText *command_label = makeText(this, "Command:", label_font, wxColour(0, 0, 0));

    stage_text = makeText(this, "", text_font);
    time_text = makeText(this, "", text_font);
    command_text = makeText(this, "", text_font);

    ref_sizer->Add(command_label, gbp(0, 0), wxDefaultSpan);
    ref_sizer->Add(stage_label, gbp(1, 0), wxDefaultSpan);
    ref_sizer->Add(time_label, gbp(2, 0), wxDefaultSpan);

    ref_sizer->Add(command_text, gbp(0, 2), wxDefaultSpan);
    ref_sizer->Add(stage_text, gbp(1, 2), wxDefaultSpan);
    ref_sizer->Add(time_text, gbp(2, 2), wxDefaultSpan);

    ref_sizer->Add(50, 0, gbp(0, 1));

    sizer->Add(ref_sizer, 0, wxALIGN_LEFT);
  }
}

void wxGameInfoPanel::addUpdate(wxCommandEvent &event)
{
  SSL_Referee *update_p = reinterpret_cast<SSL_Referee *>(event.GetClientData());
  SSL_Referee update = *update_p;
  delete update_p;

  std::string blue_name = update.blue().name();
  std::string yellow_name = update.yellow().name();

  if (!blue_name.size()) {
    blue_name = "Blue";
  }
  if (!yellow_name.size()) {
    yellow_name = "Yellow";
  }

  blue_name_text->SetLabel(ws(blue_name));
  yellow_name_text->SetLabel(ws(yellow_name));

  blue_score_text->SetLabel(wxString::Format(_T("%d"), update.blue().score()));
  yellow_score_text->SetLabel(wxString::Format(_T("%d"), update.yellow().score()));

  stage_text->SetLabel(ws(stageDisplayName(update.stage())));
  command_text->SetLabel(ws(commandDisplayName(update.command())));
  time_text->SetLabel(formatTime(update.stage_time_left()));

  sizer->Layout();
}
