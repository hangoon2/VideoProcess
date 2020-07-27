#ifndef VPS_COMMON_H
#define VPS_COMMON_H

#define MAXCHCNT    10

#define INVALID_SOCKET 0L

typedef int Socket;
typedef int ServerSocket;

#define CMD_START_CODE  0x7F
#define CMD_END_CODE    0xEF
#define CMD_HEAD_SIZE   8
#define CMD_TAIL_SIZE   3

typedef unsigned char   ONYPACKET_UINT8;
typedef unsigned short  ONYPACKET_UINT16;
typedef int             ONYPACKET_INT;
typedef long            ONYPACKET_INT32;
typedef long long       ONYPACKET_INT64;

#define MOBILE_CONTROLL_ID  "MOBILECONTROL"

/* command code */
#define CMD_START       1
#define CMD_STOP        2

#define CMD_ID          30000
#define CMD_ID_GUEST    30007
#define CMD_ID_MONITOR  30008

#define CMD_PLAYER_QUALITY  30001
#define CMD_PLAYER_FPS      30002
#define CMD_PLAYER_EXIT     30010

#define CMD_JPG_DEV_VERT_IMG_VERT   20004
#define CMD_JPG_DEV_HORI_IMG_HORI   20005
#define CMD_JPG_DEV_VERT_IMG_HORI   20006
#define CMD_JPG_DEV_HORI_IMG_VERT   20007

#define CMD_MIRRORING_JPEG_CAPTURE_FAILED   32401

#define CMD_ON_OFF          21002

#endif