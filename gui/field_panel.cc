#include "field_panel.h"

#include "gui_utils.h"
#include "style.h"

IMPLEMENT_DYNAMIC_CLASS(wxFieldPanel, wxPanel);

BEGIN_EVENT_TABLE(wxFieldPanel, wxPanel)
EVT_PAINT(wxFieldPanel::paintEvent)
END_EVENT_TABLE()

wxFieldPanel::wxFieldPanel(wxWindow *parent, wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE), sizer(new wxBoxSizer(wxVERTICAL))
{
}

void wxFieldPanel::paintEvent(wxPaintEvent &evt)
{
  wxPaintDC dc(this);
  render(dc);
}

void wxFieldPanel::render(wxDC &dc)
{
  if (!geo.has_field()) {
    puts("no geo!");

    SSL_GeometryFieldSize &dims = *geo.mutable_field();
    dims.set_field_length(9000);
    dims.set_field_width(6000);
    dims.set_goal_width(1000);
    dims.set_goal_depth(200);
    dims.set_defense_radius(1000);
    dims.set_defense_stretch(500);
  }

  const SSL_GeometryFieldSize &dims = geo.field();
  wxSize sz = GetSize();

  double fwh = dims.field_width() / 2, flh = dims.field_length() / 2;
  double gd = dims.goal_depth(), gwh = dims.goal_width() / 2;
  double dr = dims.defense_radius(), dsh = dims.defense_stretch() / 2;

  double margin = 500;
  double min_x = -flh - margin;
  double max_x = flh + margin;
  double min_y = -fwh - margin;
  double max_y = fwh + margin;

  dc.SetDeviceOrigin(sz.x / 2, sz.y / 2);
  double scale = std::min(sz.y / (max_y - min_y), sz.x / (max_x - min_x));
  dc.SetUserScale(scale, -scale);

  // draw background green rectangle
  dc.SetPen(wxNullPen);
  dc.SetBrush(wxBrush(wxColour(0, 128, 0)));
  dc.DrawRectangle(min_x, min_y, max_x - min_x, max_y - min_y);

  // fill goals
  int blue_side = vars.blue_side;
  if (blue_side != 0) {
    dc.SetPen(wxNullPen);

    dc.SetBrush(wxBrush(blue_side > 0 ? blue_team_colour : yellow_team_colour));
    dc.DrawRectangle(flh, -gwh, gd, gwh * 2);

    dc.SetBrush(wxBrush(blue_side > 0 ? yellow_team_colour : blue_team_colour));
    dc.DrawRectangle(-flh, gwh, -gd, -gwh * 2);
  }

  // highlight last touch robot
  for (const auto &r : w.robots) {
    if (vars.toucher.isValid() && r.robot_id == vars.toucher.id && r.team == vars.toucher.team) {
      dc.SetPen(wxNullPen);
      dc.SetBrush(wxBrush(wxColour(224, 128, 0)));
      dc.DrawCircle(wxPoint(r.loc.x, r.loc.y), 200);
      break;
    }
  }

  // draw basic field lines
  dc.SetBrush(wxBrush(wxColour(0, 0, 0), wxTRANSPARENT));
  dc.SetPen(wxPen(wxColour(255, 255, 255)));
  dc.DrawCircle(wxPoint(0, 0), 500);
  dc.DrawLine(0, -fwh, 0, fwh);
  dc.DrawRectangle(-flh, -fwh, flh * 2, fwh * 2);

  // draw defense areas
  dc.DrawLine(-flh + dr, dsh, -flh + dr, -dsh);
  dc.DrawArc(-flh + dr, dsh, -flh, dsh + dr, -flh, dsh);
  dc.DrawArc(-flh, -dsh - dr, -flh + dr, -dsh, -flh, -dsh);

  dc.DrawLine(flh - dr, dsh, flh - dr, -dsh);
  dc.DrawArc(flh, dsh + dr, flh - dr, dsh, flh, dsh);
  dc.DrawArc(flh - dr, -dsh, flh, -dsh - dr, flh, -dsh);

  // draw goals
  dc.SetBrush(wxBrush(wxColour(0, 0, 0), wxTRANSPARENT));
  dc.SetPen(wxPen(wxColour(255, 255, 255)));
  dc.DrawLine(-flh, gwh, -flh - gd, gwh);
  dc.DrawLine(-flh - gd, gwh, -flh - gd, -gwh);
  dc.DrawLine(-flh - gd, -gwh, -flh, -gwh);
  dc.DrawLine(flh, gwh, flh + gd, gwh);
  dc.DrawLine(flh + gd, gwh, flh + gd, -gwh);
  dc.DrawLine(flh + gd, -gwh, flh, -gwh);

  // draw ball
  dc.SetBrush(wxBrush(wxColour(255, 128, 0)));
  dc.DrawCircle(wxPoint(w.ball.loc.x, w.ball.loc.y), 30);

  // draw robots
  dc.SetPen(wxNullPen);
  for (const auto &r : w.robots) {
    if (r.team == TeamBlue) {
      dc.SetBrush(wxBrush(wxColour(0, 0, 255)));
    }
    else {
      dc.SetBrush(wxBrush(wxColour(255, 255, 0)));
    }

    dc.DrawCircle(wxPoint(r.loc.x, r.loc.y), 90);
  }

  Update();
}

void wxFieldPanel::addUpdate(wxCommandEvent &event)
{
  World *world_p = reinterpret_cast<World *>(event.GetClientData());
  w = *world_p;
  // delete world_p;
}

void wxFieldPanel::addGeometryUpdate(wxCommandEvent &event)
{
  SSL_GeometryData *geo_p = reinterpret_cast<SSL_GeometryData *>(event.GetClientData());
  geo.CopyFrom(*geo_p);
  // delete geo_p;
}

void wxFieldPanel::addAutorefUpdate(wxCommandEvent &event)
{
  AutorefVariables *vars_p = reinterpret_cast<AutorefVariables *>(event.GetClientData());
  vars = *vars_p;
  // delete vars_p;
}
