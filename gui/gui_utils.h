#pragma once

#include <string>

#include <wx/wx.h>

#include "referee.pb.h"

wxStaticText *makeText(wxWindow *parent, const wxString &text, const wxFont &font, const wxColour &colour);
wxStaticText *makeText(wxWindow *parent, const wxString &text);
wxStaticText *makeText(wxWindow *parent, const wxString &text, const wxColour &colour);
wxStaticText *makeText(wxWindow *parent, const wxString &text, const wxFont &font);

wxColour commandColour(SSL_Referee::Command command);

wxString ws(const std::string &s);
wxString formatTime(int64_t microseconds);
wxColour interpolateColour(wxColour c1, wxColour c2, double t);
