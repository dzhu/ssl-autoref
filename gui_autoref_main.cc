#include <deque>

#include <wx/gbsizer.h>
#include <wx/wx.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

#include <sys/select.h>

#include "autoref.h"
#include "base_ref.h"
#include "eval_ref.h"

#include "constants.h"
#include "messages_robocup_ssl_wrapper.pb.h"
#include "optionparser.h"
#include "rconclient.h"
#include "referee.pb.h"
#include "referee_update.pb.h"
#include "udp.h"

#include "gui/field_panel.h"
#include "gui/gui_utils.h"
#include "gui/history_item.h"
#include "gui/history_panel.h"
#include "gui/info_panel.h"

using gbp = wxGBPosition;

extern const wxEventType wxEVT_REF_UPDATE;
const wxEventType wxEVT_REF_UPDATE = wxNewEventType();

extern const wxEventType wxEVT_GAME_UPDATE;
const wxEventType wxEVT_GAME_UPDATE = wxNewEventType();

extern const wxEventType wxEVT_WORLD_UPDATE;
const wxEventType wxEVT_WORLD_UPDATE = wxNewEventType();

extern const wxEventType wxEVT_GEOMETRY_UPDATE;
const wxEventType wxEVT_GEOMETRY_UPDATE = wxNewEventType();

enum OptionIndex
{
  UNKNOWN,
  HELP,
  VERBOSE,
  EVAL,
  DRY,
};

const option::Descriptor options[] = {
  {UNKNOWN, 0, "", "", option::Arg::None, "An automatic referee for the SSL."},    //
  {HELP, 0, "h", "help", option::Arg::None, "-h, --help: print help"},             //
  {VERBOSE, 0, "v", "verbose", option::Arg::None, "-v,--verbose: verbose"},        //
  {EVAL, 0, "", "eval", option::Arg::None, "--eval: run using evaluation logic"},  //
  {DRY, 0, "q", "quiet", option::Arg::None, "-q,--quiet: don't transmit rcon"},    //
  {0, 0, nullptr, nullptr, nullptr, nullptr},
};

// TODO bad idea, but I don't feel like wrestling with wx at the moment.
int g_argc;
char **g_argv;

class AutorefWxApp : public wxApp
{
public:
  virtual bool OnInit();

private:
  wxThread *network_thread;
};

class AutorefWxFrame : public wxFrame
{
public:
  AutorefWxFrame(const wxString &title);

private:
  void OnQuit(wxCommandEvent &event);

  wxGameInfoPanel *info_panel;
  wxAutorefHistoryPanel *history_panel;
  wxFieldPanel *field_panel;
  void addRefereeUpdate(wxCommandEvent &event)
  {
    history_panel->addUpdate(event);
  }

  void addGameUpdate(wxCommandEvent &event)
  {
    info_panel->addUpdate(event);
    GetSizer()->Layout();
  }

  void addWorldUpdate(wxCommandEvent &event)
  {
    field_panel->addUpdate(event);
    GetSizer()->Layout();
    field_panel->Refresh();
  }

  void addGeometryUpdate(wxCommandEvent &event)
  {
    field_panel->addGeometryUpdate(event);
    GetSizer()->Layout();
  }

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AutorefWxFrame, wxFrame)
EVT_COMMAND(wxID_ANY, wxEVT_REF_UPDATE, AutorefWxFrame::addRefereeUpdate)
EVT_COMMAND(wxID_ANY, wxEVT_GAME_UPDATE, AutorefWxFrame::addGameUpdate)
EVT_COMMAND(wxID_ANY, wxEVT_WORLD_UPDATE, AutorefWxFrame::addWorldUpdate)
EVT_COMMAND(wxID_ANY, wxEVT_GEOMETRY_UPDATE, AutorefWxFrame::addGeometryUpdate)
END_EVENT_TABLE()

