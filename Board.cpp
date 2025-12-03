#include "Board.h"
Board::Board(int rows,int cols):m_rows(rows),m_cols(cols),m_grid(rows,std::vector<Tile>(cols,Tile::Empty)){}
void Board::seedDefaultFeatures(){
    // sprinkles a few mounds/flames/pits (handful, deterministic)
    for(int r=4;r<m_rows; r+=7){
        for(int c=3;c<m_cols; c+=9){
            if (inBounds(r,c)) m_grid[r][c]=Tile::Mound;
            if (inBounds(r+2,c+1)) m_grid[r+2][c+1]=Tile::Flame;
            if (inBounds(r+3,c+3)) m_grid[r+3][c+3]=Tile::Pit;}}}
