//Arena.cpp
#include "Arena.h"
#include "DamageModel.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <iomanip>

extern const std::pair<int,int> directions[9];

Arena::Arena(int rows,int cols):m_board(rows,cols){ m_board.seedDefaultFeatures();}
Arena::~Arena(){ for(auto& re : m_robots) delete re.bot;}

void Arena::addRobot(RobotBase* robot,const std::string& name,char glyph, int row,int col){
    RobotEntry r{robot,name,glyph,row,col,true};
    r.bot->set_boundaries(m_board.rows(), m_board.cols());
    r.bot->move_to(row,col);
    r.bot->m_name = name;
    r.bot->m_character = glyph;
    m_robots.push_back(r);}
int Arena::aliveCount()const{
    int n=0; for(auto& r: m_robots)if(r.alive && r.bot->get_health()>0) n++;
    return n;}
bool Arena::occupied(int r,int c, int* idx_out) const{
    for(size_t i=0;i<m_robots.size();++i){
        const auto& e=m_robots[i];
        if(e.alive && e.r==r && e.c==c){ if(idx_out) *idx_out=(int)i; return true;}}return false;}
char Arena::boardCharAt(int r, int c) const {
    // If there is any robot here (dead or alive), draw the robot
    int idx = -1;
    if (occupiedAny(r, c, &idx) && idx >= 0) {
        const auto &e = m_robots[idx];
        if (e.alive && e.bot->get_health() > 0) {
            // alive robot: show its glyph
            return e.bot->m_character;
        } else {
            // dead robot acts like a corpse/obstacle
            return 'X';
        }
    }

    // Otherwise draw terrain from the board
    Tile t = m_board.get(r, c);
    switch (t) {
        case Tile::Mound: return 'M';
        case Tile::Pit:   return 'P';
        case Tile::Flame: return 'F';
        default:          return '.';
    }
}
void Arena::printBoard(std::ostream& os) const {
    // header
    os << "    "; // left margin before column numbers
    for (int c = 0; c < m_board.cols(); ++c) {
        os << std::setw(2) << c << " ";  // width 2, right aligned
    }
    os << '\n';

    for (int r = 0; r < m_board.rows(); ++r) {
        // row label
        os << std::setw(2) << r << "  ";

        // cells
        for (int c = 0; c < m_board.cols(); ++c) {
            char ch = boardCharAt(r, c); // however you currently get '.' or 'R' etc
            os << ch << ' ' << ' ';             // extra space is purely visual
        }
        os << '\n';
    }
}

bool Arena::occupiedAlive(int r,int c, int* idx_out) const {
    for (size_t i=0;i<m_robots.size();++i){
        const auto& e=m_robots[i];
        if (e.alive && e.r==r && e.c==c){
            if (idx_out) *idx_out=(int)i;
            return true;}}
    return false;}
bool Arena::occupiedAny(int r,int c,int* idx_out)const{
    for (size_t i=0;i<m_robots.size();++i){
        const auto& e=m_robots[i];
        if (e.r==r && e.c==c){ // NOTE: dead OR alive
            if (idx_out) *idx_out=(int)i;
            return true;}}
    return false;}
std::vector<RadarObj> Arena::scanDirection(const RobotEntry& re,int dir)const{
    std::vector<RadarObj> out;
    if(dir<1||dir>8)return out;
    int dr=directions[dir].first,dc=directions[dir].second;
    int r=re.r+dr, c=re.c+dc;
    while(m_board.inBounds(r,c)){
        // report terrain if present
        Tile t = m_board.get(r,c);
        if(t==Tile::Mound){out.emplace_back('M',r,c);break; }
        if(t==Tile::Pit)  {out.emplace_back('P',r,c);/* pits don't block radar */ }
        if(t==Tile::Flame){out.emplace_back('F',r,c);/* flames don't block radar */ }

        int idx=-1;
        if(occupiedAny(r,c,&idx)){ // bodies block vision
            out.emplace_back('R',r,c);
            break;}
        r+=dr; c+=dc;}
    return out;}

