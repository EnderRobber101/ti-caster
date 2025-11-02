#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "trig.h"
#include <debug.h>
#include <math.h>

//Settings




//!Global Variables
#define SCREEN_W 320   
#define SCREEN_H 240

uint8_t *vram = gfx_vram;


//? Mini Map related
#define MAP_X int8_t(8)         // map width (tiles)
#define MAP_Y int8_t(8)         // map height (tiles)
#define MAP_S int8_t(16)        // tile pixel size on mini-map
const int8_t map_data[MAP_X * MAP_Y] =
{
    1,1,1,1,1,1,1,1,
    1,0,1,0,0,0,0,1,
    1,0,1,0,0,0,0,1,
    1,0,1,0,0,0,0,1,
    1,0,0,0,0,0,0,1,
    1,0,0,0,0,1,0,1,
    1,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,
};
#define MINI_MAP_W (MAP_X * MAP_S)
#define MINI_MAP_H (MAP_Y * MAP_S)
#define MAP_SHIFT 4 // since 2^4 = 16

//?View related
#define FOV int8_t(64)
#define RES int8_t(20)
//* Manually set but equation would be screen size / res
#define NUM_RAYS int8_t(16)
//* Value calculated via fov/num rays
#define RAY_ANGLE int8_t(4) 
#define FOVHALF int8_t(32)
#define WALLHEIGHT int8_t(8)

//? Pre-calculated / Math related

//*For the ray horizontal
#define RH_FP_SHIFT 8
#define RH_FP_SCALE (1 << FP_SHIFT)
#define RH_MAP_MASK (~(MAP_S - 1))


//?Variables
int16_t playerX = 80;
int16_t playerY = 60;
int16_t playerAngle = 0;

int16_t playerDirX = 0; //Speed when moving
int16_t playerDirY = 0;

//* could be a define to improve performance
//* Lower numbers cause inaccurate direction
/*
* 2  = bad 
* 5  = ok
* 10 = good
*/
int8_t moveSpeed = 5;

//! core functions

//? Tools
static inline bool player_hit_wall(int16_t playerNewX, int16_t playerNewY) {
    //! Works only when map size is a power of 2
    // Convert pixel/fixed coordinate to tile coordinate
    int16_t mapPosX = (int16_t)(playerNewX >> MAP_SHIFT);
    int16_t mapPosY = (int16_t)(playerNewY >> MAP_SHIFT);
    // Fast bounds check
    if (mapPosX < 0 || mapPosX >= MAP_X || mapPosY < 0 || mapPosY >= MAP_Y)
        return true;

    // Return whether there's a wall
    return map_data[mapPosY * MAP_X + mapPosX] != 0;
}


inline int24_t distance_sq(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    int24_t dx = x2 - x1;
    int24_t dy = y2 - y1;
    return dx * dx + dy * dy;
}

//! optimize later
inline void projectWall(int24_t distanceSquare,int8_t rayNum) {
    int16_t lineHeight = (WALLHEIGHT * 256) / static_cast<int24_t>(sqrtf(distanceSquare));
    int16_t drawStart = 120 - (lineHeight >> 1);
    gfx_Rectangle(rayNum*RES,drawStart,RES,lineHeight);
    // dbg_printf("Line %d height: %u\n", rayNum, static_cast<int24_t>(sqrtf(distanceSquare)));
    // dbg_printf("Line %d height: %d\n", rayNum, ( static_cast<int24_t>(sqrtf(distanceSquare))));
    // dbg_printf("Draw Start: %d\n", drawStart);
    
}

