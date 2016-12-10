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

#include "internal.h"
#include "bncs.h"
#include "settings.h"
#include "mcp.h"
#include "moduleman.h"
#include "packet.h"
#include "gui.h"
#include "d2gs.h"


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
} bot_t;


#define SETTER_PROTO(type, name) void me_set_##name(type)
#define SETTER(type, name) void me_set_##name(type _##name) {	\
		if (me.name == _##name) {								\
			return;												\
		}														\
		pthread_mutex_lock(&me_m);								\
		me.name = _##name;										\
		pthread_mutex_unlock(&me_m);							\
		plugin_debug("me", "me."#name" updated\n");				\
		me_debug();												\
	}

SETTER_PROTO(dword, id);
SETTER_PROTO(word, x);
SETTER_PROTO(word, y);
SETTER_PROTO(dword, gold);
SETTER_PROTO(word, ping);
SETTER_PROTO(word, hp);
SETTER_PROTO(word, mp);
SETTER_PROTO(byte, area);
SETTER_PROTO(byte, act);
SETTER_PROTO(byte, ingame);
SETTER_PROTO(byte, intown);

void me_debug(void);
void me_init(void);
void me_finit(void);


#endif /* ME_H_ */
