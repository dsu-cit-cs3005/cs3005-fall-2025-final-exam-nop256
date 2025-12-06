CXX=g++
CXXFLAGS=-std=c++20 -Wall -Wextra -O2
SRC=Board.cpp Arena.cpp main.cpp Robot_Reaper.cpp Robot_Flame_e_o.cpp Robot_Ratboy.cpp RobotBase.cpp Robot_Hammer.cpp
HDR=Board.h Arena.h Tile.h DamageModel.h RobotBase.h RadarObj.h
RobotWarz: $(SRC) $(HDR);$(CXX) $(CXXFLAGS) -o $@ $(SRC)
all: test_robot
RobotBase.o: RobotBase.cpp RobotBase.h
	$(CXX) $(CXXFLAGS) -c RobotBase.cpp
test_robot: test_robot.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) test_robot.cpp RobotBase.o -ldl -o test_robot
clean:;rm -f RobotWarz *.o test_robot *.so
.PHONY:clean