inline void castRay(int16_t angle,int8_t rayNum) {
    bool isFacingUp = angle > 0 && angle < 180;
    bool isFacingRight = angle < 90 || angle > 270;
    
    //* Horizontal
    bool foundHorizontalWall = false;
    int16_t horizontalWallHitX = 0;
    int16_t horizontalWallHitY = 0;
    int16_t firstIntersectionX = 0;
    int16_t firstIntersectionY = 0;
    
    // if(isFacingUp) {
    //     //* 8 is a result of mapsize of 16 2^8
    //     firstIntersectionY = (playerY & RH_MAP_MASK) - 1;
    // } else {
    //     firstIntersectionY = (playerY & RH_MAP_MASK) + MAP_S;
    // }
    
    if(isFacingUp) {
        firstIntersectionY = (int)((((double)playerY / MAP_S) * MAP_S) - 0.01);
    } else {
        firstIntersectionY = ((playerY / MAP_S) * MAP_S) + MAP_S;
    }
    firstIntersectionX =  playerX + fast_cot_mul(firstIntersectionY - playerY,angle);
    
    int16_t nextHorizontalX = firstIntersectionX;
    int16_t nextHorizontalY = firstIntersectionY;
    
    int16_t xa = 0;
    int16_t ya = (isFacingUp) ? -MAP_S : MAP_S;
    
    xa = fast_cot_mul(ya,angle);
    //while inside map
    while (nextHorizontalX <= MINI_MAP_W && nextHorizontalX >= 0 && nextHorizontalY <= MINI_MAP_H && nextHorizontalY >= 0) {
        // gfx_SetColor(70);   
        // gfx_Line(playerX,playerY,nextHorizontalX,nextHorizontalY);
        if(player_hit_wall(nextHorizontalX,nextHorizontalY)) {
            foundHorizontalWall = true;
            horizontalWallHitX = nextHorizontalX;
            horizontalWallHitY = nextHorizontalY;
            break;
        } else {
            nextHorizontalX += xa;
            nextHorizontalY += ya;
        }
    }
    
    //* Testing Draw line
    // gfx_SetColor(26);
    // gfx_Line(playerX,playerY,horizontalWallHitX,horizontalWallHitY);
    // gfx_Circle(horizontalWallHitX,horizontalWallHitY,4);
    // gfx_SetColor(30);
    // gfx_Line(playerX,playerY,(playerX + fast_cos_mul(50, angle)), (playerY + fast_sin_mul(50, angle)));
    
    
    //* Vertical
    angle += 180;
    if(angle < 0) {angle += 360;}
    else if(angle > 360) {angle -= 360;}
    isFacingUp = angle > 0 && angle < 180;
    isFacingRight = angle < 90 || angle > 270;
    
    bool foundVerticalWall = false;
    int16_t verticalHitX = 0;
    int16_t verticalHitY = 0;
    if(isFacingRight) {
        firstIntersectionX = ((playerX / MAP_S) * MAP_S) + MAP_S;
    } else {
        firstIntersectionX = (int)((((double)playerX / MAP_S) * MAP_S) - 0.01);
    }
    firstIntersectionY = playerY + fast_tan_mul(firstIntersectionX - playerX, angle);
    
    int16_t nextVerticalX = firstIntersectionX;
    int16_t nextVerticalY = firstIntersectionY;
    if(isFacingRight) {
        xa = MAP_S;
    } else {
        xa = -MAP_S;
    }
    ya = fast_tan_mul(xa,angle);
    while (nextVerticalX <= MINI_MAP_W && nextVerticalX >= 0 && nextVerticalY <= MINI_MAP_H && nextVerticalY >= 0) {
        
        // gfx_SetColor(70);   
        // gfx_Line(playerX,playerY,nextVerticalX,nextVerticalY);
        if(player_hit_wall(nextVerticalX,nextVerticalY)) {
            foundVerticalWall = true;
            verticalHitX = nextVerticalX;
            verticalHitY = nextVerticalY;
            break;
        } else {
            nextVerticalX += xa;
            nextVerticalY += ya;
        }
    }
    
    // gfx_SetColor(26);
    // gfx_Line(playerX,playerY,horizontalWallHitX,horizontalWallHitY);
    // gfx_Circle(verticalHitX,verticalHitY,4);
    // gfx_SetColor(30);
    // gfx_Line(playerX,playerY,(playerX + fast_cos_mul(50, angle)), (playerY + fast_sin_mul(50, angle)));
    
    
    //* Calculate Distance
    
    int24_t horizontalDistance = INT16_MAX;
    int24_t verticalDistance = INT16_MAX;
    
    if(foundHorizontalWall) {
        horizontalDistance = distance_sq(playerX,playerY,horizontalWallHitX,horizontalWallHitY);
    }
    if(foundVerticalWall) {
        verticalDistance = distance_sq(playerX,playerY,verticalHitX,verticalHitY);
    }
    // dbg_printf("Line %d height H: %d\n", rayNum, horizontalDistance);
    // dbg_printf("Line %d height V: %d\n", rayNum, verticalDistance);
    int16_t wallHitX = 0;
    int16_t wallHitY = 0;
    if(horizontalDistance < verticalDistance) {
        wallHitX = horizontalWallHitX;
        wallHitY = horizontalWallHitY;
        // gfx_SetColor(26);
        projectWall(horizontalDistance,rayNum);
        // dbg_printf("Line %d height OUT:     %d\n", rayNum, horizontalDistance);
    } else {
        wallHitX = verticalHitX;
        wallHitY = verticalHitY;
        // gfx_SetColor(30);
        projectWall(verticalDistance,rayNum);
        // dbg_printf("Line %d height OUT:     %d\n", rayNum, verticalDistance);
    }
    //* sqrt(_distance) = distance from player
    
    
    gfx_Line(playerX,playerY,wallHitX,wallHitY);
    
    
}

