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

#ifndef __MPU_SYSCALL_HOOK__
#define __MPU_SYSCALL_HOOK__

#include "mpu_drv.h"

// NV device major number
#define NV_MAJOR_DEVICE_NUMBER 195

/**
 * wrapping syscall ioctl into one structure
 * invoke @fn mpu_call_ioctl with such as param
 */
typedef struct mpu_ioctl_call_s
{
  asmlinkage long (*ioctl)(unsigned int fd, unsigned int cmd, unsigned long arg);
  unsigned int fd;
  unsigned int cmd;
  unsigned long arg;
} mpu_ioctl_call_t;

/**
 * starting hook syscall ioctl based on 
 * @param module hooked ioctl will invoke module's ioctl with @param ctx provided
 * @param ctx module's ioctl param
 * @return <0 means error
 * NOTE: there's no initialized check, thus we can't assure you what would happen call it twice or more
 */
int mpu_init_ioctl_hook(mpu_module_t *module, mpu_ctx_t *ctx);

/**
 * release initialized hook instance
 */
void mpu_exit_ioctl_hook(void);

static inline long mpu_call_ioctl(mpu_ioctl_call_t *c)
{
  return c->ioctl(c->fd, c->cmd, c->arg);
}

#endif // __MPU_SYSCALL_HOOK__