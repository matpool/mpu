# MPU Debug Configuration Guide

## Overview
The `print_pids` function is used to print debug information for process ID conversion. By default, this debug output is disabled and can be controlled through module parameters to enable or disable it.

## Configuration Method

### Runtime Parameter Control

Set debug parameters when loading the module:
```bash
# Load module and enable debug
sudo insmod mpu.ko debug_enabled=1

# Or if module is already loaded, dynamically modify
echo 1 | sudo tee /sys/module/mpu/parameters/debug_enabled

# Disable debug
echo 0 | sudo tee /sys/module/mpu/parameters/debug_enabled

# Check current status
cat /sys/module/mpu/parameters/debug_enabled
```

### Complete Operation Example

```bash
# 1. Confirm module is not loaded
lsmod | grep mpu

# 2. Load module and enable debug
sudo insmod mpu.ko debug_enabled=1

# 3. Confirm debug is enabled
cat /sys/module/mpu/parameters/debug_enabled
# Should output: Y

# 4. View debug logs
dmesg | grep "mpu:" | tail -10

# 5. If need to disable debug
echo 0 | sudo tee /sys/module/mpu/parameters/debug_enabled

# 6. Unload module
sudo rmmod mpu
```

## Log Viewing

Debug information is output to kernel logs, which can be viewed with the following commands:
```bash
# View kernel logs
dmesg | grep "mpu:"

# View logs in real-time
dmesg -w | grep "mpu:"

# View /var/log/kern.log
tail -f /var/log/kern.log | grep "mpu:"
```

## Log Format

The debug log format is:
```
mpu: [tag] dump process pid [pid_number]
```

Example:
```
mpu: before: u32 dump process pid 1234
mpu: after: u32 dump process pid 5678
```

## Important Notes

1. **Permission Requirements**: Root privileges are required to load/unload modules and modify parameters
2. **Module Path**: Ensure the `mpu.ko` file is in the current directory or specify the correct path
3. **Log Level**: Ensure kernel log level allows displaying `KERN_DEBUG` level information
4. **Real-time Effect**: Parameter changes take effect immediately without needing to reload the module
5. **Default Disabled**: Debug functionality is disabled by default to avoid unnecessary log output 