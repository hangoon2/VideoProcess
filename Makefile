CC=g++
TARGET=vps
CPPFLAGS=--std=c++11
OPENCV_FLAGS=`pkg-config opencv4 --cflags`
INC=-I /usr/local/include/opencv4
LDLIBS=-lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lpthread
OBJS=main.o NetManager.o AsyncMediaServerSocket.o ClientObject.o ClientList.o Mirroring.o MIR_Client.o MirrorCommon.o VPSJpeg.o

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

MirrorCommon.o: mirroring/MirrorCommon.cpp
	$(CC) -c mirroring/MirrorCommon.cpp $(CPPFLAGS)

VPSJpeg.o: mirroring/VPSJpeg.cpp
	$(CC) -c mirroring/VPSJpeg.cpp $(CPPFLAGS) $(OPENCV_FLAGS) $(INC)

clean:
	rm -f $(TARGET)