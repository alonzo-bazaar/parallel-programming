#include "graphics-common.h"

void draw_boid_coords(const float x, const float y,
					  const float dx, const float dy) {
	static const float triangle_length=10.5;
	static const float triangle_width=6.8;
 
	float speed_norm = sqrt((dx*dx) + (dy*dy));
	float normalized_dx = dx / speed_norm;
	float normalized_dy = dy / speed_norm;
 
	Vector2 tip = {
		.x = x + (normalized_dx * triangle_length),
		.y = y + (normalized_dy * triangle_length),
	};
	Vector2 right = {
		.x = x - (normalized_dy * triangle_width / 2),
		.y = y + (normalized_dx * triangle_width / 2),
	};
	Vector2 left = {
		.x = x + (normalized_dy * triangle_width / 2),
		.y = y - (normalized_dx * triangle_width / 2),
	};
 
	// vertices must be specified in counterclockwise order
	DrawTriangle(tip, left, right, WHITE);
}
