#include "RobotBase.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <limits>

class Robot_Grenadier : public RobotBase {
private:
    bool  m_hasTarget = false;
    int   m_targetRow = -1;
    int   m_targetCol = -1;
    int   m_radarDir  = 1;   // 1–8, cycles when no target

    // Manhattan distance
    int manhattan(int r1, int c1, int r2, int c2) const {
        return std::abs(r1 - r2) + std::abs(c1 - c2);
    }

    int to_dir(int dr, int dc) const {
        // Normalize to sign
        int sgnr = (dr > 0) - (dr < 0);
        int sgnc = (dc > 0) - (dc < 0);

        if (sgnr == -1 && sgnc ==  0) return 1; // up
        if (sgnr == -1 && sgnc ==  1) return 2; // up-right
        if (sgnr ==  0 && sgnc ==  1) return 3; // right
        if (sgnr ==  1 && sgnc ==  1) return 4; // down-right
        if (sgnr ==  1 && sgnc ==  0) return 5; // down
        if (sgnr ==  1 && sgnc == -1) return 6; // down-left
        if (sgnr ==  0 && sgnc == -1) return 7; // left
        if (sgnr == -1 && sgnc == -1) return 8; // up-left
        return 0;
    }

public:
    Robot_Grenadier() : RobotBase(3, 4, grenade) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        m_name = "Grenadier";
    }

    void get_radar_direction(int& radar_direction) override {
        if (m_hasTarget) {
            int cr, cc;
            get_current_location(cr, cc);
            int dr = m_targetRow - cr;
            int dc = m_targetCol - cc;
            int dir = to_dir(dr, dc);
            if (dir != 0) {
                radar_direction = dir;
                return;
            }
        }
        radar_direction = m_radarDir;
        m_radarDir = (m_radarDir % 8) + 1;
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        m_hasTarget = false;
        int cr, cc;
        get_current_location(cr, cc);

        int bestDist = std::numeric_limits<int>::max();

        for (const auto& obj : radar_results) {
            // Treat 'R' as enemy robot in the arena printout
            if (obj.m_type == 'R') {
                int d = manhattan(cr, cc, obj.m_row, obj.m_col);
                if (d < bestDist) {
                    bestDist  = d;
                    m_targetRow = obj.m_row;
                    m_targetCol = obj.m_col;
                    m_hasTarget = true;
                }
            }
        }

        if (!m_hasTarget) {
            m_targetRow = m_targetCol = -1;
        }
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (!m_hasTarget) return false;
        if (get_grenades() <= 0) return false; // be polite, don't "shoot" with no ammo

        int cr, cc;
        get_current_location(cr, cc);
        int d = manhattan(cr, cc, m_targetRow, m_targetCol);

        // Prefer mid-range throws; if too close, don't waste grenades
        if (d >= 2 && d <= 7) {
            shot_row = m_targetRow;
            shot_col = m_targetCol;
            return true;
        }

        return false;
    }

    void get_move_direction(int& move_direction, int& move_distance) override {
        int cr, cc;
        get_current_location(cr, cc);

        // default: stand still
        move_direction = 0;
        move_distance  = 0;

        if (m_hasTarget) {
            int d = manhattan(cr, cc, m_targetRow, m_targetCol);

            // If very close, try backing away to avoid hammer/flame range
            if (d <= 2) {
                int dr = cr - m_targetRow;
                int dc = cc - m_targetCol;

                int dir = to_dir(dr, dc);
                if (dir != 0) {
                    move_direction = dir;
                    move_distance  = 1;
                }
                return;
            }

            // If far away, move toward the target
            if (d > 5) {
                int dr = m_targetRow - cr;
                int dc = m_targetCol - cc;
                int dir = to_dir(dr, dc);
                if (dir != 0) {
                    move_direction = dir;
                    move_distance  = 1;
                }
                return;
            }

            // If in the sweet zone (3–5), hover / small strafes
            int choice = std::rand() % 3;
            if (choice == 0) {
                move_direction = 0; // stay
                move_distance  = 0;
            } else if (choice == 1) {
                // strafe left relative to target
                int dr = m_targetRow - cr;
                int dc = m_targetCol - cc;
                // rotate vector 90 degrees
                int sdir = to_dir(-dc, dr);
                if (sdir != 0) {
                    move_direction = sdir;
                    move_distance  = 1;
                }
            } else {
                // strafe right
                int dr = m_targetRow - cr;
                int dc = m_targetCol - cc;
                int sdir = to_dir(dc, -dr);
                if (sdir != 0) {
                    move_direction = sdir;
                    move_distance  = 1;
                }
            }
            return;
        }

        // No target: lazy random wandering
        move_direction = (std::rand() % 8) + 1;
        move_distance  = 1;
    }
};

// Factory function
extern "C" RobotBase* create_robot_grenadier() {
    return new Robot_Grenadier();
}

