#include "Arena.h"
#include "RobotBase.h"
#include <random>
#include <vector>
#include <string>

// factories implemented in robot cpp files
extern "C" RobotBase* create_robot();        // Sweeper
extern "C" RobotBase* create_robot_flame();  // Flamethrower bot
extern "C" RobotBase* create_robot_rat();    // Ratboy bot
extern "C" RobotBase* create_robot_hammer();    // Hammer bot

struct RobotSpec {
    RobotFactory factory;
    std::string  baseName;
    char         symbol;     // what appears on the board
    int          count;      // how many of this type
};

int main() {
    const int rows = 20;
    const int cols = 20;
    Arena arena(rows, cols);

    std::vector<RobotSpec> specs = {
        { create_robot,       "Sweeper",  'S', 1 },
        { create_robot_flame, "Flame",    'f', 5 },
        { create_robot_rat,   "Rat",      'R', 5 },
        { create_robot_rat,   "Hammer",   'H', 5 },
    };

    // Let the arena handle random placement (see section 2)
    for (const auto& spec : specs) {
        for (int i = 0; i < spec.count; ++i) {
            RobotBase* bot = spec.factory();
            std::string name = spec.baseName + "_" + std::to_string(i+1);
            arena.addRobotRandom(bot, name, spec.symbol);   // new helper
        }
    }

    arena.run(500);
    return 0;
}

