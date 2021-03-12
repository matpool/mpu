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

#include "mpu_drv.h"

#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "mpu_syscall_hook.h"
#include "mpu_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Magnus <Magnusbackyard@live.com>");
MODULE_DESCRIPTION("A shim driver allows in-docker nvidia-smi showing correct process list without modify anything");
MODULE_VERSION("0.1-pre");

#define MODULE_NAME "mpu"
#define DEVICE_NAME MODULE_NAME

/**
 * internal funcs
 */
static void mpu_drv_on_exit(void);
static long mpu_module_ioctl(mpu_ctx_t *ctx, struct mpu_ioctl_call_s *ioctl_c, dev_t rdev);
static mpu_ctx_t *mpu_init_ctx(void);
static void mpu_free_ctx(mpu_ctx_t *ctx);
/**
 * check the task pid-namespace pid is not equal to init namespace pid
 * @return
 *  for most container runtime, if current task is inside a container(under container pid-namespace), this function will return true
 */
static bool mpu_is_task_under_pid_ns(struct task_struct *tsk);

static mpu_ctx_t *mpu_ctx = NULL;
static mpu_module_t mpu_fops = {
    .ioctl = mpu_module_ioctl,
};

static mpu_ctx_t *mpu_init_ctx(void)
{
  mpu_ctx_t *ctx;
  ctx = kzalloc(sizeof(mpu_ctx_t), GFP_KERNEL);
  if (ctx == NULL)
    return NULL;

  ctx->hs = mpu_init_nv_handlers();
  if (ctx->hs == NULL)
  {
    kfree(ctx);
    return NULL;
  }

  return ctx;
}

static void mpu_free_ctx(mpu_ctx_t *ctx)
{
  if (ctx == NULL)
    return;

  mpu_free_nv_handlers(ctx->hs);
  kfree(ctx);
}

long mpu_module_ioctl(mpu_ctx_t *ctx, struct mpu_ioctl_call_s *ioctl_c, dev_t rdev)
{
  if (MAJOR(rdev) == NV_MAJOR_DEVICE_NUMBER)
  {
    // only works in container
    if (mpu_is_task_under_pid_ns(current))
    {
      // TODO:
    }
  }

  return mpu_call_ioctl(ioctl_c);
}

static bool mpu_is_task_under_pid_ns(struct task_struct *tsk)
{
  return task_pid_nr(tsk) != task_pid_vnr(tsk);
}

static void mpu_drv_on_exit(void)
{
  if (mpu_ctx)
  {
    mpu_free_ctx(mpu_ctx);
  }
}

static int __init mpu_drv_init(void)
{
  int ret;
  mpu_ctx = mpu_init_ctx();
  if (!mpu_ctx)
  {
    ret = -ENOMEM;
    goto error;
  }
  ret = mpu_init_ioctl_hook(&mpu_fops, mpu_ctx);
  if (ret < 0)
    goto error;

  printk(KERN_INFO "mpu: mpu driver initialized.\n");
  mpu_print_nv_handlers(mpu_ctx->hs);
  return 0;

error:
  mpu_drv_on_exit();
  return -1;
}

static void __exit mpu_drv_exit(void)
{
  mpu_exit_ioctl_hook();
  mpu_drv_on_exit();
}

module_init(mpu_drv_init);
module_exit(mpu_drv_exit);