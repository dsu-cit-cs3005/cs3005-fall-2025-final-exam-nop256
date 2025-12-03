#pragma once
enum class Tile {Empty,Mound,Pit,Flame};
inline char tileGlyph(Tile t){
    switch(t){
        case Tile::Empty:return '.';
        case Tile::Mound:return 'M';
        case Tile::Pit:  return 'P';
        case Tile::Flame:return 'F';}
    return '?';}
