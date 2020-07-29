CC=g++
TARGET=vps
CPPFLAGS=--std=c++11
OPENCV_FLAGS=`pkg-config opencv4 --cflags`
OPENCV_INC=-I /usr/local/include/opencv4
LDLIBS=-lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lpthread
OBJS=main.o NetManager.o AsyncMediaServerSocket.o ClientObject.o ClientList.o Mirroring.o MIR_Client.o VPSJpeg.o VPSCommon.o Timer.o

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)
	rm -f *.o

main.o: main.cpp
	$(CC) -c main.cpp $(CPPFLAGS)

NetManager.o: network/NetManager.cpp
	$(CC) -c network/NetManager.cpp $(CPPFLAGS)

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

clean:
	rm -f $(TARGET)