#ifndef VPS_COMMON_H
#define VPS_COMMON_H

#include <stdint.h>

#define VPS_VERSION         "1.0.0"

#define MAXCHCNT            10
#define MAXCLIENT_PER_CH    21  // Host(1) + Guest(20) = 21

#define ENABLE_JAVA_UI          1
#define ENABLE_NATIVE_UI        0
#define ENABLE_SHARED_MEMORY    0
#define ENABLE_MIRRORING_QUEUE  1
#define ENABLE_NONBLOCK_SOCKET  0

#define INVALID_SOCKET 0L
#define SOCKET_ERROR    (-1)

#define CMD_START_CODE  0x7F
#define CMD_END_CODE    0xEF
#define CMD_HEAD_SIZE   8
#define CMD_TAIL_SIZE   3

#define SEND_BUF_SIZE   512

#define MAX_SIZE_SCREEN 960

typedef int Socket;
typedef int ServerSocket;

typedef unsigned char       BYTE;
typedef unsigned short      UINT16;
typedef unsigned int        UINT;
typedef unsigned long long  ULONGLONG;
typedef int                 INT;
typedef long                INT64;
typedef unsigned long       DWORD;

#define TIMERID_JPGFPS_1SEC         1000
#define TIMERID_10SEC               10 * 1000
#define TIMERID_15SEC               15 * 1000
#define TIMERID_20SEC               20 * 1000

#define RECORDING_TIME              (30 * 60 * 1000)

#define MAX_SIZE_RGB_DATA           (MAX_SIZE_SCREEN * MAX_SIZE_SCREEN * 3)
#define MAX_SIZE_JPEG_DATA          (1024 * 1024)
#define MIR_MAX_MEM_POOL_UNIT_COUNT 30

#define VPS_DEFAULT_JPG_QUALITY     70
#define VPS_CAPTURE_JPG_QUALITY     90

#define FULLHD_IMAGE_SIZE           1382400

#define VPS_CAPTURE_RESPONSE_WAITING_TIME   3000

#define MOBILE_CONTROLL_ID          "MOBILECONTROL"

#define VPS_SZ_JPG_CAPTURE_FAIL     "JPG_CAPTURE_FAIL"
#define VPS_SZ_JPG_CREATION_FAIL    "JPG_CREATION_FAIL"
#define VPS_SZ_MP4_CREATION_FAIL    "MP4_CREATION_FAIL"

#define VPS_SZ_SECTION_STREAM       "[Stream]"
#define VPS_SZ_SECTION_CAPTURE      "[CaptureImage]"            
#define VPS_SZ_KEY_SERVER_PORT      "TCPPort"
#define VPS_SZ_KEY_MIRROR_PORT      "MirrorPort"
#define VPS_SZ_KEY_CONTROL_PORT     "ControlPort"
#define VPS_SZ_KEY_AVI_PATH         "AVISavePath"

#define VPS_SZ_MOBILE_CONTROLLER    "Device Controller"
#define VPS_SZ_CLIENT_HOST          "Host"
#define VPS_SZ_CLIENT_GUEST         "Guest"
#define VPS_SZ_CLIENT_MONITOR       "Monitor"
#define VPS_SZ_CLIENT_UNKNOWN       "Unknown"

/* ////////////////////////////////////////
//             command code              //
/////////////////////////////////////////*/
#define CMD_START       1
#define CMD_STOP        2

#define CMD_RESOURCE_USAGE_NETWORK      16
#define CMD_RESOURCE_USAGE_CPU          17
#define CMD_RESOURCE_USAGE_MEMORY       18

// test automation
#define CMD_TEST_RESULT                 309
#define CMD_TEST_START_EVENT_INDEX      318
#define CMD_TEST_START_EVENT_PATH       319
#define CMD_TEST_START_SCRIPT_RESULT    320

#define CMD_VERTICAL                    1003
#define CMD_HORIZONTAL                  1004
#define CMD_RECORD                      1005
#define CMD_JPG_CAPTURE                 1006
#define CMD_GIF                         1010
// VPS -> VD(최대 녹화 시간에 도달하면 VPS에서 클라이언트로 녹화 중지 명령 보냄)
#define CMD_STOP_RECORDING              1007
#define CMD_WAKEUP                      1008

#define CMD_ACK                         10001
#define CMD_LOGCAT                      10003

#define CMD_CAPTURE_COMPLETED           10004

