#ifndef VPS_H
#define VPS_H

#include "network/NetManager.h"

#if ENABLE_NATIVE_UI
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

#if ENABLE_JAVA_UI
#include "javaui/Callback_Queue.h"
#endif

class VPS {
public:
    VPS();
    virtual ~VPS();

#if ENABLE_NATIVE_UI
    void ShowWindow();
    void Destroy();

    void AddLog(int nHpNo, const char* log);

private:
    void CreateWindow();
    void CreateDeviceView();
    void CreateLogView();

    void RenderText();
#endif

#if ENABLE_JAVA_UI
    void Start();
    void Stop();
    void PushCallback(CALLBACK* cb);
    bool GetLastCallback(CALLBACK* cb);
    int GetLastScene(int nHpNo, BYTE* pDstJpg);
    bool IsOnService(int nHpNo);
#endif

private:
    void CreateSharedMemory();
    void DestroySharedMemory();

#if ENABLE_JAVA_UI
private:
    NetManager* m_pNetMgr;
#endif

#if ENABLE_SHARED_MEMORY
    int m_shmid;
    void* m_shared_memory;
#endif

#if ENABLE_JAVA_UI
    Callback_Queue m_cbQueue;
#endif

#if ENABLE_NATIVE_UI
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