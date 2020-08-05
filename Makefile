CC=g++
TARGET=vps
CPPFLAGS=--std=c++11
OPENCV_FLAGS=`pkg-config opencv4 --cflags`
OPENCV_INC=-I /usr/local/include/opencv4

LDLIBS=-L /usr/local/lib -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_videoio -lpthread -lavformat -lavcodec -lavutil -lswscale

OBJS=main.o NetManager.o AsyncMediaServerSocket.o ClientObject.o ClientList.o Mirroring.o \
		MIR_Client.o VPSJpeg.o VPSCommon.o Timer.o Rec_MemPool.o Rec_Queue.o VideoRecorder.o

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)
	rm -f *.o

main.o: main.cpp
	$(CC) -c main.cpp $(CPPFLAGS)

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

VPSJpeg.o: mirroring/VPSJpeg.cpp
	$(CC) -c mirroring/VPSJpeg.cpp $(CPPFLAGS) $(OPENCV_FLAGS) $(OPENCV_INC)

VPSCommon.o: common/VPSCommon.cpp
	$(CC) -c common/VPSCommon.cpp $(CPPFLAGS)

Timer.o: common/Timer.cpp
	$(CC) -c common/Timer.cpp $(CPPFLAGS)

Rec_MemPool.o: recorder/Rec_MemPool.cpp
	$(CC) -c recorder/Rec_MemPool.cpp $(CPPFLAGS)

Rec_Queue.o: recorder/Rec_Queue.cpp
	$(CC) -c recorder/Rec_Queue.cpp $(CPPFLAGS)

VideoRecorder.o: recorder/VideoRecorder.cpp
	$(CC) -c recorder/VideoRecorder.cpp $(CPPFLAGS) $(OPENCV_INC)

clean:
	rm -f $(TARGET)