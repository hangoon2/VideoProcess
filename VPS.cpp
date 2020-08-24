#include "VPS.h"

// static VPS* gs_pVps = NULL;

// void Callback_AddLog(int nHpNo, const char* log) {
//     if(gs_pVps != NULL) {
//         gs_pVps->AddLog(nHpNo, log);
//     }
// }

#if ENABLE_UI
const SDL_Color DEFAULT_TEXT_COLOR{0, 0, 0};
#endif

VPS::VPS() {
#if ENABLE_UI
    m_window = NULL;
    m_renderer = NULL;
    m_texture = NULL;

    m_font = NULL;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    CreateWindow();
#endif
}

VPS::~VPS() {
#if ENABLE_UI
    if(m_font != NULL) {
        TTF_CloseFont(m_font);
    }

    if(m_texture != NULL) {
        SDL_DestroyTexture(m_texture);
    }

    if(m_renderer != NULL) {
        SDL_DestroyRenderer(m_renderer);
    }
    
    if(m_window != NULL) {
        SDL_DestroyWindow(m_window);
    }

    SDL_Quit();
    TTF_Quit();
#endif
}

void VPS::ShowWindow() {
#if ENABLE_UI
    SDL_Event event;

    SDL_StartTextInput();

    while( SDL_WaitEvent(&event) ) {
//        SDL_WaitEvent(&event);
//        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        SDL_SetRenderDrawColor(m_renderer, 0xFF, 0xFF, 0xFF, 0x00);
        SDL_RenderClear(m_renderer);
        RenderText();
//        SDL_RenderCopy(m_renderer, m_texture, NULL, NULL);
        SDL_RenderPresent(m_renderer);
    }

    SDL_StopTextInput();
#endif
}

void VPS::CreateWindow() {
#if ENABLE_UI
    m_window = SDL_CreateWindow("VPS", 
                                100, 
                                100, 
                                600, 400, 
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // m_surface = SDL_GetWindowSurface(m_window);
    // SDL_FillRect( m_surface, NULL, SDL_MapRGB(m_surface->format, 0xFF, 0xFF, 0xFF) );

    m_texture = SDL_CreateTextureFromSurface(m_renderer, NULL);

    m_font = TTF_OpenFont("FreeSans.ttf", 12);
#endif
}

void VPS::RenderText() {
#if ENABLE_UI
    if(m_font != NULL) {
        int size = TTF_FontAscent(m_font);

        SDL_Surface* surfaceMessage = TTF_RenderText_Solid(m_font, "Test Text", DEFAULT_TEXT_COLOR);
        SDL_Texture* message = SDL_CreateTextureFromSurface(m_renderer, surfaceMessage);

        int w, h;
        TTF_SizeText(m_font, "Test Text", &w, &h);

        //Set rendering space and render to screen
//        SDL_Rect renderQuad = { 100, 100, w, h };
        SDL_Rect rectMessage{100, 100, w, h};

        //Render to screen
//        SDL_RenderCopyEx(m_renderer, message, NULL, &renderQuad, 0.0, NULL, SDL_FLIP_NONE);
        SDL_RenderCopy(m_renderer, message, NULL, &rectMessage);

        SDL_DestroyTexture(message);
        SDL_FreeSurface(surfaceMessage);

        int outline = TTF_GetFontOutline(m_font);

        printf("Render Text : %d, %d\n", size, outline);
    }
#endif
}

// VPS::VPS()
// : m_vBox(Gtk::ORIENTATION_VERTICAL) {
//     set_border_width(5);
//     set_default_size(400, 300);
//     add(m_vBox);

//     CreateDeviceView();
//     CreateLogView();

//     show_all_children();

//     AddLog(0, "UI 초기화 완료");

//     gs_pVps = this;
// }

// VPS::~VPS() {
//     Destroy();
// }

// void VPS::Destroy() {
//     // if(m_pNetMgr != NULL) {
//     //     delete m_pNetMgr;
//     //     m_pNetMgr = NULL;
//     // }
// }

// void VPS::CreateDeviceView() {
//     m_vBox.pack_start(m_deviceListView);
// }

// void VPS::CreateLogView() {
//     m_logView.set_cursor_visible(false);
//     m_logView.set_editable(false);

//     m_scrollWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
//     m_scrollWindow.add(m_logView);

//     m_vBox.pack_start(m_scrollWindow);
// //    add(m_scrollWindow);

//     //add(m_logView);
//     //m_logView.show();
// }

// void VPS::AddLog(int nHpNo, const char* log) {
// //    Glib::RefPtr<Gtk::TextBuffer> buffer = m_logView.get_buffer();

//     char logText[512] ={0,};

//     sprintf(logText, "[VPS:%d] %s\n", nHpNo, log);

// //    buffer->insert( buffer->end(), logText );
    
//     // Glib::RefPtr<Gtk::Adjustment> adj = m_scrollWindow.get_vadjustment();
//     // adj->set_value( adj->get_upper() );

//     printf("%s", logText);

//     // buffer->insert(buffer->end(), log);
//     // buffer->insert(buffer->end(), "\n");
    
//     //m_logView.scroll_to(buffer->end());
// //    buffer->set_text(log);
// }