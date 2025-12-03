#include "RobotBase.h"
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>

class Robot_Sweeper : public RobotBase {
public:
    Robot_Sweeper(): RobotBase(/*move*/4, /*armor*/3, /*weapon*/railgun) {} 

    void get_radar_direction(int& radar_direction) override {
        int cr,cc; get_current_location(cr,cc);
        // prefer scanning toward nearest corner to accelerate long LOS acquisition
        int rmax = m_board_row_max-1, cmax = m_board_col_max-1;
        int corners[4][2]={{0,0},{0,cmax},{rmax,0},{rmax,cmax}};
        int bi=0,bd=1e9;
        for(int i=0;i<4;++i){
            int d = std::abs(corners[i][0]-cr)+std::abs(corners[i][1]-cc);
            if (d<bd){ bd=d; bi=i; }
        }
        int tr=corners[bi][0], tc=corners[bi][1];
        int sgnr = (tr>cr) - (tr<cr);
        int sgnc = (tc>cc) - (tc<cc);
        int toward_corner = dirFromVec(sgnr, sgnc);

        // if we had a good lock last turn, keep it; else bias toward corner
        radar_direction = (locked_dir!=0) ? locked_dir : (toward_corner ? toward_corner : 3);


        std::cout << "\n[SWEEPER-RADAR] " << m_name
                  << radar_direction << "\n";
                 
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        last_seen_this_turn.clear();
        int cr,cc; get_current_location(cr,cc);

        bool have_lock=false;
        for (const auto& obj : radar_results){
            if (obj.m_type!='R') continue;
            last_seen_this_turn.push_back({obj.m_row,obj.m_col});

            int dr=obj.m_row-cr, dc=obj.m_col-cc;
            bool aligned = (dr==0) || (dc==0) || (std::abs(dr)==std::abs(dc));
            if (aligned && !have_lock){
                int sgnr=(dr>0)-(dr<0), sgnc=(dc>0)-(dc<0);
                locked_dir = dirFromVec(sgnr, sgnc);
                have_lock=true;
            }
        }
        if (!have_lock) locked_dir=0;
    }

    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (last_seen_this_turn.empty()) return false;

        int cr,cc; get_current_location(cr,cc);
        static const std::pair<int,int> dirs[9]={
            {0,0},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1}
        };

        // choose target: (1) aligned and “aimed back” at us → priority
        // else (2) aligned farthest along our locked_dir; if no lock, pick any aligned farthest.
        auto alignedWith = [&](int r,int c, int dir)->bool{
            int dr=r-cr, dc=c-cc;
            if (!((dr==0)||(dc==0)||(std::abs(dr)==std::abs(dc)))) return false;
            if (dir==0) return true;
            int sgnr=(dr>0)-(dr<0), sgnc=(dc>0)-(dc<0);
            return dirs[dir].first==sgnr && dirs[dir].second==sgnc;
        };

        // (1) mirror-threat first (enemy on our line AND we are on theirs)
        std::pair<int,int> best={-1,-1}; int bestd=-1;
        for (auto [r,c]: last_seen_this_turn){
            if (!alignedWith(r,c,locked_dir)) continue;
            // “aimed back” means our vector is also a valid line from them to us (always true for row/col/diag),
            // keep the farthest to maximize pierce
            int d = std::max(std::abs(r-cr), std::abs(c-cc));
            if (d>bestd){ bestd=d; best={r,c}; }
        }
        if (bestd>=1){ shot_row=best.first; shot_col=best.second; return true; }

        // (2) any aligned farthest
        best={-1,-1}; bestd=-1;
        for (auto [r,c]: last_seen_this_turn){
            if (!alignedWith(r,c,0)) continue;
            int d = std::max(std::abs(r-cr), std::abs(c-cc));
            if (d>bestd){ bestd=d; best={r,c}; }
        }
        if (bestd>=1){ shot_row=best.first; shot_col=best.second; return true; }

        std::cout << "\n\n[SWEEPER-SHOT] " << m_name
          << " chose "
          << (bestd>=1 ? "to shoot" : "not to shoot")
          << " (target=(" << shot_row << "," << shot_col << "))\n\n";


        return false;
    }

    void get_move_direction(int& direction,int& distance) override {
        int cr,cc; get_current_location(cr,cc);

        //hammer avoidance: backstep if any seen within 2 (use last scan only)
        auto closeThreat = [&](){
            for (auto [r,c]: last_seen_this_turn){
                if (std::abs(r-cr)<=2 && std::abs(c-cc)<=2) return true;
            }
            return false;
        }();

        if (closeThreat){
            // step opposite locked_dir if we have it, else step away from scan centroid
            if (locked_dir!=0){
                // move 1 opposite of locked_dir
                int op = (locked_dir<=4) ? (locked_dir+4) : (locked_dir-4);
                direction = op; distance = 1; return;
            } else {
                // simple fallback: move up if possible else left
                direction = (cr>0)?1:7; distance = 1; return;
            }
        }

        //flamer standoff: if we saw anyone within ~5 this turn, hold or slide (don’t advance)
        bool flamerProxy=false;
        for (auto [r,c]: last_seen_this_turn)
            if (std::abs(r-cr)+std::abs(c-cc) <= 5) { flamerProxy=true; break; }
        if (flamerProxy){ direction=0; distance=0; return; }

        //keep position on a good firing line
        if (locked_dir!=0){ direction=0; distance=0; return; }

        //drift toward nearest corner otherwise
        int rmax=m_board_row_max-1, cmax=m_board_col_max-1;
        int corners[4][2]={{0,0},{0,cmax},{rmax,0},{rmax,cmax}};
        int bi=0,bd=1e9;
        for(int i=0;i<4;++i){
            int d=std::abs(corners[i][0]-cr)+std::abs(corners[i][1]-cc);
            if (d<bd){ bd=d; bi=i; }
        }
        int tr=corners[bi][0], tc=corners[bi][1];
        int sgnr=(tr>cr)-(tr<cr), sgnc=(tc>cc)-(tc<cc);
        direction = dirFromVec(sgnr, sgnc);
        if (direction == 0) {
            distance = 0;
        } else {
            int maxSteps = get_move_speed();        // 4
            distance = maxSteps;                    // ask for full speed
        }
        // DEBUGGING
        std::cout << "\n\n[SWEEPER-MOVE] " << m_name
          << " at (" << cr << "," << cc << ") "
          << "locked_dir=" << locked_dir
          << " closeThreat=" << closeThreat
          << " -> dir=" << direction
          << " dist=" << distance << "\n\n";
    }

private:
    std::vector<std::pair<int,int>> last_seen_this_turn;
    int locked_dir = 0;
    int sweep_idx = 0;

    // helper: map (sgnr,sgnc) to dir index
    int dirFromVec(int sgnr,int sgnc){
        if      (sgnr==-1 && sgnc==0) return 1;
        else if (sgnr==-1 && sgnc==1) return 2;
        else if (sgnr==0  && sgnc==1) return 3;
        else if (sgnr==1  && sgnc==1) return 4;
        else if (sgnr==1  && sgnc==0) return 5;
        else if (sgnr==1  && sgnc==-1)return 6;
        else if (sgnr==0  && sgnc==-1)return 7;
        else if (sgnr==-1 && sgnc==-1)return 8;
        return 0;
    }
};

// factory hook (optional if you want to dlopen this robot later)
extern "C" RobotBase* create_robot(){return new Robot_Sweeper();}
