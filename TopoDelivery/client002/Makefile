CXX := g++
CFLAGS := -I $(shell pwd)/
CFLAGS += -I/usr/include/jsoncpp

LDFLAGS := -lpthread -lcurl -ljsoncpp -lcpprest -lboost_system -lboost_chrono -lboost_thread -lssl -lcrypto

PG := -pg

SRC = multicast.cpp http_interfaces.cpp routing_broadcast.cpp
OBJ = $(patsubst %.cpp, %.o, $(SRC))

TARGET := code



$(TARGET):$(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(CFLAGS) $(LDFLAGS)


%.o:%.cpp
	$(CXX) -c $< -o $@ $(CFLAGS) 

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(TARGET)

