#ifndef BOID_GRAPHICS_COMMON_H
#define BOID_GRAPHICS_COMMON_H

/**
 * common functionalities between graphical aos and soa versions of the program
 */

#include<math.h>    // for sqrt
#include"raylib.h"  // for graphics
#include"config.h"  // for config

void draw_boid_coords(const float x, const float y,
					  const float dx, const float dy);

#endif
