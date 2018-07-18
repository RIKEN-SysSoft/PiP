# PiP

Process-in-Process

## Description

PiP is a user-level library which allows a process to create sub-processes into the same virtual address space where the parent process runs. The parent process and sub-processes share the same address space, however, each process has its own variables. So, each process runs independently from the other process. If some or all processes agreed, then data own by a process can be accessed by the other processes.

Those processes share the same address space, just like pthreads, and each process has its own variables like a process. The parent process is called PiP process and sub-processes are called PiP task since it has the best of the both worlds of processes and pthreads.

## Installation

see the [INSTALL](INSTALL) file.

## License

This project is licensed under the 2-clause simplified BSD License - see the [LICENSE](LICENSE) file for details.

## Related Software

* [PiP-glibc](https://github.com/RIKEN-SysSoft/PiP-glibc) - patchd GNU libc for PiP
* [PiP-gdb](https://github.com/RIKEN-SysSoft/PiP-gdb) - patchd gdb to debug PiP root and PiP tasks.
