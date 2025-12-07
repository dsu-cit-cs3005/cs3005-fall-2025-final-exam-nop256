#include "RobotBase.h"
#include <vector>
#include <utility>
#include <cmath>
#include <limits>

class Robot_CornerSniper : public RobotBase {
private:
    // last radar results (only enemies)
    std::vector<std::pair<int,int>> last_seen_this_turn;

    int locked_dir = 0;   // direction of a locked target line, or 0
    int sweep_idx  = 1;   // for simple radar sweeping when no lock

    bool cornerChosen = false;
    int  corner_r     = 0;
    int  corner_c     = 0;

    // Map direction vector to dir code (1..8), same convention as Arena/Reaper.
    int dirFromVec(int sgnr, int sgnc) const {
        if      (sgnr == -1 && sgnc ==  0) return 1;
        else if (sgnr == -1 && sgnc ==  1) return 2;
        else if (sgnr ==  0 && sgnc ==  1) return 3;
        else if (sgnr ==  1 && sgnc ==  1) return 4;
        else if (sgnr ==  1 && sgnc ==  0) return 5;
        else if (sgnr ==  1 && sgnc == -1) return 6;
        else if (sgnr ==  0 && sgnc == -1) return 7;
        else if (sgnr == -1 && sgnc == -1) return 8;
        return 0;
    }

    void chooseCornerIfNeeded() {
        if (cornerChosen) return;
        if (m_board_row_max <= 0 || m_board_col_max <= 0) return;

        int cr, cc;
        get_current_location(cr, cc);

        int rmax = m_board_row_max - 1;
        int cmax = m_board_col_max - 1;

        // Four corners
        int corners[4][2] = {
            {0,    0},
            {0,    cmax},
            {rmax, 0},
            {rmax, cmax}
        };

        int bestIdx   = 0;
        int bestDist  = std::numeric_limits<int>::max();


        bestIdx   = 0;
        bestDist  = std::numeric_limits<int>::max();
        for (int i = 0; i < 4; ++i) {
            int rr = corners[i][0];
            int cc = corners[i][1];
            int d  = std::abs(rr - cr) + std::abs(cc - cc);
            if (d < bestDist) {
                bestDist = d;
                bestIdx  = i;
            }
        }

        corner_r     = corners[bestIdx][0];
        corner_c     = corners[bestIdx][1];
        cornerChosen = true;
    }

public:
    Robot_CornerSniper()
    : RobotBase(/*move*/4, /*armor*/3, /*weapon*/railgun)
    {
        // Simple, fragile railgun camper.
    }

    ~Robot_CornerSniper() override = default;

    void get_radar_direction(int& radar_direction) override {
        int cr, cc;
        get_current_location(cr, cc);
        chooseCornerIfNeeded();

        if (locked_dir != 0) {
            // If we have a locked line on an enemy, keep staring down that line.
            radar_direction = locked_dir;
        } else {
            // Otherwise, do a simple sweep around us.
            if (sweep_idx < 1 || sweep_idx > 8) sweep_idx = 1;
            radar_direction = sweep_idx;
            sweep_idx = (sweep_idx % 8) + 1;
        }
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        last_seen_this_turn.clear();
        locked_dir = 0;

        int cr, cc;
        get_current_location(cr, cc);

        auto isEnemyBot = [](char t) {
            // Terrain / corpses:
            if (t == '.' || t == 'M' || t == 'P' || t == 'F' || t == 'X') {
                return false;
            }
            // Anything else is a bot.
            return true;
        };

        bool have_lock = false;

        for (const auto& obj : radar_results) {
            char ot = obj.m_type;
            int  rr = obj.m_row;
            int  cc2 = obj.m_col;

            if (!isEnemyBot(ot)) {
                continue;
            }

            last_seen_this_turn.push_back({rr, cc2});

            int dr = rr - cr;
            int dc = cc2 - cc;

            bool aligned = (dr == 0) || (dc == 0) || (std::abs(dr) == std::abs(dc));
            if (aligned && !have_lock) {
                int sgnr = (dr > 0) - (dr < 0);
                int sgnc = (dc > 0) - (dc < 0);
                locked_dir = dirFromVec(sgnr, sgnc);
                have_lock  = true;
            }
        }

        if (!have_lock) {
            locked_dir = 0;
        }
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (last_seen_this_turn.empty()) {
            return false;
        }

        int cr, cc;
        get_current_location(cr, cc);

        static const std::pair<int,int> dirs[9] = {
            {0,0},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1}
        };

