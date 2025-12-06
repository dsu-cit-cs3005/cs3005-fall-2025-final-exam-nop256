//Arena.cpp
#include "Arena.h"
#include "DamageModel.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <iomanip>
#include <thread>
#include <chrono>
#include <fstream>


extern const std::pair<int,int> directions[9];

Arena::Arena(int rows,int cols)
    :m_board(rows,cols)
{
    //m_board.seedDefaultFeatures();
    seedRandomTerrain();
}
Arena::~Arena(){ for(auto& re : m_robots) delete re.bot;}

namespace {
    const char ID_POOL[] = {'!','@','#','$','%','^','&','*'};
    int id_pool_index = 0;

    char nextIdGlyph() {
        char ch = ID_POOL[id_pool_index];
        id_pool_index = (id_pool_index + 1) % (int)(sizeof(ID_POOL)/sizeof(ID_POOL[0]));
        return ch;
    }
}

void Arena::addRobot(RobotBase* robot,
                     const std::string& name,
                     char weaponGlyph,
                     int row,
                     int col) {
    RobotEntry r;
    r.bot         = robot;
    r.name        = name;
    r.weaponGlyph = weaponGlyph;
    r.idGlyph     = nextIdGlyph();
    r.r           = row;
    r.c           = col;
    r.alive       = true;

    r.bot->set_boundaries(m_board.rows(), m_board.cols());
    r.bot->move_to(row, col);
    r.bot->m_name      = name;
    r.bot->m_character = weaponGlyph;

    m_robots.push_back(r);
}


int Arena::aliveCount()const{
    int n=0; for(auto& r: m_robots)if(r.alive && r.bot->get_health()>0) n++;
    return n;}


bool Arena::occupied(int r,int c, int* idx_out) const{
    for(size_t i=0;i<m_robots.size();++i){
        const auto& e=m_robots[i];
        if(e.alive && e.r==r && e.c==c){ if(idx_out) *idx_out=(int)i; return true;}}return false;}

char Arena::boardCharAt(int r, int c) const {
    int idx = -1;
    if (occupiedAny(r, c, &idx) && idx >= 0) {
        const auto &e = m_robots[idx];
        if (e.alive && e.bot->get_health() > 0) {
            return e.bot->m_character;
        } else {
            return 'X';
        }
    }

    Tile t = m_board.get(r, c);
    switch (t) {
        case Tile::Mound: return 'M';
        case Tile::Pit:   return 'P';
        case Tile::Flame: return 'F';
        default:          return '.';
    }
}
void Arena::printBoard(std::ostream& os) const {
    os << "\n\n\n    ";
    for (int c = 0; c < m_board.cols(); ++c) {
        os << std::setw(3) << c << " "; 
    }
    os << '\n' << ' ';

    for (int r = 0; r < m_board.rows(); ++r) {
        os << std::setw(2) << r << "  ";
        for (int c = 0; c < m_board.cols(); ++c) {
            os << boardCellString(r,c) << ' ' << ' ';             
        }
        os << '\n' << '\n' << ' ';
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
        if (e.r==r && e.c==c){
            if (idx_out) *idx_out=(int)i;
            return true;}}
    return false;}

std::vector<RadarObj> Arena::scanDirection(const RobotEntry& re,int dir) const {
    std::vector<RadarObj> out;
    if (dir < 1 || dir > 8) return out;

    int dr = directions[dir].first;
    int dc = directions[dir].second;
    int r  = re.r + dr;
    int c  = re.c + dc;

    while (m_board.inBounds(r,c)) {
        int idx = -1;
        if (occupiedAny(r,c,&idx) && idx >= 0) {
            const auto& e = m_robots[idx];

            char type;
            if (e.alive && e.bot->get_health() > 0) {
                type = e.weaponGlyph;
            } else {
                type = 'X';
            }

            out.emplace_back(type, r, c);
        }
        Tile t = m_board.get(r,c);
        if (t == Tile::Mound) {
            out.emplace_back('M', r, c);
        }
        if (t == Tile::Pit) {
            out.emplace_back('P', r, c);
        }
        if (t == Tile::Flame) {
            out.emplace_back('F', r, c);
        }

        r += dr;
        c += dc;
    }
    return out;
}



