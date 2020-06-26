# PiP [![Build Status](https://travis-ci.org/RIKEN-SysSoft/PiP.svg?branch=pip-1)](https://travis-ci.org/RIKEN-SysSoft/PiP)

Process-in-Process

## Description

Process-in-Process (PiP) is a user-level library to have the best of
the both worlds of multi-process and multi-thread parallel execution
models. PiP allows a process to create sub-processes into the same
virtual address space where the parent process runs. So the parent
process and sub-processes share the same address space, and every
process is able to access the data owned by the other
processes. Unlike multi-thread model, each process has its own
variable set and each process runs independently from the other
process. The parent process is called PiP process and a sub-process
are called a PiP task. PiP is implemented as a user-level library
running on Linux. There is no need to have dedicated OS kernel,
patched OS kernel, nor the dedicated language system.

## License

This project is licensed under the 2-clause simplified BSD License -
see the [LICENSE](LICENSE) file for details.

## Installation

### Software Packages

* [PiP-glibc](https://github.com/RIKEN-SysSoft/PiP-glibc) - patched GNU libc for PiP
* [PiP](https://github.com/RIKEN-SysSoft/PiP) - Process in Process (this package)
* [PiP-gdb](https://github.com/RIKEN-SysSoft/PiP-gdb) - patched gdb to debug PiP root and PiP tasks.
<!-- * [MPICH over PIP
 environment](https://github.com/pmodels/mpich-pip/wiki) - patched
 MPICH for PiP -->

Before installing PiP, we strongly recommend you to install PiP-glibc.

After installing PiP, PiP-gdb can be installed too.

### Installation from the source code.

* PiP-glibc, PiP and PiP-gdb - see the [INSTALL.md](INSTALL.md) file
* MPICH over PIP environment - see the [mpich-pip official wiki](https://github.com/pmodels/mpich-pip/wiki)

### Installation from the binary packages

RPM packages and their yum repository are also available for CentOS 7 / RHEL7.

    $ sudo rpm -Uvh https://git.sys.r-ccs.riken.jp/PiP/package/el/7/noarch/pip-1/pip-release-1-0.noarch.rpm
    $ sudo yum install pip-glibc
    $ sudo yum install pip pip-debuginfo
    $ sudo yum install pip-gdb

### Installation test

A number of test programs can be found in the "test" directory
of the source distribution of PiP.  You can run them by "make check"
at the top of the source directory after "make install".

## Getting Started

### To compile and link your PiP programs

* PiP root process (spawning PiP tasks)
    must be linked with the PiP library and must specify the link
    option as follows if you have the patched GLIBC,

        --dynamic-linker=<GLIBC_INSTALL_DIR>/lib/ld-2.17.so

    Once you specify this option, PiP root process uses the patched
    GLIBC libraries, no matter how LD_LIBRARY_PATH is
    specified. The PiP root process is not required to be PIE.
    Note that the other SOLIBs are already symbolic linked into
    the <GLIBC_INSTALL_DIR>/lib directory by the "piplnlibs" command.
    The piplnlibs command is automatically invoked by the RPM pakcage
    installation, or should be manually invoked in the case of
    source installation.
    (In case of the RPM binary distribution, <GLIBC_INSTALL_DIR> is "/opt/pip")

* PiP task (spawned by PiP root process)
    must be compiled with "-fPIE -pthread", must be linked with "-pie
    -rdynamic -pthread" options. PiP task programs are not required to be
    linked with the PiP library. Thus programs to be ran as PiP tasks
    are not required to modify their source code. Since PiP root and
    PiP task(s) share the same (virtual) address space and ld-linux.so
    is already loaded by PiP root, PiP tasks use the patched GLIBC.

* pipcc(1) command

    You can use pipcc(1) command to compile and link your PiP programs.
    e.g.

        $ pipcc -Wall -O2 -g -c pipmodule.c
        $ pipcc -Wall -O2 -g -o pipprog pipprog.c

### To run your PiP programs

* Running PiP programs
    Consult [EXECMODE](EXECMODE) file located at the same directory with
    this file for details.

* How to check if PiP programs run under PiP environment
    check if they are shared the same address space by the following
    command,

        $ cat /proc/<PID>/maps

    Here, <PID> is the pid of the root process or one of the PiP
    tasks.

### To debug your PiP programs by the pip-gdb command

The following procedure attaches all PiP tasks, which are created
by same PiP root task, as GDB inferiors.

    $ pip-gdb
    (gdb) set pip-auto-attach on
    (gdb) attach <PID-of-your-PiP-program>

The attached inferiors can be seen by the following GDB command:

    (gdb) info inferiors
      Num  Description              Executable
      4    process 6453 (pip 2)     /somewhere/pip-task-2
      3    process 6452 (pip 1)     /somewhere/pip-task-1
      2    process 6451 (pip 0)     /somewhere/pip-task-0
    * 1    process 6450 (pip root)  /somewhere/pip-root

You can select and debug an inferior by the following GDB command:

    (gdb) inferior 2
    [Switching to inferior 2 [process 6451 (pip 0)] (/somewhere/pip-task-0)]

When an already-attached program calls pip_spawn() and becomes
a PiP root task, the newly created PiP child tasks aren't attached
automatically, but you can add empty inferiors and then attach
the PiP child tasks to the inferiors.
e.g.

    .... type Control-Z to stop the root task.
    ^Z
    Program received signal SIGTSTP, Stopped (user).

    (gdb) add-inferior
    Added inferior 2
    (gdb) inferior 2
    (gdb) attach 1902

    (gdb) add-inferior
    Added inferior 3
    (gdb) inferior 3
    (gdb) attach 1903

    (gdb) add-inferior
    Added inferior 4
    (gdb) inferior 4
    (gdb) attach 1904

    (gdb) info inferiors
      Num  Description              Executable
    * 4    process 1904 (pip 2)     /somewhere/pip-task-2
      3    process 1903 (pip 1)     /somewhere/pip-task-1
      2    process 1902 (pip 0)     /somewhere/pip-task-0
      1    process 1897 (pip root)  /somewhere/pip-root

You can write the following setting in your $HOME/.gdbinit file:

    set pip-auto-attach on

In that case, you can attach all relevant PiP tasks by:

    $ pip-gdb -p <PID-of-your-PiP-program>

### Docker image

    PiP, PiP-glibc and PiP-gdb can also be installed by using a Docker
    image.

	$ docker pull rikenpip/pip-v1
	$ sudo docker run -it rikenpip/pip-v1 /bin/bash

## Documents

### FAQ

* Does MPI with PiP exist?
    Currently, we are working with ANL to develop MPICH using PiP.
    This repository, located at ANL, is not yet open to public
    at the time of writing.

* After installation, any commands aborted with SIGSEGV
    This can happen when you specify LD_PRELOAD to include the installed
    PiP library. The LD_PRELOAD environment must be specified only when
    running PiP program with the "process:preload" running mode.
    Please consult the [EXECMODE](EXECMODE) file at the same directory
    with this file.

### Man pages

Man pages will be installed at <PIP_INSTALL_DIR>/share/man.

    $ man -M <PIP_INSTALL_DIR>/share/man libpip

### HTML

HTML documents will be installed at <PIP_INSTALL_DIR>/doc/pip.

(In case of the RPM binary distribution, <PIP_INSTALL_DIR> is "/usr")

## Reference

A. Hori, M. Si, B. Gerofi, M. Takagi, J. Dayal, P. Balaji, and Y. Ishikawa. "Process-in-process: techniques for practical address-space sharing," In Proceedings of the 27th International Symposium on High-Performance Parallel and Distributed Computing (HPDC '18). ACM, New York, NY, USA, 131-143. DOI: https://doi.org/10.1145/3208040.3208045

## Presentation Slides

* [HPDC'18](presentation/HPDC18-PiP.key.pdf)
* [ROSS'18](presentation/Ross-2018-PiP.key.pdf)
* [IPDPS/RADR'20](presentation/IPDPS20-RADRws.key.pdf)

## Coming soon ... (no later than the end of 2020)

Newer PiP versions will be released as branches of this repo. The
current one (v1) will be deprecated. There will be no ABI
comopatibility between any of those versions.

### PiP v2 will be the stable version of v1 with some new features.

### PiP v3 will be another stable version having Bi-Level Thread (BLT)
    and User-Level Process (ULP) features

    Details can be found in
    [IPDPS/RADR'20](presentation/IPDPS20-RADRws.key.pdf) in addition
    to the features introduced in the above v2.

### PiP v2 and v3 will be able to install by using Spack
    (https://github.com/spack/spack).

## Query

Send e-mails to pip@ml.riken.jp

Enjoy !

 Atsushi Hori (R-CCS)
