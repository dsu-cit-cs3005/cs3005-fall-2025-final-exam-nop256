CXX=g++
CXXFLAGS=-std=c++20 -Wall -Wextra -O2
SRC=Board.cpp Arena.cpp main.cpp Robot_Reaper.cpp Robot_Flame_e_o.cpp Robot_Ratboy.cpp RobotBase.cpp Robot_Hammer.cpp
HDR=Board.h Arena.h Tile.h DamageModel.h RobotBase.h RadarObj.h
RobotWarz: $(SRC) $(HDR);$(CXX) $(CXXFLAGS) -o $@ $(SRC)
clean:;rm -f RobotWarz *.o
.PHONY:clean