static constexpr bool SHOTS_BLOCKED_BY_BODIES=false;
static constexpr bool SHOTS_BLOCKED_BY_MOUNDS=false;

void Arena::resolveShot(const RobotEntry& shooterEntry, int shot_r, int shot_c) {
    if (!m_board.inBounds(shooterEntry.r, shooterEntry.c)) return;

    auto weapon = shooterEntry.bot->get_weapon();

    switch (weapon) {
        case railgun:
            resolveRailgunShot(shooterEntry, shot_r, shot_c);
            break;
        case flamethrower:
            resolveFlameShot(shooterEntry, shot_r, shot_c);
            break;
        case hammer:
            resolveHammerAttack(shooterEntry, shot_r, shot_c);
            break;
        case grenade:
            resolveGrenade(shooterEntry, shot_r, shot_c);
            break;
        default:
            break;
    }
}

void Arena::resolveRailgunShot(const RobotEntry& shooterEntry, int shot_r, int shot_c){
    int sr = shooterEntry.r, sc = shooterEntry.c;
    int dr = (shot_r > sr) ? 1 : (shot_r < sr ? -1 : 0);
    int dc = (shot_c > sc) ? 1 : (shot_c < sc ? -1 : 0);

    if (!((sr == shot_r) || (sc == shot_c) ||
          (std::abs(shot_r - sr) == std::abs(shot_c - sc)))) return;

    int shooterIdx = -1;
    for (int i = 0; i < (int)m_robots.size(); ++i) {
        if (&m_robots[i] == &shooterEntry) {
            shooterIdx = i;
            break;
        }
    }

    if (shooterIdx < 0) return;
    auto& shooter = m_robots[shooterIdx];
    shooter.shotsFired++;

    int r = sr + dr, c = sc + dc;
    while (m_board.inBounds(r,c)){
        if (SHOTS_BLOCKED_BY_MOUNDS && m_board.get(r,c) == Tile::Mound) break;

        int idx = -1;
        if (occupiedAny(r,c,&idx) && idx >= 0) {
            auto& tgt = m_robots[idx];

            if (tgt.alive) {
                int before = tgt.bot->get_health();
                DM::applyArmorThenDegrade(*tgt.bot, DM::RailgunDamage);
                int after = tgt.bot->get_health();
                int dealt = before - after;

                shooter.shotsHit++;
                shooter.damageDealt += dealt;
                tgt.damageTaken += dealt;

                if (dealt > 0) {
                    m_damage_or_death_this_round = true;
                }

                std::cout << "EVENT,SHOT,"
                             << shooter.name << ","
                             << tgt.name << ","
                             << std::to_string(r) << ","
                             << std::to_string(c) << ","
                             << std::to_string(dealt);

                if (after == 0) {
                    tgt.alive = false;
                    tgt.died = true;
                    tgt.deathRow = tgt.r;
                    tgt.deathCol = tgt.c;
                    tgt.causeOfDeath =
                        "railgun from " + shooter.name;
                    shooter.kills++;
                    m_damage_or_death_this_round = true;

                    std::cout << "EVENT,KILL,"
                                 << shooter.name << ","
                                 << tgt.name + ",railgun";
                }
            }

            if (SHOTS_BLOCKED_BY_BODIES) break;
        }

        r += dr;
        c += dc;
    }
}

