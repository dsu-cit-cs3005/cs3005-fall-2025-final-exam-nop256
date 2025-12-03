//Arena.h
#pragma once
#include <vector>
#include <string>
#include <utility>
#include "Board.h"
#include "RobotBase.h"
#include "RadarObj.h"

struct RobotEntry {
    RobotBase* bot{};
    std::string name;
    char glyph{'R'};
    int r{0}, c{0};
    bool alive{true};
};

class Arena {
public:
    Arena(int rows=20,int cols=20);
    ~Arena();

    void addRobot(RobotBase* robot, const std::string& name, char glyph, int row, int col);
    void addRobotRandom(RobotBase* robot,
                        const std::string& name,
                        char symbol);
    void run(int max_rounds=500);

private:
    bool isObstacle(int row, int col) const;      // true for M/P/F etc
    bool hasRobot(int row, int col) const;        // tile already occupied by robot
    bool hasAdjacentRobot(int row, int col) const;// any of 8 neighbors

    bool isValidSpawn(int row, int col) const;
    Board m_board;
    std::vector<RobotEntry> m_robots;

    // core steps
    void doTurn(RobotEntry& re);
    std::vector<RadarObj> scanDirection(const RobotEntry& re, int dir) const;
    void resolveShot(const RobotEntry& shooter, int shot_r, int shot_c);
    void applyMovement(RobotEntry& re, int dir, int dist);

    // helpers
    bool occupied(int r,int c, int* idx_out=nullptr) const;
    int aliveCount() const;
    char boardCharAt(int r, int c) const; 
    void printBoard(std::ostream& os) const;

    bool occupiedAny(int r,int c, int* idx_out=nullptr) const; // counts dead, too
    bool occupiedAlive(int r,int c, int* idx_out=nullptr) const;

    bool doTurnAndReportAction(RobotEntry& re); // return true if robot moved or shot

    // stalemate tracking
    int rounds_since_action = 0;
    static constexpr int STALEMATE_ROUNDS = 10;
};
