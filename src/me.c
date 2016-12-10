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

#include "me.h"

bot_t me;
static pthread_mutex_t me_m;

SETTER(dword, id)
SETTER(word, x)
SETTER(word, y)
SETTER(byte, area)
SETTER(byte, act)
SETTER(byte, ingame)
SETTER(byte, hp)
SETTER(byte, mp)

void me_debug() {
	printf("me: id:%d, x:%d, y:%d, area:%d, act:%d, ingame:%d, hp:%d, mp:%d\n",
		   me.id,
		   me.x,
		   me.y,
		   me.area,
		   me.act,
		   me.ingame,
		   me.hp,
		   me.mp
		);
}

void me_init() {
	pthread_mutex_init(&me_m, NULL);
	pthread_mutex_lock(&me_m);
	bzero(&me, sizeof(bot_t));
	pthread_mutex_unlock(&me_m);
}

void me_finit() {
	pthread_mutex_lock(&me_m);
	bzero(&me, sizeof(bot_t));
	pthread_mutex_unlock(&me_m);
	pthread_mutex_destroy(&me_m);
}


/* _export size_t internal_send(byte id, char *format, ...) { */
