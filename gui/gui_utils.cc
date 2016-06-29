#include "gui_utils.h"
#include "style.h"
#include "util.h"

#include <iomanip>
#include <sstream>

wxStaticText *makeText(wxWindow *parent, const wxString &text, const wxFont &font, const wxColour &colour)
{
  wxStaticText *t = new wxStaticText(parent, wxID_ANY, text);
  t->SetForegroundColour(colour);
  t->SetFont(font);
  return t;
}

wxStaticText *makeText(wxWindow *parent, const wxString &text)
{
  return makeText(parent, text, *wxNORMAL_FONT, *wxBLACK);
}
wxStaticText *makeText(wxWindow *parent, const wxString &text, const wxColour &colour)
{
  return makeText(parent, text, *wxNORMAL_FONT, colour);
}
wxStaticText *makeText(wxWindow *parent, const wxString &text, const wxFont &font)
{
  return makeText(parent, text, font, *wxBLACK);
}

wxColour commandColour(SSL_Referee::Command command)
{
  switch (commandTeam(command)) {
    case TeamBlue:
      return blue_team_colour;
    case TeamYellow:
      return yellow_team_colour;
    case TeamNone:
      return no_team_colour;
  }
}

wxString ws(const std::string &s)
{
  return wxString(s.c_str(), wxConvUTF8);
}

wxString formatTime(int64_t microseconds)
{
  std::ostringstream oss;

  if (microseconds < 0) {
    oss << "-";
    microseconds = -microseconds;
  }

  int minutes = microseconds / 60 / 1000000;
  int seconds = (microseconds / 1000000) % 60;
  int millis = (microseconds / 100000) % 10;

  oss << std::setw(2) << std::setfill('0') << minutes << ":";
  oss << std::setw(2) << std::setfill('0') << seconds << ".";
  oss << std::setw(1) << std::setfill('0') << millis;

  return ws(oss.str());
}

wxColour interpolateColour(wxColour c1, wxColour c2, double t)
{
  unsigned char r = (1 - t) * c1.Red() + t * c2.Red();
  unsigned char g = (1 - t) * c1.Green() + t * c2.Green();
  unsigned char b = (1 - t) * c1.Blue() + t * c2.Blue();
  return wxColour(r, g, b);
}
