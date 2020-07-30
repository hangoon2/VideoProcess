#ifndef VPS_COMMON_H
#define VPS_COMMON_H

#include <stdint.h>

#define MAXCHCNT            10
#define MAXCLIENT_PER_CH    21  // Host(1) + Guest(20) = 21

#define INVALID_SOCKET 0L

#define CMD_START_CODE  0x7F
#define CMD_END_CODE    0xEF
#define CMD_HEAD_SIZE   8
#define CMD_TAIL_SIZE   3

#define SEND_BUF_SIZE   512

typedef int Socket;
typedef int ServerSocket;

typedef unsigned char   BYTE;
typedef unsigned short  ONYPACKET_UINT16;
typedef int             ONYPACKET_INT;
typedef long            ONYPACKET_INT32;
typedef long long       ONYPACKET_INT64;

#define MOBILE_CONTROLL_ID  "MOBILECONTROL"

#define TIMERID_JPGFPS_1SEC     1000
#define TIMERID_10SEC           10 * 1000

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
// VPS -> VD(최대 녹화 시간에 도달하면 VPS에서 클라이언트로 녹화 중지 명령 보냄)
#define CMD_STOP_RECORDING              1007
#define CMD_WAKEUP                      1008

#define CMD_ACK                         10001
#define CMD_LOGCAT                      10003

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

// 디바이스가 연결된 상태임을 알리는 메시지(VD -> VPS)
#define CMD_MONITOR_VD_HEARTBEAT        32100

#define CMD_MIRRORING_CAPTURE_FAILED    32401

/*//////////////////////////////////////////////
//               Common Function              //
//////////////////////////////////////////////*/
ONYPACKET_UINT16 CalChecksum(unsigned short* ptr, int nbytes);
uint16_t SwapEndianU2(uint16_t wValue);
uint32_t SwapEndianU4(uint32_t nValue);

BYTE* MakeSendData2(short usCmd, int nHpNo, int dataLen, BYTE* pData, BYTE* pDstData, int& totLen);

#endif