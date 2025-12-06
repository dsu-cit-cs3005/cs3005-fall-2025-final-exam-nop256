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
    char weaponGlyph{'R'};  // 'R','F','H','G', etc
    char idGlyph{'?'};      // special identifier !@#$%^&*
    int r{0}, c{0};
    bool alive{true};
    int shotsFired{0};
    int shotsHit{0};
    int kills{0};
    int damageDealt{0};
    int damageTaken{0};
    int roundsAlive = 0;
    int deathRow    = -1;
    int deathCol    = -1;
    int timesStuck  = 0;     
    bool died{false};
    std::string causeOfDeath;
};


class Arena {
public:
    Arena(int rows=20,int cols=20);
    ~Arena();
    void addRobot(RobotBase* robot, const std::string& name, char weaponGlyph, int row, int col);
    void addRobotRandom(RobotBase* robot,
                        const std::string& name,
                        char weaponGlyph);

    void run(int ms_delay_between_rounds = 100);

private:
    bool isObstacle(int row, int col) const;      
    bool hasRobot(int row, int col) const;        
    bool hasAdjacentRobot(int row, int col) const;

    bool isValidSpawn(int row, int col) const;
    Board m_board;
    std::vector<RobotEntry> m_robots;

    void doTurn(RobotEntry& re);
    std::vector<RadarObj> scanDirection(const RobotEntry& re, int dir) const;
    void applyMovement(RobotEntry& re, int dir, int dist);

    void resolveShot(const RobotEntry& shooter, int shot_r, int shot_c);
    void resolveRailgunShot(const RobotEntry& shooterEntry,int shot_r, int shot_c);
    void resolveFlameShot(const RobotEntry& shooterEntry,int shot_r, int shot_c);
    void resolveHammerAttack(const RobotEntry& shooterEntry,int shot_r, int shot_c);
    void resolveGrenade(const RobotEntry& shooterEntry,int shot_r, int shot_c);

    bool occupied(int r,int c, int* idx_out=nullptr) const;
    int aliveCount() const;
    char boardCharAt(int r, int c) const; 
    void printBoard(std::ostream& os) const;

    bool occupiedAny(int r,int c, int* idx_out=nullptr) const;
    bool occupiedAlive(int r,int c, int* idx_out=nullptr) const;

    bool doTurnAndReportAction(RobotEntry& re);


    int rounds_since_action = 0;
    static constexpr int STALEMATE_ROUNDS = 200;
    bool m_damage_or_death_this_round = false;

    std::string boardCellString(int r, int c) const;

    void writeReaperStats(long gameId);
};
