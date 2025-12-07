#include "Arena.h"
#include "RobotBase.h"
#include <vector>
#include <string>

extern "C" RobotBase* create_robot();           //reaper
extern "C" RobotBase* create_robot_flame();     //flamethrower bot
extern "C" RobotBase* create_robot_rat();       //ratboy bot
extern "C" RobotBase* create_robot_hammer();    //hammer bot
extern "C" RobotBase* create_robot_grenadier(); //grenadier bot
extern "C" RobotBase* create_robot_sniper();    //sniper bot
extern "C" RobotBase* create_robot_cornersniper();    //corner sniper bot
extern "C" RobotBase* create_robot_tune();      //tune?
extern "C" RobotBase* create_robot_bob();       //bob

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
            { create_robot,          "Reaper",   'S', 1  },
            { create_robot_flame,    "Flame",    'R', 7 },
            { create_robot_rat,      "Rat",      'R', 4 },
            { create_robot_hammer,   "Hammer",   'R', 3 },
            { create_robot_grenadier,"Grenadier",'R', 3 },
            { create_robot_sniper,   "Sniper",   'R', 5 },
            { create_robot_cornersniper,"Corner",'C', 1 },
            { create_robot_tune,     "Tune",     'T', 1 },
            { create_robot_bob,      "Bob",      'B', 1 },
        };

        for (const auto& spec : specs) {
            for (int i = 0; i < spec.count; ++i) {
                RobotBase* bot = spec.factory();
                std::string name = spec.baseName + "_" + std::to_string(i+1);
                arena.addRobotRandom(bot, name, spec.symbol);
            }
        }
            arena.run(20);
        }
    return 0;
}

