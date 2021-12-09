CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = server
OBJS = ../src/Log/*.cpp ../src/SqlConnPool/*.cpp ../src/Timer/*.cpp \
       ../src/Http/*.cpp ../src/WebServer/*.cpp \
       ../src/Buffer/*.cpp ../src/main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -L /usr/lib64/mysql -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)




