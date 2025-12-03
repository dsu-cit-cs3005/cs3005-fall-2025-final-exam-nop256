#pragma once
#include "RobotBase.h"

// Tweakables — centralized:
namespace DM {
    constexpr int RailgunDamage=20;     // pierces in LOS
    constexpr int FlamethrowerDamage=12;// cone; not implemented in v1
    constexpr int GrenadeDamage=18;     // AoE; not implemented in v1
    constexpr int HammerDamage =30;     // adjacent; not implemented in v1
    constexpr int CollisionDamage=10;

    inline int applyArmorThenDegrade(RobotBase& target,int base){
        int a=target.get_armor();
        int dealt=base-a;
        if(dealt<0)dealt=0;
        target.take_damage(dealt);
        if(a>0)target.reduce_armor(1);
        return dealt;}}
