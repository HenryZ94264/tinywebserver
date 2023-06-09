CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

server: main.cpp  ./timer/lst_timer.cpp ./timer/wheel_timer.h ./timer/heap_timer.h ./timer/utils.cpp ./http/http_conn.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp  webserver.cpp config.cpp 
	$(CXX) -std=c++11 -o server  $^ $(CXXFLAGS) -lpthread -lmysqlclient

clean:
	rm  -r server
