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

#ifndef __MPU_NV_H__
#define __MPU_NV_H__

// This file declares nv types

#include <linux/types.h>

#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64)
#define MPU_64_BITS
#endif

#if !defined(MPU_64_BITS)
#error("mpu-drv: must be compiled with 64-bits")
#endif // MPU_64_BITS

#define ALIGN_BYTES(size) __attribute__ ((aligned (size)))

///
// ioctl commands
///
#define NV_IOCTL_DEV_QUERY 0x2a

///
// below types are the work of reverse engineering based on ioctl syscall
// if you haven't got what it's meaning, type the variable with '_' prefix and end it with sequential numbers.
// to avoid incorrect memory access, if the pointer is pointing to an UVA, you must start the variable with 'u_'
///

// for driver device query
typedef struct mpu_dev_query {
    u32   _0;
    u32   _1;
    u32   _2;
    void  * u_ptr  ALIGN_BYTES(8); // internal pointer
    u32   tag;
} mpu_dev_query_t;

typedef struct mpu_nvml_process_list {
    u32 _0;
    u32 _1;
    u32 cnt;
    u32 pl[];
} mpu_nvml_process_list_t;

// ver. 440
typedef struct mpu_nvml_process_mem_item_1f48 {
    u32 pid;
    u32 _0[9];
} mpu_nvml_process_mem_item_1f48_t;

typedef struct mpu_nvml_process_mem_list_1f48 {
    u32 cnt;
    u32 _0;
    mpu_nvml_process_mem_item_1f48_t pl[];
} mpu_nvml_process_mem_list_1f48_t;

// ver. 460
typedef struct mpu_nvml_process_mem_item_2588 {
    u32 pid;
    u32 _0[11];
} mpu_nvml_process_mem_item_2588_t;

typedef struct mpu_nvml_process_mem_list_2588 {
    u32 cnt;
    u32 _0;
    mpu_nvml_process_mem_item_2588_t pl[];
} mpu_nvml_process_mem_list_2588_t;

// ver. 530
typedef struct mpu_nvml_process_mem_item_3848 {
    u32 pid;
    u32 _0[17];
} mpu_nvml_process_mem_item_3848_t;

typedef struct mpu_nvml_process_mem_list_3848 {
    u32 cnt;
    u32 _0;
    mpu_nvml_process_mem_item_3848_t pl[];
} mpu_nvml_process_mem_list_3848_t;

#endif // __MPU_NV_H__