class AutorefWxThread : public wxThread
{
  int argc;
  char **argv;

public:
  AutorefWxThread(AutorefWxFrame *_handler) : wxThread(wxTHREAD_DETACHED), handler(_handler), argc(g_argc), argv(g_argv)
  {
  }
  // ~AutorefWxThread();

protected:
  virtual wxThread::ExitCode Entry();
  AutorefWxFrame *handler;
};

bool AutorefWxApp::OnInit()
{
  if (!wxApp::OnInit()) {
    return false;
  }

  AutorefWxFrame *frame = new AutorefWxFrame(_T("CMDragons autoref"));
  frame->Show(true);

  network_thread = new AutorefWxThread(frame);
  network_thread->Create();
  network_thread->Run();

  return true;
}

AutorefWxFrame::AutorefWxFrame(const wxString &title) : wxFrame(nullptr, wxID_ANY, title)
{
  wxIcon icon(_T("icon.gif"), wxBITMAP_TYPE_GIF);
  SetIcon(icon);

  auto sizer = new wxGridBagSizer(0, 0);
  SetSizer(sizer);

  sizer->Add(15, 15, gbp(0, 0));
  sizer->Add(15, 15, gbp(4, 2));
  sizer->Add(15, 15, gbp(0, 4));
  sizer->AddGrowableRow(3);
  sizer->AddGrowableCol(1, 3);
  sizer->AddGrowableCol(3, 5);

  wxFont top_label_font(45, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL);

  wxStaticText *info_label = makeText(this, "Game state", top_label_font);
  wxStaticText *history_label = makeText(this, "Autoref decisions", top_label_font);

  sizer->Add(info_label, gbp(1, 1), wxDefaultSpan, wxALIGN_LEFT);
  sizer->Add(history_label, gbp(1, 3), wxDefaultSpan, wxALIGN_LEFT);

  info_panel = new wxGameInfoPanel(this, wxID_ANY);
  sizer->Add(info_panel, gbp(2, 1), wxDefaultSpan, wxEXPAND);

  history_panel = new wxAutorefHistoryPanel(this, wxID_ANY, 4);
  sizer->Add(history_panel, gbp(2, 3), wxGBSpan(2, 1), wxEXPAND);

  field_panel = new wxFieldPanel(this, wxID_ANY);
  sizer->Add(field_panel, gbp(3, 1), wxDefaultSpan, wxEXPAND);

  history_panel->SetMinSize(wxSize(0, 0));
  info_panel->SetMinSize(wxSize(0, info_panel->GetMinHeight()));

  sizer->Layout();
}

void AutorefWxFrame::OnQuit(wxCommandEvent &event)
{
  Close(true);
}

void log(const char *s)
{
  if (0) {
    return;
  }
  char time_buf[64];
  uint64_t t0 = GetTimeMicros();
  time_t tt = t0 / 1e6;
  tm *st = localtime(&tt);
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", st);
  printf("%s.%06lu %s\n", time_buf, t0 % 1000000, s);
}

