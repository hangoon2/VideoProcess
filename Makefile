CC = g++

TARGET = libVPSNativeLib.dylib

CPPFLAGS = --std=c++11 -fpic -Wall -O2

JNI_INC = -I $(JAVA_HOME)/include -I $(JAVA_HOME)/include/darwin
OPENCV_INC = -I /usr/local/include/opencv4
JPEG_INC = -I /usr/local/opt/jpeg-turbo/include
GLOG_INC = -I /usr/local/opt/glog/include

OPENCV_LIBS = -lopencv_core -lopencv_imgcodecs
FFMPEG_LIBS = -lavformat -lavcodec -lavutil -lswscale

LDLIBS = -lglog $(OPENCV_LIBS) $(FFMPEG_LIBS) \
		-L /usr/local/opt/jpeg-turbo/lib -lturbojpeg \
		-lpthread 

OBJS = main.o VPS.o \
		com_onycom_vps_VPSNativeLib.o Callback_MemPool.o Callback_Queue.o \
		NetManager.o AsyncMediaServerSocket.o ClientObject.o ClientList.o Mirroring.o \
		MIR_Client.o MIR_MemPool.o MIR_Queue.o MIR_QueueHandler.o VPSJpeg.o VPSCommon.o \
		Timer.o Mutex.o VPSLogger.o Rec_MemPool.o Rec_Queue.o VideoRecorder.o

SRCS = $(OBJS:.o=.cpp)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)
	rm -f *.o

main.o: main.cpp
	$(CC) -c main.cpp $(CPPFLAGS)

VPS.o: VPS.cpp
	$(CC) -c VPS.cpp $(CPPFLAGS)

com_onycom_vps_VPSNativeLib.o: javaui/com_onycom_vps_VPSNativeLib.cpp
	$(CC) -c javaui/com_onycom_vps_VPSNativeLib.cpp $(CPPFLAGS) $(JNI_INC) 

Callback_MemPool.o: javaui/Callback_MemPool.cpp
	$(CC) -c javaui/Callback_MemPool.cpp $(CPPFLAGS)

Callback_Queue.o: javaui/Callback_Queue.cpp
	$(CC) -c javaui/Callback_Queue.cpp $(CPPFLAGS)

NetManager.o: network/NetManager.cpp
	$(CC) -c network/NetManager.cpp $(CPPFLAGS) $(OPENCV_INC)

AsyncMediaServerSocket.o: network/AsyncMediaServerSocket.cpp
	$(CC) -c network/AsyncMediaServerSocket.cpp $(CPPFLAGS)

ClientObject.o: network/ClientObject.cpp
	$(CC) -c network/ClientObject.cpp $(CPPFLAGS)

ClientList.o: network/ClientList.cpp
	$(CC) -c network/ClientList.cpp $(CPPFLAGS)

Mirroring.o: mirroring/Mirroring.cpp
	$(CC) -c mirroring/Mirroring.cpp $(CPPFLAGS)

MIR_Client.o: mirroring/MIR_Client.cpp
	$(CC) -c mirroring/MIR_Client.cpp $(CPPFLAGS)

MIR_MemPool.o: mirroring/MIR_MemPool.cpp
	$(CC) -c mirroring/MIR_MemPool.cpp $(CPPFLAGS)

MIR_Queue.o: mirroring/MIR_Queue.cpp
	$(CC) -c mirroring/MIR_Queue.cpp $(CPPFLAGS)

MIR_QueueHandler.o: mirroring/MIR_QueueHandler.cpp
	$(CC) -c mirroring/MIR_QueueHandler.cpp $(CPPFLAGS)

VPSJpeg.o: mirroring/VPSJpeg.cpp
	$(CC) -c mirroring/VPSJpeg.cpp $(CPPFLAGS) $(OPENCV_INC) $(JPEG_INC)

VPSCommon.o: common/VPSCommon.cpp
	$(CC) -c common/VPSCommon.cpp $(CPPFLAGS)

Timer.o: common/Timer.cpp
	$(CC) -c common/Timer.cpp $(CPPFLAGS)

Mutex.o: common/Mutex.cpp
	$(CC) -c common/Mutex.cpp $(CPPFLAGS)

VPSLogger.o: common/VPSLogger.cpp
	$(CC) -c common/VPSLogger.cpp $(CPPFLAGS) $(GLOG_INC)

Rec_MemPool.o: recorder/Rec_MemPool.cpp
	$(CC) -c recorder/Rec_MemPool.cpp $(CPPFLAGS)

Rec_Queue.o: recorder/Rec_Queue.cpp
	$(CC) -c recorder/Rec_Queue.cpp $(CPPFLAGS)

VideoRecorder.o: recorder/VideoRecorder.cpp
	$(CC) -c recorder/VideoRecorder.cpp $(CPPFLAGS) $(OPENCV_INC)

clean:
	rm -f $(TARGET)