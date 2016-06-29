#include "field_panel.h"

#include "gui_utils.h"

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

  double fw = dims.field_width(), fl = dims.field_length();
  double gd = dims.goal_depth(), gw = dims.goal_width();
  double dr = dims.defense_radius(), ds = dims.defense_stretch();

  double margin = 250;
  double min_x = -fl / 2 - margin;
  double max_x = fl / 2 + margin;
  double min_y = -fw / 2 - margin;
  double max_y = fw / 2 + margin;

  dc.SetDeviceOrigin(sz.x / 2, sz.y / 2);
  double scale = std::min(sz.y / (max_y - min_y), sz.x / (max_x - min_x));
  dc.SetUserScale(scale, -scale);

  // draw background green rectangle
  dc.SetPen(wxNullPen);
  dc.SetBrush(wxBrush(wxColour(0, 128, 0)));
  dc.DrawRectangle(min_x, min_y, max_x - min_x, max_y - min_y);

  // draw basic field lines
  dc.SetBrush(wxBrush(wxColour(0, 0, 0), wxTRANSPARENT));
  dc.SetPen(wxPen(wxColour(255, 255, 255)));
  dc.DrawCircle(wxPoint(0, 0), 500);
  dc.DrawLine(0, -fw / 2, 0, fw / 2);
  dc.DrawRectangle(-fl / 2, -fw / 2, fl, fw);

  // draw defense areas
  dc.DrawLine(-fl / 2 + dr, ds / 2, -fl / 2 + dr, -ds / 2);
  dc.DrawArc(-fl / 2 + dr, ds / 2, -fl / 2, ds / 2 + dr, -fl / 2, ds / 2);
  dc.DrawArc(-fl / 2, -ds / 2 - dr, -fl / 2 + dr, -ds / 2, -fl / 2, -ds / 2);

  dc.DrawLine(fl / 2 - dr, ds / 2, fl / 2 - dr, -ds / 2);
  dc.DrawArc(fl / 2, ds / 2 + dr, fl / 2 - dr, ds / 2, fl / 2, ds / 2);
  dc.DrawArc(fl / 2 - dr, -ds / 2, fl / 2, -ds / 2 - dr, fl / 2, -ds / 2);

  // draw goals
  dc.SetBrush(wxBrush(wxColour(0, 0, 0), wxTRANSPARENT));
  dc.SetPen(wxPen(wxColour(255, 255, 255)));
  dc.DrawLine(-fl / 2, gw / 2, -fl / 2 - gd, gw / 2);
  dc.DrawLine(-fl / 2 - gd, gw / 2, -fl / 2 - gd, -gw / 2);
  dc.DrawLine(-fl / 2 - gd, -gw / 2, -fl / 2, -gw / 2);
  dc.DrawLine(fl / 2, gw / 2, fl / 2 + gd, gw / 2);
  dc.DrawLine(fl / 2 + gd, gw / 2, fl / 2 + gd, -gw / 2);
  dc.DrawLine(fl / 2 + gd, -gw / 2, fl / 2, -gw / 2);

  // draw robots and ball
  dc.SetBrush(wxBrush(wxColour(255, 125, 0)));
  dc.DrawCircle(wxPoint(w.ball.loc.x, w.ball.loc.y), 30);

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
  delete world_p;
}

void wxFieldPanel::addGeometryUpdate(wxCommandEvent &event)
{
  SSL_GeometryData *geo_p = reinterpret_cast<SSL_GeometryData *>(event.GetClientData());
  geo.CopyFrom(*geo_p);
  delete geo_p;
}
