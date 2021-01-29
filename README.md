# Vangen
Vangen is a simple, machine-agnostic, deterministic  crash reproducing utility for [ir0nc0w](https://github.com/ir0nc0w)/[hypervisor-fuzz](https://github.com/ir0nc0w/hypervisor-fuzz) (_currently private_).
> Vangen enables you to reproduce crash on **van**illa linux by providing kernel module **gen**erated from test case.

## Prerequisites
- test case log generated by hyfuzz
- vanilla linux running on hypervisor

## Installation
```
$ git clone https://github.com/cyanide17/vangen.git
$ cd vangen
```
1. In `script.sh`,   

- set `PRJ_PATH` to your hyfuzz directory. *(line 2)*   
it is set to `~/Desktop/hypervisor-fuzz` as default.   

- set `PORT` to port number you opened. *(line 3)*   
it is set to `2345` as default.

2. From `System.map` which is located at `$PRJ_PATH/build/linux-...`,   
find `pnp_global` and copy the address value.

- In `src/template.c`, set value of `list_pnp_head` to copied value.
- This job is required to get list_head of ISA devices for ISA device driver unregistering.
    - I feel sorry for asking you to do this bothersome job :<


## Usage
With linux running on hypervisor such as QEMU,
You can generate kernel module from log file and send it to linux with this command.
```
$ ./script.sh 'relative-path-to-log-file'"
```
Or you can try this for debugging.
```
$ ./script.sh 'relative-path-to-log-file' -dbg"
```
This leaves debris from intermediate processes.   
   
When kernel module is successfully sent to linux, insert module into your linux.
```
# in linux virtual machine shell
$ insmod mod.ko
```
Then reproduction procedure will start right after insertion.


***
## Design Overview
Entire process can be break down into 2 statges.
- before file transfer
- after file transfer

### before file transfer
after `$ ./script ...`
1. **parsing**
    - parse test case log file into corresponding C code.
    - Some proper preprocessing and postprocessing codes can be included. *(ex.allocating & freeing DMA zone)*
2. **merging**
    - merge C code with provided kernel module template.
3. **compiling**
    - compile the merged file to get `mod.ko` module file.
4. **file transfer** to linux virtual machine!

### after file transfer
after `$ insmod mod.ko`
1. **address space probing**
    - probe virtual PCI devices' PCI configuration space to get BAR addresses for I/O operation
2. **noise reduction**
    - disable hardware interrupts from any devices
    - unregister all device registers (PCI & ISA)
3. **I/O operation**
    - if DMA operations exist, allocate all memory required first.
    - execute parsed C code.
    - if allocation occurred, free them.
    
    
***
## Future Works
- get ISA list_head through some linux kernel API, so that no manual job is required.
- test case minimization.
