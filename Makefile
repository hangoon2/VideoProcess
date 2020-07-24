CC=g++
TARGET=vps
CPPFLAGS=--std=c++11
OPENCV_FLAGS=`pkg-config opencv4 --cflags`
INC=-I $(JAVA_HOME)/include -I $(JAVA_HOME)/include/darwin -I /usr/local/include/opencv4
LDLIBS=-lopencv_core -lopencv_imgcodecs -lopencv_imgproc
OBJS=main.o NetManager.o AsyncMediaServerSocket.o ClientObject.o ClientList.o

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)
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

clean:
	rm -f $(TARGET)