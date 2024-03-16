CXX = g++
TARGET = Contact_Server
SRC = $(wildcard ./Server/*.cpp \
				 ./Session/*.cpp \
				 ./ConnectionPool/*.cpp \
				 ./main.cpp)
OBJ = $(patsubst %.cpp, %.o, $(SRC))

CXXFLAGS = -std=c++20 -c -fPIC

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ -lmysqlclient -fPIC

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ 

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)