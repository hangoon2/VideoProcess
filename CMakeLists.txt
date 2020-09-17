cmake_minimum_required(VERSION 3.5.2)
project(vps)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -fpic -Wall -g")

set(SOURCE_FILES
        src/main.cpp 
        src/VPS.cpp src/VPS.h
        src/javaui/com_onycom_vps_VPSNativeLib.cpp src/javaui/com_onycom_vps_VPSNativeLib.h
        src/javaui/Callback_MemPool.cpp src/javaui/Callback_MemPool.h
        src/javaui/Callback_Queue.cpp src/javaui/Callback_Queue.h 
        src/network/NetManager.cpp src/network/NetManager.h
        src/network/AsyncMediaServerSocket.cpp src/network/AsyncMediaServerSocket.h
        src/network/ClientObject.cpp src/network/ClientObject.h
        src/network/ClientList.cpp src/network/ClientList.h 
        src/mirroring/Mirroring.cpp src/mirroring/Mirroring.h
        src/mirroring/MIR_Client.cpp src/mirroring/MIR_Client.h
        src/mirroring/MIR_MemPool.cpp src/mirroring/MIR_MemPool.h
        src/mirroring/MIR_Queue.cpp src/mirroring/MIR_Queue.h
        src/mirroring/MIR_QueueHandler.cpp src/mirroring/MIR_QueueHandler.h
        src/mirroring/VPSJpeg.cpp src/mirroring/VPSJpeg.h
        src/common/VPSCommon.cpp src/common/VPSCommon.h
        src/common/Timer.cpp src/common/Timer.h
        src/common/Mutex.cpp src/common/Mutex.h
        src/common/VPSLogger.cpp src/common/VPSLogger.h
        src/recorder/Rec_MemPool.cpp src/recorder/Rec_MemPool.h
        src/recorder/Rec_Queue.cpp src/recorder/Rec_Queue.h
        src/recorder/VideoRecorder.cpp src/recorder/VideoRecorder.h)

if(SHARED_LIBRARY)
        message("-- Make Shared Library")
        set(EXECUTABLE VPSNativeLib)
        add_library(${EXECUTABLE}
                SHARED
                ${SOURCE_FILES})
else()
        message("-- Make Executable")
        set(EXECUTABLE vps)
        add_executable(vps ${SOURCE_FILES})
        add_compile_definitions(TYPE_EXECUTABLE)
endif()

include_directories(
        /Library/Java/JavaVirtualMachines/jdk1.8.0_211.jdk/Contents/Home/include 
        /Library/Java/JavaVirtualMachines/jdk1.8.0_211.jdk/Contents/Home/include/darwin
        /usr/local/opt/jpeg-turbo/include
        /usr/local/include/opencv4
        /usr/local/opt/gflags/include
        /usr/local/opt/glog/include
        /usr/local/opt/ffmpeg/include
)

target_link_libraries (${EXECUTABLE}
        "-L /usr/local/opt/opencv/lib/ -lopencv_core"
        "-lopencv_imgcodecs"
        "-lopencv_imgproc"
        -lpthread
        "-L /usr/local/opt/glog/lib/ -lglog"
        "-L /usr/local/opt/ffmpeg/lib/ -lavformat"
        "-lavcodec"
        "-lavutil" 
        "-lswscale"
        /usr/local/opt/jpeg-turbo/lib/libturbojpeg.a)