static constexpr bool SHOTS_BLOCKED_BY_BODIES=false;
static constexpr bool SHOTS_BLOCKED_BY_MOUNDS=false;


void Arena::resolveShot(const RobotEntry& shooter,int shot_r,int shot_c){
    int sr=shooter.r, sc=shooter.c;
    int dr=(shot_r>sr)?1:(shot_r<sr ? -1:0);
    int dc=(shot_c>sc)?1:(shot_c<sc ? -1:0);
    // must be row/col/diagonal aligned
    if(!((sr==shot_r)||(sc==shot_c)||(std::abs(shot_r-sr)==std::abs(shot_c-sc))))return;
    int r=sr+dr, c=sc+dc;
    while (m_board.inBounds(r,c)){
        if(SHOTS_BLOCKED_BY_MOUNDS && m_board.get(r,c)==Tile::Mound)break;
        int idx=-1;
        if(occupiedAny(r,c,&idx)&&idx>=0){
            auto& tgt=m_robots[idx];
            if(tgt.alive){
                int before = tgt.bot->get_health();
                DM::applyArmorThenDegrade(*tgt.bot,DM::RailgunDamage);
                int after = tgt.bot->get_health();
                std::cout << "  " << shooter.name
                          << " hits " << tgt.name
                          << " at (" << r << "," << c << ")"
                          << " for " << (before - after)
                          << " damage (hp " << before << " -> " << after << ")\n";
                 if (after == 0) {
                    tgt.alive = false;
                    std::cout << "  " << tgt.name
                              << " is DESTROYED by " << shooter.name << "!\n";
                }
            }
            if (SHOTS_BLOCKED_BY_BODIES) break;
        }
        r += dr; c += dc;
    }
}

void Arena::applyMovement(RobotEntry& re,int dir,int dist){
    if(dir<1||dir>8||dist<=0) return;
    dist =std::min(dist,re.bot->get_move_speed());
    int dr=directions[dir].first,dc=directions[dir].second;
    for(int step=0;step<dist;++step){
        int nr=re.r+dr,nc=re.c+dc;
        if(!m_board.inBounds(nr,nc))break;
        if(m_board.get(nr,nc)==Tile::Mound)break;
        int idx=-1;
        if(occupiedAny(nr,nc,&idx)){ // dead or alive: collision stop
            auto& other = m_robots[idx];
            if(other.alive){ // damage only if the other is still alive
                DM::applyArmorThenDegrade(*re.bot, DM::CollisionDamage);
                DM::applyArmorThenDegrade(*other.bot, DM::CollisionDamage);
                if(re.bot->get_health()==0)re.alive=false;
                if(other.bot->get_health()==0)other.alive=false;}break;}
        re.r=nr; re.c=nc; re.bot->move_to(nr,nc);
        if(m_board.get(nr,nc)==Tile::Pit){ re.bot->take_damage(9999); re.alive=false;break;}
        if(m_board.get(nr,nc)==Tile::Flame){DM::applyArmorThenDegrade(*re.bot,8);if(re.bot->get_health()==0){re.alive=false;break;}}}}

bool Arena::doTurnAndReportAction(RobotEntry& re){
    if (!re.alive || re.bot->get_health() <= 0) {
        re.alive = false;
        return false;
    }

    bool acted = false;

    // 1) Radar
    int radar_dir = 0;
    re.bot->get_radar_direction(radar_dir);
    auto hits = scanDirection(re, radar_dir);

    // (we’ll add debug printing below)
    re.bot->process_radar_results(hits);

    // 2) Shoot?
    int sr = 0, sc = 0;
    bool shot = re.bot->get_shot_location(sr, sc);

    if (shot) {
        // robot chose to shoot – no movement this turn
        std::cout << "Robot " << re.name
                  << " shoots at (" << sr << "," << sc << ")\n";
        resolveShot(re, sr, sc);
        acted = true;
    } else {
        // 3) No shot → ask for movement
        int md = 0, dist = 0;
        re.bot->get_move_direction(md, dist);
        if (md != 0 && dist > 0) {
            std::cout << "Robot " << re.name
                      << " moves: dir=" << md
                      << " dist=" << dist << "\n";
            applyMovement(re, md, dist);
            acted = true;
        } else {
            std::cout << "Robot " << re.name << " does nothing.\n";
        }
    }

    return acted;
}