void Arena::resolveFlameShot(const RobotEntry& shooterEntry,
                             int shot_r, int shot_c) {
    int sr = shooterEntry.r, sc = shooterEntry.c;
    int dr = (shot_r > sr) ? 1 : (shot_r < sr ? -1 : 0);
    int dc = (shot_c > sc) ? 1 : (shot_c < sc ? -1 : 0);

    if (dr == 0 && dc == 0) return;

    int shooterIdx = -1;
    for (int i = 0; i < (int)m_robots.size(); ++i) {
        if (&m_robots[i] == &shooterEntry) { shooterIdx = i; break; }
    }
    if (shooterIdx < 0) return;
    auto& shooter = m_robots[shooterIdx];
    shooter.shotsFired++;

    const int MAX_RANGE = 4;
    const int HALF_WIDTH = 1;
    for (int step = 1; step <= MAX_RANGE; ++step) {
        int center_r = sr + dr * step;
        int center_c = sc + dc * step;
        if (!m_board.inBounds(center_r, center_c)) break;

        for (int off = -HALF_WIDTH; off <= HALF_WIDTH; ++off) {
            int r = center_r;
            int c = center_c;

            if (dr == 0 && dc != 0) {
                r = center_r + off;
            } else if (dc == 0 && dr != 0) {
                c = center_c + off;
            } else {
                r = center_r + off;
                c = center_c + off;
            }

            if (!m_board.inBounds(r,c)) continue;

            int idx = -1;
            if (occupiedAny(r,c,&idx) && idx >= 0) {
                auto& tgt = m_robots[idx];
                if (!tgt.alive) continue;

                int before = tgt.bot->get_health();
                DM::applyArmorThenDegrade(*tgt.bot, DM::FlamethrowerDamage); 
                int after = tgt.bot->get_health();
                int dealt = before - after;

                if (dealt > 0) {
                    shooter.shotsHit++;
                    shooter.damageDealt += dealt;
                    tgt.damageTaken     += dealt;
                    m_damage_or_death_this_round = true;
                }

                if (after == 0) {
                    tgt.alive = false;
                    tgt.died  = true;    
                    tgt.deathRow = tgt.r;
                    tgt.deathCol = tgt.c;
                    tgt.causeOfDeath = "flamethrower from " + shooter.name;
                    shooter.kills++;
                    m_damage_or_death_this_round = true;
                }
            }

        }
    }
}

void Arena::resolveHammerAttack(const RobotEntry& shooterEntry,
                                int /*shot_r*/, int /*shot_c*/) {
    int sr = shooterEntry.r, sc = shooterEntry.c;

    int shooterIdx = -1;
    for (int i = 0; i < (int)m_robots.size(); ++i) {
        if (&m_robots[i] == &shooterEntry) { shooterIdx = i; break; }
    }
    if (shooterIdx < 0) return;
    auto& shooter = m_robots[shooterIdx];
    shooter.shotsFired++;

    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int r = sr + dr;
            int c = sc + dc;
            if (!m_board.inBounds(r,c)) continue;

            int idx = -1;
            if (!occupiedAny(r,c,&idx) || idx < 0) continue;
            auto& tgt = m_robots[idx];
            if (!tgt.alive) continue;

            int before = tgt.bot->get_health();
            DM::applyArmorThenDegrade(*tgt.bot, DM::HammerDamage); 
            int after = tgt.bot->get_health();
            int dealt = before - after;

            if (dealt > 0) {
                shooter.shotsHit++;
                shooter.damageDealt += dealt;
                tgt.damageTaken     += dealt;
                m_damage_or_death_this_round = true;
            }

            if (after == 0) {
                tgt.alive = false;
                tgt.died  = true;
                tgt.deathRow = tgt.r;
                tgt.deathCol = tgt.c;
                tgt.causeOfDeath = "hammer from " + shooter.name;
                shooter.kills++;
                m_damage_or_death_this_round = true;
            }
        }
    }
}

