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

#ifndef ME_H_
#define ME_H_

#include <util/types.h>
#include <util/net.h>
#include <util/system.h>
#include <util/list.h>
#include <util/string.h>

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// setsocketopt
#include <sys/socket.h>
#include <sys/types.h>
// struct timeval
#include <sys/time.h>

#include <config.h>

#include <math.h>

#include "internal.h"
#include "bncs.h"
#include "settings.h"
#include "mcp.h"
#include "moduleman.h"
#include "packet.h"
#include "gui.h"
#include "d2gs.h"

#define module_setting(name) ((struct setting *)list_find(&module_settings_list, (comparator_t) compare_setting, name))
#define object_clear(o) memset(&(o), 0, sizeof(object_t))

#define FATAL(expr) if (!(expr)) return FALSE

#define MIN_WALKING_DISTANCE  3
#define MAX_WALKING_DISTANCE  35

#define MIN_TELEPORT_DISTANCE 5
#define MAX_TELEPORT_DISTANCE 45

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define SQUARE(x) ((x) * (x))
#define DISTANCE(a, b) ((int) sqrt((double) (SQUARE((a).x - (b).x) + SQUARE((a).y - (b).y))))
#define PERCENT(a, b) ((a) != 0 ? (int) (((double) (b) / (double) (a)) * 100) : 0)

#define IN_TOWN(area) (area == 1 || area == 40 || area == 75 || area == 103 || area == 109)
#define IN_ACT1(area) (area >= 1 && area < 40)
#define IN_ACT2(area) (area >= 40 && area < 75)
#define IN_ACT3(area) (area >= 75 && area < 103)
#define IN_ACT4(area) (area >= 103 && area < 109)
#define IN_ACT5(area) (area >= 109)
#define ACT(area) (IN_ACT1(area) ? 1 : (IN_ACT2(area) ? 2 : (IN_ACT3(area) ? 3 : (IN_ACT4(area) ? 4 : 5))))
#define TOWN_AREA(act) (act == 1 ? 1 : (act == 2 ? 40 : (act == 3 ? 75 : (act == 4 ? 103 : 109))))

typedef enum {
	VOID = 0,
	WP,
	TP,
	ORMUS,
	FARA,
	LYSANDER,
	GREIZ
} e_town_move;


typedef struct {
	word x;
	word y;
} point_t;

typedef struct {
	dword id;
	point_t location;
} object_t;

typedef struct {
	object_t obj;
	dword gold; //inventory only
	word ping; //ms
	word hp; //doesn't count any bonus
	word mp; //idem
	byte area;
	byte act; //act is updated before area
	byte ingame;
	byte intown;
	word lskill;
	word rskill;
} bot_t;


#define SETTER_PROTO(obj, type, name) void obj##_set_##name(type)
#define SETTER(obj, type, name) void obj##_set_##name(type _##name) {	\
		if (obj.name == _##name) {										\
			return;														\
		}																\
		pthread_mutex_lock(&me_m);										\
		obj.name = _##name;												\
		pthread_mutex_unlock(&me_m);									\
		plugin_debug("me", #obj"."#name" updated\n");					\
		me_debug();														\
	}

SETTER_PROTO(me, dword, id);
SETTER_PROTO(me, word, x);
SETTER_PROTO(me, word, y);
SETTER_PROTO(me, dword, gold);
SETTER_PROTO(me, word, ping);
SETTER_PROTO(me, word, hp);
SETTER_PROTO(me, word, mp);
SETTER_PROTO(me, byte, area);
SETTER_PROTO(me, byte, act);
SETTER_PROTO(me, byte, ingame);
SETTER_PROTO(me, byte, intown);
SETTER_PROTO(me, word, lskill);
SETTER_PROTO(me, word, rskill);

void me_debug(void);
void me_init(void);
void me_finit(void);

void me_leave(void);

void me_cast_self(object_t *obj, dword arg);
void me_cast_right(object_t *obj, dword arg);
void me_cast_left(object_t *obj, dword arg);
void me_swap_right(object_t *obj, dword arg);
void me_swap_left(object_t *obj, dword arg);

bool me_walk(int x, int y);
bool me_teleport(int x, int y);

bool me_move(int x, int y);

extern bot_t me;

#endif /* ME_H_ */
