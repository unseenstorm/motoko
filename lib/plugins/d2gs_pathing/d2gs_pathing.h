/*
 * Copyright (C) 2016 JeanMax
 *
 * Please check the CREDITS file for further information.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.	 If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef D2GS_PATHING_H_
#define D2GS_PATHING_H_

#include <dirent.h>
#include <sys/stat.h>

#include <util/file.h>

#include "levels.h"
#include "exits.h"
#include "mazes.h"

#include <me.h>

//#define tile_new(x, y, s) (tile_t) { x, y, s, FALSE, .adjacent.north = NULL, .adjacent.east = NULL, .adjacent.south = NULL, .adjacent.west = NULL, .objects = LIST(NULL, object_t, 0), .npcs = LIST(NULL, object_t, 0) }

#define SCALE 5

#define TILE_TO_WORLD(c) ((c) * SCALE)
#define WORLD_TO_TILE(c) ((c) / SCALE)

typedef struct _tile_t {
	word x;
	word y;
	int size;
	bool visited;
	struct {
		struct _tile_t *north;
		struct _tile_t *east;
		struct _tile_t *south;
		struct _tile_t *west;
	} adjacent;
	struct list *objects;
	struct list *npcs;
} tile_t;

typedef struct _node_t {
	word x;
	word y;
	int size;
	int distance;
	bool visited;
	struct _node_t *previous;
} node_t;

typedef struct {
	int layout[512][512];
	point_t origin;
	int width;
	int height;
	int size;
} room_layout_t;

#define MAX_TILE_SIZE 14

enum {
	NORTH = 0, EAST, SOUTH, WEST, NORTHEAST, NORTHWEST, SOUTHEAST, SOUTHWEST
};

typedef struct {
	unsigned n_events;
	double p_north;
	double p_east;
	double p_south;
	double p_west;
	bool north;
	bool east;
	bool south;
	bool west;
	word objects[MAX_TILE_SIZE][MAX_TILE_SIZE];
	bool walkable[MAX_TILE_SIZE][MAX_TILE_SIZE];
} tile_data_t;

typedef struct {
	object_t object;
	byte id;
} exit_t;

// y seems to grow to south
#define northof(a, b) (((a)->x == (b)->x) && ((a)->y == (b)->y + (b)->size))
#define eastof(a, b) (((a)->y == (b)->y) && ((a)->x == (b)->x + (b)->size))
#define southof(a, b) (((a)->x == (b)->x) && ((a)->y == (b)->y - (b)->size))
#define westof(a, b) (((a)->y == (b)->y) && ((a)->x == (b)->x - (b)->size))

#define tile_contains(t, o) (((o)->location.x >= (t)->x * SCALE) && ((o)->location.x < ((t)->x + (t)->size) * SCALE) &&\
                             ((o)->location.y >= (t)->y * SCALE) && ((o)->location.y < ((t)->y + (t)->size) * SCALE))


#define north(l, x, y) ((y) > 0 && (l)->layout[(x)][(y) - 1])
#define west(l, x, y) ((x) > 0 && (l)->layout[(x) - 1][(y)])
#define south(l, x, y) ((l)->layout[(x)][(y) + 1])
#define east(l, x, y) ((l)->layout[(x) + 1][(y)])

#define list_index(l, e) ((void *) (e) >= (void *) (l)->elements && (void *) (e) < (void *) ((l)->elements + (l)->len * (l)->size) ? ((void *) (e) - (void *) (l)->elements) / (l)->size : -1) // we should inlcude this in list.c/list.h


#endif /* D2GS_PATHING_H_ */
