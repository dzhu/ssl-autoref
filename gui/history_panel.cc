#include "history_panel.h"
#include "history_item.h"

IMPLEMENT_DYNAMIC_CLASS(wxAutorefHistoryPanel, wxPanel);

void wxAutorefHistoryPanel::addUpdate(wxCommandEvent &event)
{
  RefereeUpdate *update_p = reinterpret_cast<RefereeUpdate *>(event.GetClientData());
  RefereeUpdate update = *update_p;
  delete update_p;

  // set current first item to small
  if (sizer->GetItemCount() > 0) {
    wxSizerItem *first_item = sizer->GetItem(static_cast<size_t>(0));
    wxRefereeHistoryItem *win = (wxRefereeHistoryItem *)(first_item->GetWindow());
    win->setEmph(false);
  }

  // create and insert new item
  wxRefereeHistoryItem *item = new wxRefereeHistoryItem(this, wxID_ANY, update);
  sizer->Insert(0, item, 0, wxEXPAND);

  // remove oldest item, if too many
  if (sizer->GetItemCount() > max_items) {
    wxSizerItem *last_item = sizer->GetItem(max_items);
    sizer->Detach(last_item->GetWindow());
    delete last_item->GetWindow();
  }

  sizer->Layout();
}
