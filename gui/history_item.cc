#include "gui_utils.h"
#include "style.h"
#include "util.h"

#include "history_item.h"

BEGIN_EVENT_TABLE(wxRefereeHistoryItem, wxPanel)
EVT_TIMER(wxID_ANY, wxRefereeHistoryItem::OnTimer)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxRefereeHistoryItem, wxPanel);

using gbp = wxGBPosition;

wxRefereeHistoryItem::wxRefereeHistoryItem(wxWindow *parent, wxWindowID id, RefereeCall _update)
    : wxPanel(parent, id), sizer(new wxStaticBoxSizer(wxVERTICAL, this)), update(_update), emph(true)
{
  create_time = GetTimeMicros();

  SetSizer(sizer);
  SetWindowStyle(wxBORDER_DOUBLE);
  Refresh();

  sizer->Add(100, 0);

  auto command_sizer = new wxGridBagSizer(0, 50);

  if (update.has_command()) {
    command_label = makeText(this, "Command");
    command_text = makeText(this, ws(commandDisplayName(update.command())), commandColour(update.command()));

    command_sizer->Add(command_label, gbp(0, 0));
    command_sizer->Add(command_text, gbp(1, 0));
  }

  if (update.has_next_command()) {
    next_label = makeText(this, "Next Command");
    next_text = makeText(this, ws(commandDisplayName(update.next_command())), commandColour(update.next_command()));

    command_sizer->Add(next_label, gbp(0, 1));
    command_sizer->Add(next_text, gbp(1, 1));
  }

  sizer->Add(command_sizer, 0, wxEXPAND);

  if (update.has_description()) {
    description_label = makeText(this, "Reason");
    description_text = makeText(this, update.description());

    sizer->Add(description_label);
    sizer->Add(description_text);
  }

  if (update.has_stage_time_left()) {
    stage_time_label = makeText(this, "Stage time left");
    stage_time_text = makeText(this, formatTime(update.stage_time_left()));

    sizer->Add(stage_time_label);
    sizer->Add(stage_time_text);
  }

  setEmph(emph);

  sizer->Layout();

  animate_timer = new wxTimer(this);
  animate_timer->Start(50);
}

void wxRefereeHistoryItem::setEmph(bool e)
{
  emph = e;

  wxFont label_font(wxFontInfo(20).Family(wxFONTFAMILY_DEFAULT).Italic());
  wxFont text_font(wxFontInfo(40).Family(wxFONTFAMILY_DEFAULT));
  wxFont command_font(wxFontInfo(69).Family(wxFONTFAMILY_DEFAULT));
  wxFont next_command_font(wxFontInfo(69).Family(wxFONTFAMILY_DEFAULT));

  wxFont small_label_font(wxFontInfo(12).Family(wxFONTFAMILY_DEFAULT).Italic());
  wxFont small_text_font(wxFontInfo(20).Family(wxFONTFAMILY_DEFAULT));
  wxFont small_command_font(wxFontInfo(40).Family(wxFONTFAMILY_DEFAULT));
  wxFont small_next_command_font(wxFontInfo(40).Family(wxFONTFAMILY_DEFAULT));

  if (!emph) {
    animate_timer->Stop();
    SetBackgroundColour(wxNullColour);
  }

  if (description_label != nullptr) {
    description_label->SetFont(emph ? label_font : small_label_font);
  }
  if (command_label != nullptr) {
    command_label->SetFont(emph ? label_font : small_label_font);
  }
  if (next_label != nullptr) {
    next_label->SetFont(emph ? label_font : small_label_font);
  }
  if (stage_time_label != nullptr) {
    stage_time_label->SetFont(emph ? label_font : small_label_font);
  }

  if (description_text != nullptr) {
    description_text->SetFont(emph ? text_font : small_text_font);
  }
  if (command_text != nullptr) {
    command_text->SetFont(emph ? command_font : small_command_font);
  }
  if (next_text != nullptr) {
    next_text->SetFont(emph ? next_command_font : small_next_command_font);
  }
  if (stage_time_text != nullptr) {
    stage_time_text->SetFont(emph ? text_font : small_text_font);
  }
}

void printColour(wxColour c)
{
  printf("%d %d %d\n", c.Red(), c.Green(), c.Blue());
}

void wxRefereeHistoryItem::OnTimer(wxTimerEvent &event)
{
  wxColour start_colour(0, 255, 0);
  wxColour end_colour = wxSystemSettings::GetColour(wxSYS_COLOUR_BACKGROUND);
  uint64_t dt = GetTimeMicros() - create_time;
  double factor = std::min(1., dt / (3 * 1e6));
  SetBackgroundColour(interpolateColour(start_colour, end_colour, factor));
}

wxRefereeHistoryItem::~wxRefereeHistoryItem()
{
  delete animate_timer;
}