#define CMD_JPG_LANDSCAPE               20001
#define CMD_JPG_PORTRAIT                20002
#define CMD_JPG_DEV_VERT_IMG_VERT       20004
#define CMD_JPG_DEV_HORI_IMG_HORI       20005
#define CMD_JPG_DEV_VERT_IMG_HORI       20006
#define CMD_JPG_DEV_HORI_IMG_VERT       20007

#define CMD_CHANGE_RESOLUTION           21001
// VPS -> Device
#define CMD_ON_OFF                      21002
#define CMD_KEY_FRAME                   21003
#define CMD_CHANGE_RESOLUTION_RATIO     21004
// VPS -> VD
#define CMD_REQUEST_MAX_RESOLUTION      21005

// Monitor 연결 해제(VPS -> DC)
#define CMD_MIRRORING_STOPPED           22002
// Device 연결끊김 통지(DC -> VPS -> VD)
#define CMD_DEVICE_DISCONNECTED         22003

#define CMD_ID                          30000
#define CMD_ID_GUEST                    30007
#define CMD_ID_MONITOR                  30008

#define CMD_PLAYER_QUALITY              30001
#define CMD_PLAYER_FPS                  30002
#define CMD_PLAYER_EXIT                 30010
#define CMD_DISCONNECT_GUEST            30011
#define CMD_UPDATE_SERVICE_TIME         30012

#define CMD_GUEST_UPDATED               31001

// vps가 살아있는 상태임을 알리는 메시지(VPS -> DC)
#define CMD_MONITOR_VPS_HEARTBEAT       32000
#define CMD_MONITOR_VIDEO_FPS_STATUS    32001
// 디바이스가 연결된 상태임을 알리는 메시지(VD -> VPS)
#define CMD_MONITOR_VD_HEARTBEAT        32100

#define CMD_DUPLICATED_CLIENT           32201

#define CMD_MIRRORING_CAPTURE_FAILED    32401

#define SAHRED_MEMORY_KEY               6769
#define MEM_SHARED_MAX_COUNT            (4 * MAXCHCNT)

typedef enum {
    TYPE_ACCESS_EMPTY = 0,
    TYPE_ACCESS_DATA,
    TYPE_ACCESS_WRITTING,
    TYPE_ACESS_READING
} ACCESS_MODE_TYPE;

typedef struct TAG_HDCAP {
    int nHpNo;
    int accessMode;     
    ULONGLONG msec;
    ULONGLONG ui64ID;
    BYTE btImg[FULLHD_IMAGE_SIZE];
} HDCAP;

typedef enum {
    CALLBACK_TYPE_UNKNOWN = 0,
    CALLBACK_TYPE_LOG,
    CALLBACK_TYPE_DEVICE_START,
    CALLBACK_TYPE_DEVICE_STOP,
    CALLBACK_TYPE_HOST_CONNECT,
    CALLBACK_TYPE_HOST_DISCONNECT,
    CALLBACK_TYPE_RECORD_START,
    CALLBACK_TYPE_RECORD_STOP,
    CALLBACK_TYPE_UPDATE_FPS,
    CALLBACK_TYPE_UPDATE_TRANSFER
} CALLBACK_TYPE;

typedef struct TAG_CALLBACK {
    int nHpNo;
    int type;
    char data[128];
} CALLBACK;

struct SYSTEM_TIME {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int millisecond;
};

typedef enum {
    LOG_TO_FILE = 0,
    LOG_TO_UI = 1,
    LOG_TO_BOTH = 2
} vps_log_target_t;

typedef bool (*PMIRRORING_ROUTINE)(void* pMirroringPacket);
typedef void (*PMIRRORING_STOP_ROUTINE)(int nHpNo, int nStopCode);
typedef void (*PVPS_ADD_LOG_ROUTINE)(int nHpNo, const char* log, vps_log_target_t nTarget);
typedef void (*PVPS_CALLBACK_ROUTINE)(CALLBACK* cb);

/*//////////////////////////////////////////////
//               Common Function              //
//////////////////////////////////////////////*/

UINT16 CalChecksum(UINT16* ptr, int nbytes);
UINT16 SwapEndianU2(UINT16 wValue);
uint32_t SwapEndianU4(UINT nValue);

BYTE* MakeSendData2(short usCmd, int nHpNo, int dataLen, BYTE* pData, BYTE* pDstData, int& totLen);

void GetLocalTime(SYSTEM_TIME &stTime);
int GetCurrentDay();
ULONGLONG GetTickCount();

bool DoesFileExist(const char* filePath);

void GetPrivateProfileString(const char* section, const char* key, const char* defaultValue, char* value);

void* Shared_GetPointer();

#endif