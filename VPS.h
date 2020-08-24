#ifndef VPS_H
#define VPS_H

#include "network/NetManager.h"

#if ENABLE_UI
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

class VPS {
public:
    VPS();
    virtual ~VPS();

    void ShowWindow();
    void Destroy();

    void AddLog(int nHpNo, const char* log);

private:
    void CreateWindow();
    void CreateDeviceView();
    void CreateLogView();

    void RenderText();

private:
    NetManager* m_pNetMgr;

#if ENABLE_UI
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;
    SDL_Surface* m_surface;

    TTF_Font* m_font;
#endif

    // Gtk::Box m_vBox;

    // Gtk::ScrolledWindow m_deviceListView;

    // Gtk::ScrolledWindow m_scrollWindow;
    // Gtk::TextView m_logView;
};

#endif