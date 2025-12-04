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

int main() {
    const int rows = 28;
    const int cols = 28;
    Arena arena(rows, cols);

    std::vector<RobotSpec> specs = {
        { create_robot,       "Reaper",  'S', 1 },
        { create_robot_flame, "Flame",    'F', 15 },
        { create_robot_rat,   "Rat",      'R', 15 },
        { create_robot_hammer,   "Hammer",   'H', 15 },
    };

    for (const auto& spec : specs) {
        for (int i = 0; i < spec.count; ++i) {
            RobotBase* bot = spec.factory();
            std::string name = spec.baseName + "_" + std::to_string(i+1);
            arena.addRobotRandom(bot, name, spec.symbol);
        }
    }

    arena.run(250);
    return 0;
}

