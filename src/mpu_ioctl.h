//  MPU, A shim driver allows in-docker nvidia-smi showing correct process list without modify anything
//  Copyright (C) 2021, Matpool
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#ifndef __MPU_IOCTL_H__
#define __MPU_IOCTL_H__

#include "mpu_drv.h"

typedef struct mpu_nv_handler_s
{
  int cmd;
  /**
   * @param pid request process global pid
   */
  long (*handle)(struct mpu_ioctl_call_s *ioctl_c, dev_t rdev, pid_t pid);
} mpu_nv_handler_t;

/**
 * @param hs handlers
 * @param cmd incoming request command
 * @return valid handler or ERR_PTR
 * and you should not retain the ptr returned from mpu_find_nv_handler
 */
extern mpu_nv_handler_t *mpu_find_nv_handler(struct mpu_nv_handlers_s *hs, int cmd);

/**
 * @return valid handlers or null when out of memory
 */
extern struct mpu_nv_handlers_s *mpu_init_nv_handlers(void);
extern void mpu_free_nv_handlers(struct mpu_nv_handlers_s *hs);

extern void mpu_print_nv_handlers(struct mpu_nv_handlers_s *hs);

#endif // __MPU_IOCTL_H__