#ifndef VPS_COMMON_H
#define VPS_COMMON_H

#define INVALID_SOCKET 0L

typedef int Socket;
typedef int ServerSocket;

#define CMD_START_CODE  0x7F
#define CMD_END_CODE    0xEF
#define CMD_HEAD_SIZE   8
#define CMD_TAIL_SIZE   3

#endif