void Arena::resolveGrenade(const RobotEntry& shooterEntry,
                           int shot_r, int shot_c) {
    int sr = shooterEntry.r, sc = shooterEntry.c;

    int shooterIdx = -1;
    for (int i = 0; i < (int)m_robots.size(); ++i) {
        if (&m_robots[i] == &shooterEntry) { shooterIdx = i; break; }
    }
    if (shooterIdx < 0) return;
    auto& shooter = m_robots[shooterIdx];

    // limit range
    const int MAX_RANGE = 6;
    int manhattan = std::abs(shot_r - sr) + std::abs(shot_c - sc);
    if (manhattan > MAX_RANGE) return;

    shooter.shotsFired++;

    const int BLAST_RADIUS = 1; 
    for (int dr = -BLAST_RADIUS; dr <= BLAST_RADIUS; ++dr) {
        for (int dc = -BLAST_RADIUS; dc <= BLAST_RADIUS; ++dc) {
            int r = shot_r + dr;
            int c = shot_c + dc;
            if (!m_board.inBounds(r,c)) continue;

            int idx = -1;
            if (!occupiedAny(r,c,&idx) || idx < 0) continue;
            auto& tgt = m_robots[idx];
            if (!tgt.alive) continue;

            int before = tgt.bot->get_health();
            DM::applyArmorThenDegrade(*tgt.bot, DM::GrenadeDamage);
            int after = tgt.bot->get_health();
            int dealt = before - after;

            if (dealt > 0) {
                shooter.shotsHit++;
                shooter.damageDealt += dealt;
                tgt.damageTaken     += dealt;
                m_damage_or_death_this_round = true;
            }

            if (after == 0) {
                tgt.alive = false;
                tgt.died  = true;
                tgt.deathRow = tgt.r;
                tgt.deathCol = tgt.c;
                tgt.causeOfDeath = "grenade from " + shooter.name;
                shooter.kills++;
                m_damage_or_death_this_round = true;
            }
        }
    }
}


void Arena::applyMovement(RobotEntry& re, int dir, int dist) {
    if (!re.alive || re.trappedInPit) return;
    if (dir < 1 || dir > 8 || dist <= 0) return;

    dist = std::min(dist, re.bot->get_move_speed());
    int dr = directions[dir].first;
    int dc = directions[dir].second;

    for (int step = 0; step < dist; ++step) {
        int nr = re.r + dr;
        int nc = re.c + dc;

        if (!m_board.inBounds(nr, nc)) break;
        if (m_board.get(nr, nc) == Tile::Mound) break;

        int idx = -1;
        if (occupiedAny(nr, nc, &idx)) {
            auto& other = m_robots[idx];
            if (other.alive) {
                int beforeA = re.bot->get_health();
                int beforeB = other.bot->get_health();

                DM::applyArmorThenDegrade(*re.bot,   DM::CollisionDamage);
                DM::applyArmorThenDegrade(*other.bot, DM::CollisionDamage);

                int afterA = re.bot->get_health();
                int afterB = other.bot->get_health();

                int dealtA = beforeA - afterA;
                int dealtB = beforeB - afterB;

                re.damageTaken    += dealtB;
                other.damageTaken += dealtA;

                if (dealtA > 0 || dealtB > 0) {
                    m_damage_or_death_this_round = true;
                }
                if (afterA == 0 && !re.died) {
                    re.alive = false;
                    re.died  = true;
                    re.causeOfDeath = "collision with " + other.name;
                    m_damage_or_death_this_round = true;
                    re.deathRow = re.r;
                    re.deathCol = re.c;
                }
                if (afterB == 0 && !other.died) {
                    other.alive = false;
                    other.died  = true;
                    other.causeOfDeath = "collision with " + re.name;
                    m_damage_or_death_this_round = true;
                    other.deathRow = other.r;
                    other.deathCol = other.c;
                }
            }
            break;
        }

        re.r = nr;
        re.c = nc;
        re.bot->move_to(nr, nc);

        Tile t = m_board.get(nr, nc);
        if (t == Tile::Pit) {
            re.trappedInPit = true;

            break;
        }

        if (m_board.get(nr, nc) == Tile::Flame) {
            static std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<int> flameDmgDist(30, 50); //30 to 50 damage per spec
            int dmg = flameDmgDist(rng);

            int before = re.bot->get_health();
            DM::applyArmorThenDegrade(*re.bot, dmg);  //now uses armor ANDdegrades it
            int after = re.bot->get_health();
            int dealt = before - after;
            re.damageTaken += dealt;

            if (dealt > 0) {
                m_damage_or_death_this_round = true;
            }

            if (after == 0 && !re.died) {
                re.alive = false;
                re.died  = true;
                re.causeOfDeath = "flame";
                re.deathRow = nr;
                re.deathCol = nc;
                m_damage_or_death_this_round = true;
            }

            if (!re.alive) break;
        }
    }
}


