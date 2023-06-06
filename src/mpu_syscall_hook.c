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

#include "mpu_syscall_hook.h"

#include <linux/syscalls.h>
#include <linux/cgroup.h>
#include <asm/paravirt.h>
#include <linux/slab.h>
#include <linux/file.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0))
#define FILE_INODE(f) (f->f_inode)
#else
#define FILE_INODE(f) (f->f_path.dentry->d_inode)
#endif // FILE_INODE

// kernel 4.17 introduces syscall wrapper
#ifdef CONFIG_ARCH_HAS_SYSCALL_WRAPPER
typedef asmlinkage long (*ioctl_fn)(const struct pt_regs *);

typedef struct mpu_ioctl_private_s
{
  mpu_ioctl_call_t c;
  ioctl_fn ioctl;
  const struct pt_regs *regs;
} mpu_ioctl_private_t;

static asmlinkage long mpu_hooked_ioctl(const struct pt_regs *regs);
#else
typedef asmlinkage long (*ioctl_fn)(unsigned int fd, unsigned int cmd, unsigned long arg);

typedef struct mpu_ioctl_private_s
{
  mpu_ioctl_call_t c;
  ioctl_fn ioctl;
} mpu_ioctl_private_t;

static asmlinkage long mpu_hooked_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);
#endif // CONFIG_ARCH_HAS_SYSCALL_WRAPPER

// all its fields are immutable without ownership
typedef struct mpu_syscall_hook_s
{
  mpu_module_t *module;
  mpu_ctx_t *ctx;
  unsigned long **syscall_tbl;
  ioctl_fn ioctl;
} mpu_syscall_hook_t;

static mpu_syscall_hook_t mpu_hook_instance;

static dev_t get_rdev(unsigned int fd)
{
  struct fd f = fdget(fd);
  struct inode *inode;
  dev_t rdev;

  if (f.file)
  {
    inode = FILE_INODE(f.file);
    if (inode)
    {
      rdev = inode->i_rdev;
    }
    fdput(f);
  }
  return rdev;
}

static void write_syscall(unsigned long **syscall_tbl, ioctl_fn sys_ioctl)
{
  unsigned long local_cr0;

  local_cr0 = read_cr0();
  write_cr0(local_cr0 & ~0x00010000);
  syscall_tbl[__NR_ioctl] = (unsigned long *)sys_ioctl;
  write_cr0(local_cr0);
}

int mpu_init_ioctl_hook(mpu_module_t *module, mpu_ctx_t *ctx)
{
  unsigned long **syscall_tbl;
  ioctl_fn sys_ioctl;

#ifdef KPROBE_LOOKUP
  typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
  kallsyms_lookup_name_t kallsyms_lookup_name;
  register_kprobe(&kp);
  kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
  unregister_kprobe(&kp);
#endif

  if (!module || !module->ioctl || !ctx)
  {
    return -EINVAL;
  }

  syscall_tbl = (unsigned long **)(kallsyms_lookup_name("sys_call_table"));
  if (!syscall_tbl)
  {
    return -ENXIO;
  }
  sys_ioctl = (ioctl_fn)syscall_tbl[__NR_ioctl];

  mpu_hook_instance.module = module;
  mpu_hook_instance.ctx = ctx;
  mpu_hook_instance.syscall_tbl = syscall_tbl;
  mpu_hook_instance.ioctl = sys_ioctl;

  // prevent any re-ordering causing accessing null mpu_hook_instance when hook triggered
  barrier();
  write_syscall(syscall_tbl, mpu_hooked_ioctl);

  return 0;
}

// if module is un-loaded but still retain hooked ioctl address
// the kernel will be panic
void mpu_exit_ioctl_hook(void)
{
  if (mpu_hook_instance.syscall_tbl && mpu_hook_instance.ioctl)
  {
    write_syscall(mpu_hook_instance.syscall_tbl, mpu_hook_instance.ioctl);
  }
}

#ifdef CONFIG_ARCH_HAS_SYSCALL_WRAPPER
static asmlinkage long mpu_hooked_ioctl(const struct pt_regs *regs)
{
  mpu_ioctl_private_t pc = {
      .c = {
          .fd = (unsigned int)regs->di,
          .cmd = (unsigned int)regs->si,
          .arg = (unsigned long)regs->dx,
      },
      .ioctl = mpu_hook_instance.ioctl,
      .regs = regs,
  };
  dev_t dev = get_rdev(pc.c.fd);
  return mpu_hook_instance.module->ioctl(mpu_hook_instance.ctx, &pc.c, dev);
}

long mpu_call_ioctl(mpu_ioctl_call_t *c)
{
  mpu_ioctl_private_t *pc = container_of(c, mpu_ioctl_private_t, c);
  return pc->ioctl(pc->regs);
}
#else
static asmlinkage long mpu_hooked_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
  mpu_ioctl_private_t pc = {
      .c = {
          .fd = fd,
          .cmd = cmd,
          .arg = arg,
      },
      .ioctl = mpu_hook_instance.ioctl,
  };
  dev_t dev = get_rdev(fd);
  return mpu_hook_instance.module->ioctl(mpu_hook_instance.ctx, &pc.c, dev);
}

long mpu_call_ioctl(mpu_ioctl_call_t *c)
{
  mpu_ioctl_private_t *pc = container_of(c, mpu_ioctl_private_t, c);
  return pc->ioctl(c->fd, c->cmd, c->arg);
}
#endif // CONFIG_ARCH_HAS_SYSCALL_WRAPPER