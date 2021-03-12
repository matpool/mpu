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

#include "mpu_ioctl.h"

#include <linux/slab.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>
#include <linux/uaccess.h>

#include "mpu_nv.h"
#include "mpu_syscall_hook.h"

#define MAX_HANDLER_SLOTS 4

struct mpu_nv_handlers_s
{
  int len;
  mpu_nv_handler_t *vals;
};

static long nv_handle_dev_query(struct mpu_ioctl_call_s *ioctl_c, dev_t rdev, pid_t pid);

static mpu_nv_handler_t handlers[] = {
    {NV_IOCTL_DEV_QUERY, nv_handle_dev_query},
};

mpu_nv_handler_t *mpu_find_nv_handler(struct mpu_nv_handlers_s *hs, int cmd)
{
  int i = 0;
  for (; i < hs->len; i++)
  {
    if (hs->vals[i].cmd == cmd)
    {
      return &hs->vals[i];
    }
  }
  return ERR_PTR(-ENOENT);
}

struct mpu_nv_handlers_s *mpu_init_nv_handlers(void)
{
  struct mpu_nv_handlers_s *hs = kmalloc(sizeof(struct mpu_nv_handlers_s), GFP_KERNEL);
  if (hs == NULL)
  {
    return NULL;
  }

  hs->vals = handlers;
  hs->len = sizeof(handlers) / sizeof(handlers[0]);
  return hs;
}

void mpu_free_nv_handlers(struct mpu_nv_handlers_s *hs)
{
  if (hs)
    kfree(hs);
}

void mpu_print_nv_handlers(struct mpu_nv_handlers_s *hs)
{
  printk(KERN_INFO "mpu: %d nv handlers registered.\n", hs->len);
}

/**
 * in-place update pids from global pids to current pid-ns pids
 * rcu read protected
 * @param pids len(pids) equals to off*cnt
 */
static void cast_vnr_pids(u32 *pids, u32 off, u32 cnt)
{
  u32 i;
  pid_t pid;
  struct pid *found = NULL;
  struct pid_namespace *current_ns;

  rcu_read_lock();
  current_ns = task_active_pid_ns(current);
  for (i = 0; i < cnt; i++, pids += off)
  {
    found = find_pid_ns(*pids, &init_pid_ns);
    if (found)
    {
      pid = pid_nr_ns(found, current_ns);
      *pids = (u32)pid;
    }
  }
  rcu_read_unlock();
}

/**
 * in-place update pids from current pid-ns pids to global pids
 * rcu read protected
 * @param pids len(pids) equals to off*cnt
 */
static void cast_nr_pids(u32 *pids, u32 off, u32 cnt)
{
  u32 i;
  pid_t pid;
  struct pid *found = NULL;
  struct pid_namespace *current_ns;

  rcu_read_lock();
  current_ns = task_active_pid_ns(current);
  for (i = 0; i < cnt; i++, pids += off)
  {
    if (*pids != 0)
    {
      if ((found = find_pid_ns(*pids, current_ns)) != NULL)
      {
        pid = pid_nr_ns(found, &init_pid_ns);
        *pids = (u32)pid;
      }
    }
  }
  rcu_read_unlock();
}

static void print_pids(u32 *pids, u32 off, u32 cnt, const char *tag)
{
  u32 i;
  for (i = 0; i < cnt; i++, pids += off)
  {
    printk(KERN_DEBUG "mpu: %s dump process pid %u\n", tag, *pids);
  }
}