bool Arena::doTurnAndReportAction(RobotEntry& re){
    if (!re.alive || re.bot->get_health() <= 0) {
        re.alive = false;
        return false;
    }

    bool acted = false;

    int radar_dir = 0;
    re.bot->get_radar_direction(radar_dir);
    auto hits = scanDirection(re, radar_dir);

    re.bot->process_radar_results(hits);

    int sr = 0, sc = 0;
    bool shot = re.bot->get_shot_location(sr, sc);

    if (shot) {
        std::cout << "Robot " << re.name << re.weaponGlyph << re.idGlyph
                  << " shoots at (" << sr << "," << sc << ")\n";
        resolveShot(re, sr, sc);
        acted = true;
    } else {
        int md = 0, dist = 0;
        re.bot->get_move_direction(md, dist);
        if (md != 0 && dist > 0) {
            std::cout << "Robot " << re.name << re.weaponGlyph << re.idGlyph
                      << " moves: dir=" << md
                      << " dist=" << dist << "\n";
            applyMovement(re, md, dist);
            acted = true;
        } else {
            std::cout << "Robot " << re.name << re.weaponGlyph << re.idGlyph << " does nothing.\n";
        }
    }
    return acted;
}

//RUN: GAME LOOP
void Arena::run(int ms_delay_between_rounds){
    std::cout << "Starting Robot Warz on "
              << m_board.rows() << "x" << m_board.cols() << " board.\n";

    int round = 0;
    rounds_since_action = 0;

    auto now = std::time(nullptr);
    long gameId = static_cast<long>(now);

    while (aliveCount() > 1) {     
        ++round;
        std::cout << "\n\n";
        std::cout << "\n=== Round " << round << " ===\n";

        m_damage_or_death_this_round = false;

        bool any_action = false;
        for (auto& re : m_robots) {
            if (aliveCount() <= 1) break;
            any_action = doTurnAndReportAction(re) || any_action;
        }

        printBoard(std::cout);

        for (auto& re : m_robots) {
            std::cout << "[" << re.weaponGlyph << re.idGlyph << "] "
                      << re.bot->print_stats()
                      << (re.alive ? "" : "  [DEAD]") << "\n";
        }

        if (m_damage_or_death_this_round) {
            rounds_since_action = 0;
        } else {
            ++rounds_since_action;
        }

        for (auto& re : m_robots) {
            if (re.alive && re.bot->get_health() > 0) {
                re.roundsAlive++;
            }
        }

        // stalemate check
        if (rounds_since_action >= STALEMATE_ROUNDS && aliveCount() > 1) {
            std::cout << "\n=== Stalemate Reached ("
                      << STALEMATE_ROUNDS
                      << " rounds without damage or kills). Co-winners: ===\n";
            for (auto& re : m_robots) {
                if (re.alive && re.bot->get_health() > 0) {
                    std::cout << re.name << "\n";
                }
            }
            writeReaperStats(gameId);
            return;
        }

        if (ms_delay_between_rounds > 0) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(ms_delay_between_rounds)
            );
        }
    }
    writeReaperStats(gameId);

    std::cout << "\n=== Game Over ===\n";
    for (auto& re : m_robots) {
        if (re.alive && re.bot->get_health() > 0) {
            std::cout << "Winner: " << re.name << "\n";
        }
    }
}

