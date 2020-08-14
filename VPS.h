#ifndef VPS_H
#define VPS_H

#if ENABLE_UI

#include "network/NetManager.h"

#include <gtkmm.h>

class VPS : public Gtk::Window {
public:
    VPS();
    virtual ~VPS();

    void Destroy();

    void AddLog(int nHpNo, const char* log);

private:
    void CreateDeviceView();
    void CreateLogView();

private:
    NetManager* m_pNetMgr;

    Gtk::Box m_vBox;

    Gtk::ScrolledWindow m_deviceListView;

    Gtk::ScrolledWindow m_scrollWindow;
    Gtk::TextView m_logView;
};

#endif

#endif