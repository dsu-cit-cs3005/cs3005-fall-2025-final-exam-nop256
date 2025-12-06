#include "Arena.h"
#include "RobotBase.h"
#include <vector>
#include <string>

extern "C" RobotBase* create_robot();        // Reaper
extern "C" RobotBase* create_robot_flame();  // Flamethrower bot
extern "C" RobotBase* create_robot_rat();    // Ratboy bot
extern "C" RobotBase* create_robot_hammer();    // Hammer bot

struct RobotSpec {
    RobotFactory factory;
    std::string  baseName;
    char         symbol;     
    int          count;      
};

int main(int argc, char** argv) {
    int numGames=1;
    if (argc > 1) {
        numGames = std::stoi(argv[1]);
    }
    const int rows = 20;
    const int cols = 20;

    for (int i = 0; i < numGames; ++i) {
        Arena arena(rows, cols);

        std::vector<RobotSpec> specs = {
            { create_robot,       "Reaper",  'S', 1 },
            { create_robot_flame, "Flame",    'R', 8 },
            { create_robot_rat,   "Rat",      'R', 7 },
            { create_robot_hammer,   "Hammer",   'R', 8 },
        };

        for (const auto& spec : specs) {
            for (int i = 0; i < spec.count; ++i) {
                RobotBase* bot = spec.factory();
                std::string name = spec.baseName + "_" + std::to_string(i+1);
                arena.addRobotRandom(bot, name, spec.symbol);
            }
        }
            arena.run(300);
        }
    return 0;
}

