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

#ifndef __MPU_DRV_H__
#define __MPU_DRV_H__

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>

struct mpu_ioctl_call_s;
struct mpu_nv_handlers_s;

typedef struct mpu_ctx_s
{
  // read-only
  struct mpu_nv_handlers_s *hs;
} mpu_ctx_t;

typedef struct mpu_module_s
{
  /**
     * @param ioctl_c do NOT retain it
     * @param rdev is requested nv rdev
     */
  long (*ioctl)(mpu_ctx_t *ctx, struct mpu_ioctl_call_s *ioctl_c, dev_t rdev);
} mpu_module_t;

#endif // __MPU_DRV_H__