        auto alignedWith = [&](int r, int c, int dir) -> bool {
            int dr = r - cr;
            int dc = c - cc;
            if (!((dr == 0) || (dc == 0) || (std::abs(dr) == std::abs(dc)))) return false;
            if (dir == 0) return true;
            int sgnr = (dr > 0) - (dr < 0);
            int sgnc = (dc > 0) - (dc < 0);
            return dirs[dir].first == sgnr && dirs[dir].second == sgnc;
        };

        // 1) Prefer targets aligned with locked_dir if we have it.
        std::pair<int,int> best = {-1, -1};
        int bestd = std::numeric_limits<int>::max();

        if (locked_dir != 0) {
            for (auto [r, c] : last_seen_this_turn) {
                if (!alignedWith(r, c, locked_dir)) continue;
                int d = std::max(std::abs(r - cr), std::abs(c - cc));
                if (d < bestd) {
                    bestd = d;
                    best  = {r, c};
                }
            }
        }

        // 2) Otherwise, pick any aligned target (closest).
        if (bestd == std::numeric_limits<int>::max()) {
            for (auto [r, c] : last_seen_this_turn) {
                if (!alignedWith(r, c, 0)) continue;
                int d = std::max(std::abs(r - cr), std::abs(c - cc));
                if (d < bestd) {
                    bestd = d;
                    best  = {r, c};
                }
            }
        }

        if (bestd == std::numeric_limits<int>::max() || bestd < 1) {
            return false;
        }

        shot_row = best.first;
        shot_col = best.second;
        return true;
    }

    void get_move_direction(int& direction, int& distance) override {
        int cr, cc;
        get_current_location(cr, cc);
        chooseCornerIfNeeded();

        direction = 0;
        distance  = 0;

        // If we haven't reached our corner yet, move directly toward it.
        if (cornerChosen) {
            int dr = corner_r - cr;
            int dc = corner_c - cc;
            int cheb = std::max(std::abs(dr), std::abs(dc));
            if (cheb > 0) {
                int sgnr = (dr > 0) - (dr < 0);
                int sgnc = (dc > 0) - (dc < 0);
                int d = dirFromVec(sgnr, sgnc);
                if (d != 0) {
                    direction = d;
                    distance  = std::min(get_move_speed(), cheb);
                    return;
                }
            }
        }

        // At (or very near) the corner: mostly camp.
        // Only move if something gets too close (Chebyshev <= 2).
        int bestCheb = std::numeric_limits<int>::max();
        int er = cr, ec = cc;

        for (auto [r, c] : last_seen_this_turn) {
            int ch = std::max(std::abs(r - cr), std::abs(c - cc));
            if (ch < bestCheb) {
                bestCheb = ch;
                er = r;
                ec = c;
            }
        }

        if (bestCheb <= 2) {
            // Enemy too close: step one tile away from them.
            int dr = cr - er;
            int dc = cc - ec;
            int sgnr = (dr > 0) - (dr < 0);
            int sgnc = (dc > 0) - (dc < 0);
            int d = dirFromVec(sgnr, sgnc);
            if (d != 0) {
                direction = d;
                distance  = 1;
                return;
            }
        }

        // Otherwise: camp (no movement).
        direction = 0;
        distance  = 0;
    }
};

extern "C" RobotBase* create_robot_cornersniper() {
    return new Robot_CornerSniper();
}

