#include "RobotBase.h"
#include <vector>
#include <cmath>
#include <limits>

class Robot_Sniper : public RobotBase {
private:
    bool m_hasTarget = false;
    int  m_targetRow = -1;
    int  m_targetCol = -1;
    int  m_radarDir  = 1; // sweep 1-8

    int to_dir(int dr, int dc) const {
        int sgnr = (dr > 0) - (dr < 0);
        int sgnc = (dc > 0) - (dc < 0);

        if (sgnr == -1 && sgnc ==  0) return 1;
        if (sgnr == -1 && sgnc ==  1) return 2;
        if (sgnr ==  0 && sgnc ==  1) return 3;
        if (sgnr ==  1 && sgnc ==  1) return 4;
        if (sgnr ==  1 && sgnc ==  0) return 5;
        if (sgnr ==  1 && sgnc == -1) return 6;
        if (sgnr ==  0 && sgnc == -1) return 7;
        if (sgnr == -1 && sgnc == -1) return 8;
        return 0;
    }

public:
    Robot_Sniper() : RobotBase(3, 4, railgun) {
        m_name = "Sniper";
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
        m_targetRow = m_targetCol = -1;

        int cr, cc;
        get_current_location(cr, cc);

        int bestDist = -1;

        for (const auto& obj : radar_results) {
            if (obj.m_type != 'R') continue;

            int dr = obj.m_row - cr;
            int dc = obj.m_col - cc;

            bool aligned = (dr == 0) || (dc == 0) || (std::abs(dr) == std::abs(dc));
            if (!aligned) continue;

            int cheb = std::max(std::abs(dr), std::abs(dc));
            if (cheb > bestDist) {
                bestDist  = cheb;
                m_targetRow = obj.m_row;
                m_targetCol = obj.m_col;
                m_hasTarget = true;
            }
        }
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (!m_hasTarget) return false;

        int cr, cc;
        get_current_location(cr, cc);
        int dr = m_targetRow - cr;
        int dc = m_targetCol - cc;
        bool aligned = (dr == 0) || (dc == 0) || (std::abs(dr) == std::abs(dc));
        if (!aligned) {
            return false;
        }

        shot_row = m_targetRow;
        shot_col = m_targetCol;
        return true;
    }

    void get_move_direction(int& move_direction, int& move_distance) override {
        int cr, cc;
        get_current_location(cr, cc);

        // Default: stay
        move_direction = 0;
        move_distance  = 0;

        // If we have a nice LOS target, we often just sit and shoot
        if (m_hasTarget) {
            int dr = m_targetRow - cr;
            int dc = m_targetCol - cc;
            int d  = std::max(std::abs(dr), std::abs(dc));

            // If they're VERY close (< 3), step back a bit
            if (d < 3) {
                int backDir = to_dir(cr - m_targetRow, cc - m_targetCol);
                if (backDir != 0) {
                    move_direction = backDir;
                    move_distance  = 1;
                }
            }
            // Otherwise camp and shoot
            return;
        }

        // No target: move toward the center of the arena
        int center_r = (m_board_row_max - 1) / 2;
        int center_c = (m_board_col_max - 1) / 2;

        int dr = center_r - cr;
        int dc = center_c - cc;

        if (dr == 0 && dc == 0) {
            // Already near center; small idle wiggle
            move_direction = (std::rand() % 8) + 1;
            move_distance  = 1;
            return;
        }

        int dir = to_dir(dr, dc);
        if (dir != 0) {
            move_direction = dir;
            move_distance  = 1;
        }
    }
};

extern "C" RobotBase* create_robot_sniper() {
    return new Robot_Sniper();
}

