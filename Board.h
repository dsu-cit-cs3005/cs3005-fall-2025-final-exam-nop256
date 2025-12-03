#pragma once
#include <vector>
#include "Tile.h"
class Board {
public:
    Board(int rows=20,int cols=20);
    int rows() const{return m_rows;}
    int cols() const{return m_cols;}
    bool inBounds(int r,int c) const{return r>=0 && c>=0 && r<m_rows && c<m_cols;}
    Tile get(int r,int c) const{return m_grid[r][c];}
    void set(int r,int c,Tile t){m_grid[r][c]=t;}
    // simple default layout: border is empty; a few interior features
    void seedDefaultFeatures();
private:
    int m_rows,m_cols;
    std::vector<std::vector<Tile>> m_grid;};
