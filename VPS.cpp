#include "VPS.h"

#include <pthread.h>
#include <string.h>
#include <sys/shm.h>

#if ENABLE_JAVA_UI
static VPS* gs_pVps = NULL;

void* VPSStartThreadFuc(void* pArg) {
    NetManager* pNetMgr = (NetManager*)pArg;
    if(pNetMgr != NULL) {
        pNetMgr->OnServerModeStart();
    }

    return NULL;
}

void NativeCallback(CALLBACK* cb) {
    if(gs_pVps != NULL) {
        gs_pVps->PushCallback(cb);
    }
}
#endif

#if ENABLE_NATIVE_UI
const SDL_Color DEFAULT_TEXT_COLOR{0, 0, 0};
#endif

VPS::VPS() {
#if ENABLE_NATIVE_UI
    m_window = NULL;
    m_renderer = NULL;
    m_texture = NULL;

    m_font = NULL;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    CreateWindow();
#endif

#if ENABLE_JAVA_UI

#if ENABLE_SHARED_MEMORY
    m_shmid = -1;
    m_shared_memory = (void *)0;

    CreateSharedMemory();
#endif

    gs_pVps = this;
#endif
}

VPS::~VPS() {
#if ENABLE_NATIVE_UI
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

#if ENABLE_SHARED_MEMORY
    DestroySharedMemory();
#endif

#if ENABLE_JAVA_UI
    m_cbQueue.ClearQueue();
#endif
}

#if ENABLE_JAVA_UI
void VPS::Start() {
    m_pNetMgr = new NetManager(NativeCallback);

    pthread_t tID;
    pthread_create(&tID, NULL, &VPSStartThreadFuc, m_pNetMgr);
}

void VPS::Stop() {
    if(m_pNetMgr != NULL) {
        m_pNetMgr->OnDestroy();

        delete m_pNetMgr;
        m_pNetMgr = NULL;
    }
}

void VPS::PushCallback(CALLBACK* cb) {
    m_cbQueue.EnQueue(cb);
}

bool VPS::GetLastCallback(CALLBACK* cb) {
    bool ret = false;

    if( m_cbQueue.DeQueue(cb) ) {
        ret = true;
    }

    return ret;
}

int VPS::GetLastScene(int nHpNo, BYTE* pDstJpg) {
    if(m_pNetMgr != NULL) {
        return m_pNetMgr->GetLastScene(nHpNo, pDstJpg);
    }

    return 0;
}

bool VPS::IsOnService(int nHpNo) {
    if(m_pNetMgr != NULL) {
        return m_pNetMgr->IsOnService(nHpNo);
    }

    return false;
}
#endif

#if ENABLE_SHARED_MEMORY
void VPS::CreateSharedMemory() {
    HDCAP* pCap= NULL;
    
	// 공유메모리 공간을 만든다.
	m_shmid = shmget((key_t)SAHRED_MEMORY_KEY, sizeof(HDCAP) * MEM_SHARED_MAX_COUNT, 0666 | IPC_CREAT);
	if(m_shmid == -1) {
		perror("shmget failed : ");
		return;
	}

	// 공유메모리를 사용하기 위해 프로세스메모리에 붙인다. 
	m_shared_memory = shmat(m_shmid, (void*)0, 0);
	if (m_shared_memory == (void*)-1) {
		perror("shmat failed : ");
		return;
	}

	pCap = (HDCAP*)m_shared_memory;

    for(int i = 0 ;i < MEM_SHARED_MAX_COUNT; i++){
        memset( &pCap[i], 0x00, sizeof(HDCAP) );
    }
}

void VPS::DestroySharedMemory() {
    if(m_shared_memory != NULL) {
        shmdt(m_shared_memory);
    }

    if(m_shmid != -1) {
        shmctl(m_shmid, IPC_RMID, 0);
    }
}
#endif

#if ENABLE_NATIVE_UI

void VPS::ShowWindow() {
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
}

void VPS::CreateWindow() {
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
}

void VPS::RenderText() {
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
}

#endif

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