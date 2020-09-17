#include "VPSLogger.h"
#include "VPSCommon.h"

#include <glog/logging.h>

#define VPS_LOGGER_LOG_FILENAME_PREFIX  "vps-"

VPSLogger::VPSLogger() {
    Init();
}

VPSLogger::~VPSLogger() {
    google::FlushLogFiles(google::GLOG_INFO);
    google::ShutdownGoogleLogging();
}

void VPSLogger::Init() {
    m_isDirty = false;

    static char szFilenamePrefix[64] = {0,};

    if(szFilenamePrefix[0] == 0) {
        sprintf(szFilenamePrefix, "%s%s-", VPS_LOGGER_LOG_FILENAME_PREFIX, VPS_VERSION);
    }

    google::SetLogFilenameExtension(szFilenamePrefix);
    google::SetStderrLogging(google::GLOG_FATAL);
    google::SetLogDestination(google::GLOG_INFO, "./Log/");
    google::InitGoogleLogging("vps");

    m_nLoggingStartDay = GetCurrentDay();
}

void VPSLogger::AddLog(const char* log) {
    m_lock.Lock();

    LOG(INFO) << log;
    m_isDirty = true;

    m_lock.Unlock();
}   

void VPSLogger::Flush() {
    m_lock.Lock();

    google::FlushLogFilesUnsafe(google::GLOG_INFO);
    m_isDirty = false;

    m_lock.Unlock();
}

void VPSLogger::Restart() {
    m_lock.Lock();

    google::FlushLogFilesUnsafe(google::GLOG_INFO);
    google::ShutdownGoogleLogging();

    Init();

    m_lock.Unlock();
}

bool VPSLogger::IsDirty() {
    return m_isDirty;
}

int VPSLogger::GetLoggingStartDay() {
    return m_nLoggingStartDay;
}