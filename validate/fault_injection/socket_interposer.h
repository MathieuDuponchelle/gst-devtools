/* GStreamer
 *
 * Copyright (C) 2014 YouView TV Ltd
 *  Author: Mariusz Buras <mariusz.buras@youview.com>
 *
 * socket_interposer.h : overrides for standard socket functions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _SOCKET_INTERPOSER_H_
#define _SOCKET_INTERPOSER_H_

#include <sys/socket.h>
#include <netinet/ip.h>

/* Return 0 to remove the callback immediately */
typedef int (*socket_interposer_callback)(void*,const void*,size_t);

extern void socket_interposer_reset(void)  __attribute__((visibility("default")));
extern int socket_interposer_set_callback(struct sockaddr_in*, socket_interposer_callback,void*)  __attribute__((visibility("default")));

#endif /* _SOCKET_INTERPOSER_H_ */