void Arena::run(int max_rounds){
    std::cout<<"Starting Robot Warz on "<<m_board.rows()<<"x"<<m_board.cols()<<" board.\n";
    int round=0;
    rounds_since_action = 0;
    while(round<max_rounds && aliveCount()>1){
        ++round;
        std::cout<<"\n=== Round "<<round<<" ===\n";
        bool any_action=false;
        for(auto& re : m_robots){
            if(aliveCount()<=1) break;
            any_action=doTurnAndReportAction(re)||any_action;}
        printBoard(std::cout);
        for(auto& re : m_robots)
            std::cout<<re.bot->print_stats()<<(re.alive? "" : "  [DEAD]")<<"\n";
        if(any_action) rounds_since_action = 0;
        else           rounds_since_action++;
        if(rounds_since_action >= STALEMATE_ROUNDS && aliveCount()>1){
            std::cout <<"\n=== Stalemate Reached (" << STALEMATE_ROUNDS
                      <<" rounds without actions). Co-winners: ===\n";
            for (auto& re : m_robots)
                if(re.alive && re.bot->get_health()>0)std::cout<<re.name<<"\n";
            return;}}
    std::cout << "\n=== Game Over ===\n";
    for (auto& re : m_robots)
        if (re.alive && re.bot->get_health()>0) std::cout << "Winner: " << re.name << "\n";}
bool Arena::isObstacle(int row, int col) const {
    // Treat any non-empty terrain as an obstacle for spawning
    Tile t = m_board.get(row, col);
    return (t == Tile::Mound ||
            t == Tile::Pit   ||
            t == Tile::Flame);
}

bool Arena::hasRobot(int row, int col) const {
    // Any robot (alive or dead) blocks this tile for spawning
    return occupiedAny(row, col, nullptr);
}

bool Arena::hasAdjacentRobot(int row, int col) const {
    // True if any of the 8 neighbors contains a robot
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = row + dr;
            int nc = col + dc;
            if (!m_board.inBounds(nr, nc)) continue;
            if (hasRobot(nr, nc)) return true;
        }
    }
    return false;
}
bool Arena::isValidSpawn(int r, int c) const {
    if (r < 0 || r >= m_board.rows() || c < 0 || c >= m_board.cols()) return false;
    if (isObstacle(r, c)) return false;
    if (hasRobot(r, c))   return false;

    // enforce at least 1 tile away from other robots
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr;
            int nc = c + dc;
            if (nr < 0 || nr >= m_board.rows() || nc < 0 || nc >= m_board.cols()) continue;
            if (hasRobot(nr, nc)) return false;
        }
    }
    return true;
}
#include <random>

void Arena::addRobotRandom(RobotBase* robot,
                           const std::string& name,
                           char symbol) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> rowDist(0, m_board.rows() - 1);
    std::uniform_int_distribution<int> colDist(0, m_board.cols() - 1);

    int attempts = 0;
    const int maxAttempts = 1000; // safety

    while (attempts < maxAttempts) {
        ++attempts;
        int r = rowDist(rng);
        int c = colDist(rng);
        if (isValidSpawn(r, c)) {
            addRobot(robot, name, symbol, r, c);
            return;
        }
    }

    // If we get here, we failed to find a valid spawn. Later, might either:
    // - throw, or
    // - fall back to a relaxed rule (like ignore adjacency), or
    // - just don't add the robot.
    // For now:
    std::cerr << "Warning: could not place robot " << name
              << " after " << maxAttempts << " attempts.\n";
}