void Arena::writeReaperStats(long gameId) {
    std::ofstream reaperFile("reaper_only_stats.csv", std::ios::app);
    if (!reaperFile) return;

    if (reaperFile.tellp() == 0) {
        reaperFile
            << "gameId,name,weapon,won,"
            << "shotsFired,shotsHit,kills,"
            << "damageDealt,damageTaken,causeOfDeath,"
            << "roundsSurvived,deathRow,deathCol,timesStuck\n";
    }

    for (auto& re : m_robots) {
        bool isMyReaper =
            (re.bot->get_weapon() == railgun) &&
            (re.name.rfind("Reaper_", 0) == 0); 

        if (!isMyReaper) continue;

        bool survived = (re.alive && re.bot->get_health() > 0);
        std::string weapon(1, re.weaponGlyph);

        reaperFile
            << gameId << ","
            << re.name << ","
            << weapon << ","
            << (survived ? "1" : "0") << ","
            << re.shotsFired << ","
            << re.shotsHit << ","
            << re.kills << ","
            << re.damageDealt << ","
            << re.damageTaken << ","
            << (re.died ? re.causeOfDeath : "alive") << ","
            << re.roundsAlive << ","
            << re.deathRow << ","
            << re.deathCol << ","
            << 0
            << "\n";
    }
}


bool Arena::isObstacle(int row, int col) const {
    Tile t = m_board.get(row, col);
    return (t == Tile::Mound ||
            t == Tile::Pit   ||
            t == Tile::Flame);
}

bool Arena::hasRobot(int row, int col) const {
    return occupiedAny(row, col, nullptr);
}

bool Arena::hasAdjacentRobot(int row, int col) const {
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

    std::cerr << "Warning: could not place robot " << name
              << " after " << maxAttempts << " attempts.\n";
}

std::string Arena::boardCellString(int r, int c) const {
    int idx = -1;
    if (occupiedAny(r, c, &idx) && idx >= 0) {
        const auto& e = m_robots[idx];

        char base = (e.alive && e.bot->get_health() > 0)
                    ? e.weaponGlyph    
                    : 'X';             // dead

        std::string s;
        s += base;
        s += e.idGlyph;                
        return s;
    }

    Tile t = m_board.get(r, c);
    switch (t) {
        case Tile::Mound: return " M";
        case Tile::Pit:   return " P";
        case Tile::Flame: return " F";
        default:          return " .";
    }
}

void Arena::seedRandomTerrain()
{
    static std::mt19937 rng(std::random_device{}());

    std::uniform_int_distribution<int> rowDist(0, m_board.rows() - 1);
    std::uniform_int_distribution<int> colDist(0, m_board.cols() - 1);

    auto isEmpty = [&](int r, int c) {
        // Replace Tile::Empty with whatever your “open” tile is
        Tile t = m_board.get(r, c);
        return (t != Tile::Mound &&
                t != Tile::Pit   &&
                t != Tile::Flame);
    };

    auto placeMany = [&](Tile tileType, int count) {
        int placed   = 0;
        int attempts = 0;
        const int MAX_ATTEMPTS = 10'000;

        while (placed < count && attempts < MAX_ATTEMPTS) {
            ++attempts;
            int r = rowDist(rng);
            int c = colDist(rng);

            if (!isEmpty(r, c)) continue;

            m_board.set(r, c, tileType);
            ++placed;
        }
    };

    std::uniform_int_distribution<int> pitCountDist(1, 3);
    std::uniform_int_distribution<int> flameCountDist(2, 5);
    std::uniform_int_distribution<int> moundCountDist(3, 5);

    int numPits   = pitCountDist(rng);
    int numFlames = flameCountDist(rng);
    int numMounds = moundCountDist(rng);

    placeMany(Tile::Pit,   numPits);
    placeMany(Tile::Flame, numFlames);
    placeMany(Tile::Mound, numMounds);
}