wxThread::ExitCode AutorefWxThread::Entry()
{
  argc -= (argc > 0);
  argv += (argc > 0);  // skip program name argv[0] if present

  option::Stats stats(options, argc, argv);
  std::vector<option::Option> args(stats.options_max);
  std::vector<option::Option> buffer(stats.buffer_max);
  option::Parser parse(options, argc, argv, &args[0], &buffer[0]);

  if (parse.error() || args[HELP] != nullptr || args[UNKNOWN] != nullptr) {
    option::printUsage(std::cout, options);
    return nullptr;
  }

  UDP vision_net;
  if (!vision_net.open(VisionGroup, VisionPort, true)) {
    puts("SSL-Vision port open failed!");
  }

  UDP ref_net;
  if (!ref_net.open(RefGroup, RefPort, false)) {
    puts("Referee port open failed!");
  }

  RemoteClient rcon;
  bool rcon_opened = false;
  if (args[DRY] != nullptr) {
    puts("Not opening remote client.");
  }
  else if (rcon.open("localhost", 10007)) {
    puts("Remote client opened!");
    rcon_opened = true;
  }
  else {
    rcon_opened = false;
    puts("Remote client port open failed!");
  }

  bool verbose = (args[VERBOSE] != nullptr);
  BaseAutoref *autoref;
  if (args[EVAL]) {
    puts("Starting evaluation autoref.");
    autoref = new EvaluationAutoref(verbose);
  }
  else {
    puts("Starting full autoref.");
    autoref = new Autoref(verbose);
  }

  // Address ref_addr(RefGroup, RefPort);

  SSL_WrapperPacket vision_msg;
  SSL_Referee ref_msg;

  fd_set read_fds;
  int n_fds = std::max(vision_net.getFd(), ref_net.getFd()) + 1;

  int t = 0;

  for (int i = 0; i < 40; i++) {
    break;
    auto up = new RefereeUpdate();
    up->set_description("test test test test test test test test description");
    up->set_command(SSL_Referee::STOP);
    up->set_next_command(i % 2 ? SSL_Referee::INDIRECT_FREE_BLUE : SSL_Referee::INDIRECT_FREE_YELLOW);
    up->set_stage_time_left(1717171717);
    auto evt = new wxCommandEvent(wxEVT_REF_UPDATE);
    evt->SetClientData(up);
    handler->GetEventHandler()->QueueEvent(evt);
    usleep(3000000);
  }

  while (true) {
    FD_ZERO(&read_fds);
    FD_SET(vision_net.getFd(), &read_fds);
    FD_SET(ref_net.getFd(), &read_fds);
    select(n_fds, &read_fds, nullptr, nullptr, nullptr);

    if (FD_ISSET(vision_net.getFd(), &read_fds) && vision_net.recv(vision_msg)) {
      if (vision_msg.has_detection()) {
        autoref->updateVision(vision_msg.detection());

        {
          auto w = new World();
          if (autoref->tracker.getWorld(*w)) {
            auto evt = new wxCommandEvent(wxEVT_WORLD_UPDATE);
            evt->SetClientData(w);
            handler->GetEventHandler()->QueueEvent(evt);
          }
        }

        if (autoref->isRemoteReady()) {
          if (rcon_opened) {
            rcon.sendRequest(autoref->makeRemote());
          }

          if (autoref->tracker.isReady()) {
            const RefereeUpdate &orig_update = autoref->getUpdate();
            auto update = new RefereeUpdate();
            update->CopyFrom(orig_update);
            auto evt = new wxCommandEvent(wxEVT_REF_UPDATE);
            evt->SetClientData(update);
            handler->GetEventHandler()->QueueEvent(evt);

            log(update->description().c_str());
          }
        }
      }
      if (vision_msg.has_geometry()) {
        autoref->updateGeometry(vision_msg.geometry());

        auto geo = new SSL_GeometryData();
        geo->CopyFrom(vision_msg.geometry());
        auto evt = new wxCommandEvent(wxEVT_GEOMETRY_UPDATE);
        evt->SetClientData(geo);
        handler->GetEventHandler()->QueueEvent(evt);
      }
    }
    if (FD_ISSET(ref_net.getFd(), &read_fds) && ref_net.recv(ref_msg)) {
      autoref->updateReferee(ref_msg);

      auto update = new SSL_Referee();
      update->CopyFrom(ref_msg);
      auto evt = new wxCommandEvent(wxEVT_GAME_UPDATE);
      evt->SetClientData(update);
      handler->GetEventHandler()->QueueEvent(evt);
    }
  }
}

int main(int argc, char *argv[])
{
  wxInitAllImageHandlers();

  g_argc = argc;
  g_argv = argv;
  wxApp::SetInstance(new AutorefWxApp());

  // also don't feel like wrangling wx
  int _argc = 0;
  char **_argv = {nullptr};
  wxEntryStart(_argc, _argv);

  wxTheApp->OnInit();
  wxTheApp->OnRun();

  wxTheApp->OnExit();
  wxEntryCleanup();
  return 0;
}
