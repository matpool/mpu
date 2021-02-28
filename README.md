# MPU
A shim driver allows in-docker nvidia-smi showing correct process list.

# The problems
The NVIDIA driver is not aware of the PID namespace and nvidia-smi has no capability to map global pid to virtual pid, thus it shows nothing.
What's more, The NVIDIA driver is proprietary and we have no idea what's going on inside even small part of the Linux NVIDIA driver is open sourced.

# The solution steps
- figure out the basic mechanism of the NVIDIA driver with the open sourced part
- do some reverse engineering tests on the driver via GDB tools and several scripts (cuda/NVML)
- use our module to intercept syscalls and re-write fields of data strucuture with the knowledge of reverse engineering
- run the nvidia-smi with our module with several test cases

# The details
- nvidia-smi requests `0x20` ioctl command with `0xee4` flag to getting the global PID list (under `init_pid_ns`) **①**
- after getting non-empty PID list, it'll request `0x20` ioctl command with `0x1f48` flag with previous returned pids as input arguments to getting the process GPU memory consumptions **②**
- we hook the syscalls in system-wide approaching and intercept only NVIDIA device ioctl syscall (device major number is `195` and minor is `255` (control dev) which is defined in NVIDIA header file)
- check if request task is under any PID namespace, do nothing if it's global one (under `init_pid_ns`)
- if so, **①** convert the PID list from global to virtual
- however, **②** is a little more complicated which contains two-way interceptors--pre and post. 
  - on pre-stage, before invoking NVIDIA ioctl, the virtual PIDs (returned from **①**, converted) must convert back to global ones, since NVIDIA driver only recognize global PIDs. 
  - and one post-stage, after NVIDIA ioctl invoked, cast global PIDs back

---
![71614489144_ pic](https://user-images.githubusercontent.com/14119758/109408926-28831a00-79c9-11eb-8abf-a8382f5a897a.jpg)
---
![61614489023_ pic](https://user-images.githubusercontent.com/14119758/109408930-2d47ce00-79c9-11eb-8ec3-90f4324c6dd3.jpg)
---

# NOTE
only tested on _kernel 4.15.0-136_ , _docker 19.03.15_ , _NVIDIA driver 440.64_

---
Afterwords, we'd like to maintain the project with fully tested and more kernels and NVIDIA drivers supported. 
However we sincerely hope NVIDIA will fix this with simplicity and professionalism. Thx.
