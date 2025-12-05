// Logger.h
#pragma once
#include <fstream>
#include <string>

class Logger {
public:
    static std::ofstream& get() {
        static std::ofstream out("sweeper_stats.csv",
                                 std::ios::out | std::ios::app);
        return out;
    }

    template<typename T>
    static void line(const T& msg) {
        auto& out = get();
        out << msg << '\n';
        out.flush(); 
    }
};

