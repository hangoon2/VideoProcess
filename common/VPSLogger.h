#ifndef VPS_LOGGER_H
#define VPS_LOGGER_H

#include "Mutex.h"

class VPSLogger {
public:
    VPSLogger();
    virtual ~VPSLogger();

    void AddLog(const char* log);
    void Flush();
    void Restart();
    bool IsDirty();
    int GetLoggingStartDay();

private:
    void Init();

private:
    Mutex m_lock;

    bool m_isDirty;
    int m_nLoggingStartDay;
};

#endif