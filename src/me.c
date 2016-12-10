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

#include "me.h"

bot_t me;
static pthread_mutex_t me_m;

void me_set_id(dword _id) {
	if (me.obj.id == _id) {
		return;
	}
	pthread_mutex_lock(&me_m);
	me.obj.id = _id;
	pthread_mutex_unlock(&me_m);
	plugin_debug("me", "me.id updated\n");
	me_debug();
}

void me_set_x(word _x) {
	if (me.obj.location.x == _x) {
		return;
	}
	pthread_mutex_lock(&me_m);
	me.obj.location.x = _x;
	pthread_mutex_unlock(&me_m);
	plugin_debug("me", "me.x updated\n");
	me_debug();
}

void me_set_y(word _y) {
	if (me.obj.location.y == _y) {
		return;
	}
	pthread_mutex_lock(&me_m);
	me.obj.location.y = _y;
	pthread_mutex_unlock(&me_m);
	plugin_debug("me", "me.y updated\n");
	me_debug();
}

void me_set_area(byte _area) {
	if (!_area || me.area == _area) {
		return;
	}
	byte act = ACT(_area);
	pthread_mutex_lock(&me_m);
	me.area = _area;
	if (me.act != act) {
		me.act = act;
	}
	if (IN_TOWN(_area)) {
		me.intown = TRUE;
	}
	pthread_mutex_unlock(&me_m);
	plugin_debug("me", "me.area updated\n");
	me_debug();
}

void me_set_act(byte _act) {
	if (!_act || me.act == _act) {
		return;
	}
	pthread_mutex_lock(&me_m);
	me.act = _act;
	if (!me.area && me.intown) {
		me.area = TOWN_AREA(_act);
	}
	pthread_mutex_unlock(&me_m);
	plugin_debug("me", "me.act updated\n");
	me_debug();
}

SETTER(dword, gold)
SETTER(word, ping)
SETTER(word, hp)
SETTER(word, mp)
SETTER(byte, ingame)
SETTER(byte, intown)



void me_debug() {
	plugin_debug("me", "id:%u, x:%d, y:%d, gold:%u, ping:%d, hp:%d, mp:%d, area:%d, act:%d, ingame:%d, intown:%d\n",
				 me.obj.id,
				 me.obj.location.x,
				 me.obj.location.y,
				 me.gold,
				 me.ping,
				 me.hp,
				 me.mp,
				 me.area,
				 me.act,
				 me.ingame,
				 me.intown);
}

void me_init() {
	pthread_mutex_init(&me_m, NULL);
	pthread_mutex_lock(&me_m);
	bzero(&me, sizeof(bot_t));
	me.intown = TRUE;
	pthread_mutex_unlock(&me_m);
}

void me_finit() {
	pthread_mutex_lock(&me_m);
	bzero(&me, sizeof(bot_t));
	pthread_mutex_unlock(&me_m);
	pthread_mutex_destroy(&me_m);
}
