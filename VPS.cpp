#include "VPS.h"

#if ENABLE_UI

#include <thread>

using namespace std;

static VPS* gs_pVps = NULL;

void Callback_AddLog(int nHpNo, const char* log) {
    if(gs_pVps != NULL) {
        gs_pVps->AddLog(nHpNo, log);
    }
}

VPS::VPS()
: m_vBox(Gtk::ORIENTATION_VERTICAL) {
    // thread([=]() {
    //     m_pNetMgr = new NetManager(Callback_AddLog);
    //     m_pNetMgr->OnServerModeStart();
    // }).detach();

    set_border_width(5);
    set_default_size(400, 300);
    add(m_vBox);

    CreateDeviceView();
    CreateLogView();

    show_all_children();

    AddLog(0, "UI 초기화 완료");

    gs_pVps = this;
}

VPS::~VPS() {
    Destroy();
}

void VPS::Destroy() {
    // if(m_pNetMgr != NULL) {
    //     delete m_pNetMgr;
    //     m_pNetMgr = NULL;
    // }
}

void VPS::CreateDeviceView() {
    m_vBox.pack_start(m_deviceListView);
}

void VPS::CreateLogView() {
    m_logView.set_cursor_visible(false);
    m_logView.set_editable(false);

    m_scrollWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_scrollWindow.add(m_logView);

    m_vBox.pack_start(m_scrollWindow);
//    add(m_scrollWindow);

    //add(m_logView);
    //m_logView.show();
}

void VPS::AddLog(int nHpNo, const char* log) {
//    Glib::RefPtr<Gtk::TextBuffer> buffer = m_logView.get_buffer();

    char logText[512] ={0,};

    sprintf(logText, "[VPS:%d] %s\n", nHpNo, log);

//    buffer->insert( buffer->end(), logText );
    
    // Glib::RefPtr<Gtk::Adjustment> adj = m_scrollWindow.get_vadjustment();
    // adj->set_value( adj->get_upper() );

    printf("%s", logText);

    // buffer->insert(buffer->end(), log);
    // buffer->insert(buffer->end(), "\n");
    
    //m_logView.scroll_to(buffer->end());
//    buffer->set_text(log);
}

#endif