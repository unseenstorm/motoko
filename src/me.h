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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

typedef struct {
	dword id;
	word x;
	word y;
	byte area;
	byte act;
	byte ingame;
	byte hp;
	byte mp;
} bot_t;


#define SETTER_PROTO(type, name) void me_set_##name(type)
#define SETTER(type, name) void me_set_##name(type _##name) { \
        pthread_mutex_lock(&me_m);                            \
        me.name = _##name;                                    \
        pthread_mutex_unlock(&me_m);                          \
    }

SETTER_PROTO(dword, id);
SETTER_PROTO(word, x);
SETTER_PROTO(word, y);
SETTER_PROTO(byte, area);
SETTER_PROTO(byte, act);
SETTER_PROTO(byte, ingame);
SETTER_PROTO(byte, hp);
SETTER_PROTO(byte, mp);

void me_debug(void);
void me_init(void);
void me_finit(void);


#endif /* ME_H_ */
