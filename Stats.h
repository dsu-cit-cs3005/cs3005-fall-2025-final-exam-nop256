#pragma once
#include <string>
#include <cmath>
#include <vector>

struct BotStats{
        //data members
        std::string name;
        int wins;
        int losses;
        int wl_ratio;
};

class Stats{
    public:
        std::vector<BotStats> botStats_;
};
