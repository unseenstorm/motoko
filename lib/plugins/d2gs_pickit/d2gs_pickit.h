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

#ifndef D2GS_PICKIT_H_
#define D2GS_PICKIT_H_


#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <module.h>

#include <d2gs.h>
#include <internal.h>
#include <moduleman.h>
#include <packet.h>
#include <settings.h>
#include <gui.h>
#include <data/item_codes.h>

#include <util/config.h>
#include <util/net.h>
#include <util/list.h>
#include <util/string.h>
#include <util/system.h>
#include <util/types.h>

#include <me.h>

#define UNSPECIFIED 0xff

typedef void (*pthread_cleanup_handler_t)(void *);

typedef struct {
	byte action;
	byte destination;
	byte container;
	byte quality;
	char code[4];
	point_t location;
	byte level;
	dword id;
	int ethereal;
	dword amount;
	int sockets;
	int distance; // test pickit
} item_t;



#endif /* D2GS_PICKIT_H_ */
