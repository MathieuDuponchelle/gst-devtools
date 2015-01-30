/* GStreamer
 *
 * Copyright (C) 2014 YouView TV Ltd
 *  Author: Mariusz Buras <mariusz.buras@youview.com>
 *
 * socket_interposer.c : overrides for standard socket functions
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

#define _GNU_SOURCE

#include "socket_interposer.h"

#include <sys/socket.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#define MAX_CALLBACKS (16)

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct
{
  socket_interposer_callback callback;
  void *userdata;
  struct sockaddr_in sockaddr;
  int fd;
} callbacks[MAX_CALLBACKS];

void
socket_interposer_reset (void)
{
  pthread_mutex_lock (&mutex);
  memset (&callbacks, 0, sizeof (callbacks));
  pthread_mutex_unlock (&mutex);
}

int
socket_interposer_remove_callback_unlocked (struct sockaddr_in *addrin,
    socket_interposer_callback callback, void *userdata)
{
  size_t i;
  for (i = 0; i < MAX_CALLBACKS; i++) {
    if (callbacks[i].callback == callback
        && callbacks[i].userdata == userdata
        && callbacks[i].sockaddr.sin_addr.s_addr == addrin->sin_addr.s_addr
        && callbacks[i].sockaddr.sin_port == addrin->sin_port) {
      memset (&callbacks[i], 0, sizeof (callbacks[0]));
      return 1;
    }
  }
  return 0;
}

int
socket_interposer_set_callback (struct sockaddr_in *addrin,
    socket_interposer_callback callback, void *userdata)
{
  size_t i;
  pthread_mutex_lock (&mutex);


  socket_interposer_remove_callback_unlocked (addrin, callback, userdata);
  for (i = 0; i < MAX_CALLBACKS; i++) {
    if (callbacks[i].callback == NULL) {
      callbacks[i].callback = callback;
      callbacks[i].userdata = userdata;
      memcpy (&callbacks[i].sockaddr, addrin, sizeof (struct sockaddr_in));
      callbacks[i].fd = -1;
      break;
    }
  }
  pthread_mutex_unlock (&mutex);
}

int connect (int, const struct sockaddr_in *, socklen_t)
    __attribute__ ((visibility ("default")));

int
connect (int socket, const struct sockaddr_in *addrin, socklen_t address_len)
{
  size_t i;
  int override_errno = 0;
  typedef ssize_t (*real_connect_fn) (int, const struct sockaddr_in *,
      socklen_t);

  pthread_mutex_lock (&mutex);

  for (i = 0; i < MAX_CALLBACKS; i++) {
    if (callbacks[i].sockaddr.sin_addr.s_addr == addrin->sin_addr.s_addr
        && callbacks[i].sockaddr.sin_port == addrin->sin_port) {

      callbacks[i].fd = socket;

      if (callbacks[i].callback) {
        int ret = callbacks[i].callback (callbacks[i].userdata, NULL,
            0);
        if (ret != 0)
          override_errno = ret;
        else                    /* Remove the callback */
          memset (&callbacks[i], 0, sizeof (callbacks[0]));
      }

      break;
    }
  }

  pthread_mutex_unlock (&mutex);

  static real_connect_fn real_connect = 0;

  if (!real_connect) {
    real_connect = (real_connect_fn) dlsym (RTLD_NEXT, "connect");
  }

  ssize_t ret = 0;
  if (!override_errno) {
    ret = real_connect (socket, addrin, address_len);
  } else {
    // override errno
    errno = override_errno;
    ret = -1;
  }
  return ret;
}

ssize_t send (int socket, const void *buffer, size_t len, int flags)
    __attribute__ ((visibility ("default")));
ssize_t
send (int socket, const void *buffer, size_t len, int flags)
{
  size_t i;
  int override_errno = 0;
  typedef ssize_t (*real_send_fn) (int, const void *, size_t, int);

  static real_send_fn real_send = 0;

  pthread_mutex_lock (&mutex);
  for (i = 0; i < MAX_CALLBACKS; i++) {
    if (callbacks[i].fd != 0 && callbacks[i].fd == socket) {
      int ret = callbacks[i].callback (callbacks[i].userdata, buffer,
          len);

      if (ret != 0)
        override_errno = ret;
      else                      /* Remove the callback */
        memset (&callbacks[i], 0, sizeof (callbacks[0]));

      break;
    }
  }
  pthread_mutex_unlock (&mutex);

  if (!real_send) {
    real_send = (real_send_fn) dlsym (RTLD_NEXT, "send");
  }

  ssize_t ret = real_send (socket, buffer, len, flags);

  // override errno
  if (override_errno != 0) {
    errno = override_errno;
    ret = -1;
  }

  return ret;

}

ssize_t recv (int socket, void *buffer, size_t length, int flags)
    __attribute__ ((visibility ("default")));
ssize_t
recv (int socket, void *buffer, size_t length, int flags)
{
  size_t i;
  int old_errno;
  typedef ssize_t (*real_recv_fn) (int, void *, size_t, int);

  static real_recv_fn real_recv = 0;

  if (!real_recv) {
    real_recv = (real_recv_fn) dlsym (RTLD_NEXT, "recv");
  }

  ssize_t ret = real_recv (socket, buffer, length, flags);
  old_errno = errno;

  pthread_mutex_lock (&mutex);
  for (i = 0; i < MAX_CALLBACKS; i++) {
    if (callbacks[i].fd != 0 && callbacks[i].fd == socket) {
      int newerrno = callbacks[i].callback (callbacks[i].userdata, buffer,
          ret);

      // override errno
      if (newerrno != 0) {
        old_errno = newerrno;
        ret = -1;
      } else {                  /* Remove the callback */
        memset (&callbacks[i], 0, sizeof (callbacks[0]));
      }

      break;
    }
  }
  pthread_mutex_unlock (&mutex);

  errno = old_errno;

  return ret;
}
