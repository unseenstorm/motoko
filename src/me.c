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


void me_debug() {
	plugin_debug("me", "id:%u, x:%d, y:%d, gold:%u, ping:%d, hp:%d, mp:%d, area:%d, act:%d, ingame:%d, intown:%d, lskill:%d, rskill:%d\n",
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
				 me.intown,
				 me.lskill,
				 me.rskill);
}

void me_init() {
	pthread_mutex_init(&me_m, NULL);
	pthread_mutex_lock(&me_m);
	bzero(&me, sizeof(bot_t));
	me.intown = TRUE;
	me.rskill = -1;
	me.lskill = -1;
	pthread_mutex_unlock(&me_m);
}

void me_finit() {
	pthread_mutex_lock(&me_m);
	bzero(&me, sizeof(bot_t));
	pthread_mutex_unlock(&me_m);
	pthread_mutex_destroy(&me_m);
}


void me_leave() {
	d2gs_send(0x69, "");

	me_set_ingame(FALSE);

	plugin_print("me", "leaving game\n");
}


/* SETTERS */

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

SETTER(me, dword, gold)
SETTER(me, word, ping)
SETTER(me, word, hp)
SETTER(me, word, mp)
SETTER(me, byte, ingame)
SETTER(me, byte, intown)
SETTER(me, word, lskill)
SETTER(me, word, rskill)


/* CAST */

void me_cast_self(object_t *obj, dword arg) {
	(void)arg;
	d2gs_send(0x0c, "%w %w", obj->location.x, obj->location.y);

	/* msleep(module_setting("CastDelay")->i_var); */
	msleep(200 + me.ping);
}

void me_cast_right(object_t *obj, dword arg) {
	(void)arg;
	d2gs_send(0x0d, "01 00 00 00 %d", obj->id);

	/* msleep(module_setting("CastDelay")->i_var); */
	msleep(200 + me.ping);
}

void me_cast_left(object_t *obj, dword arg) {
	(void)arg;
	d2gs_send(0x06, "01 00 00 00 %d", obj->id);

	/* msleep(module_setting("CastDelay")->i_var); */
	msleep(200 + me.ping);
}

void me_swap_right(object_t *obj, dword arg) {
	(void)obj;
	if (me.rskill == arg) {
		return;
	}

	d2gs_send(0x3c, "%w 00 00 ff ff ff ff", arg & 0xFFFF);

	msleep(300);

	me_set_rskill(arg);
}

void me_swap_left(object_t *obj, dword arg) {
	(void)obj;
	if (me.lskill == arg) {
		return;
	}

	d2gs_send(0x3c, "%w 00 80 ff ff ff ff", arg & 0xFFFF);

	msleep(300);

	me_set_lskill(arg);
}


/* MOVE */

bool me_walk(int x, int y) {
	point_t p = {x, y};
	//pthread_mutex_lock(&game_exit_m);
	//pthread_cleanup_push((cleanup_handler) pthread_mutex_unlock, (void *) &game_exit_m);

	/*if (exited) {
		goto end;
	}*/
	int d = DISTANCE(me.obj.location, p);

	int delay = 2000; //ugly hack to wait me.object.location update
	while (d > MAX_WALKING_DISTANCE && delay) {
		msleep(100);
		delay -= 100;
		d = DISTANCE(me.obj.location, p);
	}

	if (d > MAX_WALKING_DISTANCE) {
		plugin_print("me", "got stuck trying to move to %i/%i (%i)\n", x, y, d);

		return FALSE;
	}


	/*
	 * sleeping can lead to a problem:
	 * bot's at 10000/10000, opens portal, chickens, moves to 5000/5000 -> long sleep
	 * C -> S 0x03 | C-> S 0x69 | msleep()
	 */


	//int t_s = t / 1000;
	//int t_ns = (t - (t_s * 1000)) * 1000 * 1000;

	//struct timespec ts;
	//clock_gettime(CLOCK_REALTIME, &ts);
	//ts.tv_sec += t_s;
	//ts.tv_nsec += t_ns;

	int t;
	int retry = 0;
	do {
		if (retry) {
			plugin_print("me", "moving to %i/%i (retry:%d)\n", (word) x, (word) y, retry);
			d2gs_send(0x4b, "00 00 00 00 %d", me.obj.id);
			msleep(100 + me.ping);
		} else {
			plugin_print("me", "moving to %i/%i\n", (word) x, (word) y);
		}

		d2gs_send(0x03, "%w %w", (word) x, (word) y);

		retry++;

		t = d * 50;
		/* plugin_debug("me", "sleeping for %ims\n", t); */

		msleep(t > 3000 ? 3000 : t);

		delay = 2000; //ugly hack to wait me.object.location update
		do {
			msleep(100);
			delay -= 100;
			d = DISTANCE(me.obj.location, p);
		} while (d > MIN_WALKING_DISTANCE && delay);
	} while (retry <= 3 && d > MIN_WALKING_DISTANCE);

	//pthread_cond_timedwait(&game_exit_cv, &game_exit_m, &ts);

	msleep(200 + me.ping);

	//end:
	//pthread_cleanup_pop(1);

	return d <= MIN_WALKING_DISTANCE;
}

bool me_teleport(int x, int y) {
	plugin_print("me", "teleporting to %i/%i\n", x, y);

	me_swap_right(&(me.obj), 0x36);

	d2gs_send(0x0c, "%w %w", x, y);

	/* msleep(module_setting("CastDelay")->i_var); */
	msleep(200 + me.ping);

	return TRUE;
}


static bool split_path(int x, int y, int min_range, int max_range, bool (*move_fun)(int, int)) {
	point_t p = {x, y};
	int d = DISTANCE(me.obj.location, p);

	if (d < min_range) {
		return TRUE;
	}

	if (d > max_range) {
		int n_nodes = d / max_range + 1;

		int inc_x = (x - me.obj.location.x) / n_nodes;
		int inc_y = (y - me.obj.location.y) / n_nodes;

		int target_x = me.obj.location.x;
		int target_y = me.obj.location.y;

		int i;
		for (i = 0; i <= n_nodes - 1; i++) {
			target_x += inc_x;
			target_y += inc_y;
			if (!move_fun(target_x, target_y)) {
				return FALSE;
			}
		}

		d = DISTANCE(me.obj.location, p);
	}

	if (d >= min_range && d <= max_range && !move_fun(x, y)) {
		return FALSE;
	}

	return split_path(x, y, min_range, max_range, move_fun);
}

static bool me_teleport_straight(int x, int y) {
	plugin_debug("me", "teleporting straight to %i/%i\n", x, y);

	return split_path(x, y, MIN_TELEPORT_DISTANCE, MAX_TELEPORT_DISTANCE, me_teleport);
}

static bool me_walk_straight(int x, int y) {
	plugin_debug("me", "walking straight to %i/%i\n", x, y);

	return split_path(x, y, MIN_WALKING_DISTANCE, MAX_WALKING_DISTANCE / 2, me_walk);
}

bool me_move(int x, int y) {
	return me.intown ? me_walk_straight(x, y) : me_teleport_straight(x, y);
}

/* TODO: use d2gs_pathing instead */
