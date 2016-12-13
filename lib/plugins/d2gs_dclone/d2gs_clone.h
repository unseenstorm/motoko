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

#ifndef D2GS_DCLONE_H_
#define D2GS_DCLONE_H_

#include <me.h>

typedef void (*pthread_cleanup_handler_t)(void *);

typedef struct {
	char *s_addr;
	int hits;
} hot_addr;


#endif /* D2GS_DCLONE_H_ */