//? Visual
void drawMiniMap() {
    gfx_SetColor(0);
    gfx_Rectangle_NoClip(0, 0, MINI_MAP_W, MINI_MAP_H);
    for (uint8_t y = 0; y < MAP_Y; ++y)
    {
        for (uint8_t x = 0; x < MAP_X; ++x)
        { 
            if (map_data[y * MAP_X + x] == 1) { gfx_SetColor(255); }
            else { gfx_SetColor(0); }
            gfx_FillRectangle_NoClip(x * MAP_S, y * MAP_S, MAP_S, MAP_S);
        }    
    }    
    gfx_SetColor(7);
    gfx_FillCircle(playerX, playerY, 2);
    //! Change to noclip later
    gfx_Line(playerX,playerY,(playerX + fast_cos_mul(50, playerAngle)), (playerY + fast_sin_mul(50, playerAngle)));
    
    
}    

void castAllRays() {
    int16_t rayAngle = playerAngle - FOVHALF + 180;
    
    //! change 2 back to NUM_RAYS
    for(int8_t x = 0; x < 16; x++) {
        if(rayAngle < 0) {rayAngle += 360;}
        else if(rayAngle > 360) {rayAngle -= 360;}
        //Cast each ray here
        castRay(rayAngle,x);
        // dbg_printf("%d\n", rayAngle);
        rayAngle += RAY_ANGLE;
    }
}








void initialize() {
    playerDirX = fast_cos_mul(moveSpeed,playerAngle);
    playerDirY = fast_sin_mul(moveSpeed,playerAngle);
}



int main(void) {
    
    initialize();
    gfx_Begin();
    gfx_SetDrawBuffer();
    
    bool running = true;
    while (running)
    {
        //For Testing
        gfx_FillScreen(255);
        
        kb_Scan();
        if (kb_IsDown(kb_KeyClear)) { running = false; break; }
        
        //Movement Code
        if (kb_IsDown(kb_KeyLeft)) {
            playerAngle -= 4;
            if (playerAngle <= 0) { playerAngle += 360; }
            playerDirX = fast_cos_mul(moveSpeed,playerAngle);
            playerDirY = fast_sin_mul(moveSpeed,playerAngle);
        } else if(kb_IsDown(kb_KeyRight)) {
            playerAngle += 4;
            if (playerAngle >= 0) { playerAngle -= 360; }
            playerDirX = fast_cos_mul(moveSpeed,playerAngle);
            playerDirY = fast_sin_mul(moveSpeed,playerAngle);
        }
        
        //Forward/Back
        if (kb_IsDown(kb_KeyUp)) {
            int16_t newX = playerX + playerDirX;
            int16_t newY = playerY + playerDirY;
            //Add collision check
            if (!player_hit_wall(newX, newY)) {
                playerX = newX;
                playerY = newY;
            }
        }
        if (kb_IsDown(kb_KeyDown)) {
            int16_t newX = playerX - playerDirX;
            int16_t newY = playerY - playerDirY;
            //Add collision check
            if (!player_hit_wall(newX, newY)) {
                playerX = newX;
                playerY = newY;
            }
        }
        
        
        //Render Code
        drawMiniMap();
        
        castAllRays();
       
        // castRay(45);
        // break;
        gfx_SwapDraw();
        // sleep(1);
    }
    
    gfx_End();
    return 0;
    
}