#define MPU_NV_CAST_PIDS_IMPL(ptr, list_type, item_type, cast)                     \
  {                                                                                \
    long ret = 0;                                                                  \
    list_type pl;                                                                  \
    u32 *pids = NULL; /* temp buffer for saving global pids or ns pids */          \
    size_t len;        /* total items length in bytes */                            \
    size_t elen;                                                                    \
                                                                                   \
    if (copy_from_user(&pl, ptr, sizeof(list_type)))                               \
    {                                                                              \
      printk(KERN_ERR "mpu: read NVML userspace type " #list_type " failed\n");    \
      ret = -EFAULT;                                                               \
      goto done;                                                                   \
    }                                                                              \
                                                                                   \
    if (pl.cnt > 0)                                                                \
    {                                                                              \
      len = pl.cnt * sizeof(item_type);                                            \
      elen = sizeof(item_type) / sizeof(u32);                                      \
      pids = kmalloc(len, GFP_KERNEL);                                             \
      if (pids == NULL)                                                            \
      {                                                                            \
        ret = -ENOMEM;                                                             \
        goto done;                                                                 \
      }                                                                            \
                                                                                   \
      if (copy_from_user(pids, ptr + offsetof(list_type, pl), len))                \
      {                                                                            \
        ret = -EFAULT;                                                             \
        goto done;                                                                 \
      }                                                                            \
                                                                                   \
      print_pids(pids, elen, pl.cnt, "before: " #item_type);                       \
      cast(pids, elen, pl.cnt);                                                    \
      print_pids(pids, 1, pl.cnt, "after: " #item_type);                           \
                                                                                   \
      if (copy_to_user(qm->u_ptr + offsetof(list_type, pl), pids, len))            \
      {                                                                            \
        printk(KERN_ERR "mpu: write NVML userspace type " #list_type " failed\n"); \
        ret = -EFAULT;                                                             \
      }                                                                            \
    }                                                                              \
  done:                                                                            \
    if (pids)                                                                      \
      kfree(pids);                                                                 \
    return ret;                                                                    \
  }

static long nv_handle_query_nvml_processes(mpu_dev_query_t *qm)
  MPU_NV_CAST_PIDS_IMPL(qm->u_ptr, mpu_nvml_process_list_t, u32, cast_vnr_pids)

static long nv_handle_query_nvml_process_mem_pre(mpu_dev_query_t *qm)
  MPU_NV_CAST_PIDS_IMPL(qm->u_ptr, mpu_nvml_process_mem_list_t, mpu_nvml_process_mem_item_t, cast_nr_pids)

static long nv_handle_query_nvml_process_mem_post(mpu_dev_query_t *qm)
  MPU_NV_CAST_PIDS_IMPL(qm->u_ptr, mpu_nvml_process_mem_list_t, mpu_nvml_process_mem_item_t, cast_vnr_pids)

static long nv_handle_dev_query(struct mpu_ioctl_call_s *ioctl_c, dev_t rdev, pid_t pid)
{
  size_t arg_size = _IOC_SIZE(ioctl_c->cmd);
  mpu_dev_query_t *arg_copy = NULL;
  void *arg_ptr = (void *)ioctl_c->arg;
  long ret = 0;

  if (arg_size != sizeof(mpu_dev_query_t))
  {
    return mpu_call_ioctl(ioctl_c);
  }

  arg_copy = (mpu_dev_query_t *)kmalloc(arg_size, GFP_KERNEL);
  if (arg_copy == NULL)
  {
    ret = -ENOMEM;
    goto done;
  }
  else if (copy_from_user(arg_copy, arg_ptr, arg_size))
  {
    ret = -EINVAL;
    goto done;
  }

  if (arg_copy->tag == 0x1f48)
  {
    ret = nv_handle_query_nvml_process_mem_pre(arg_copy);
    if (ret < 0)
      goto done;
  }

  ret = mpu_call_ioctl(ioctl_c);
  if (ret < 0)
    goto done;

  switch (arg_copy->tag)
  {
  case 0xee4:
    ret = nv_handle_query_nvml_processes(arg_copy);
    break;
  case 0x1f48:
    ret = nv_handle_query_nvml_process_mem_post(arg_copy);
    break;
  }

done:
  if (ret < 0)
    printk(KERN_ERR "mpu: mpu_dev_query_t failed with an error 0x%lx\n", -ret);
  if (arg_copy)
    kfree(arg_copy);

  return ret;
}
