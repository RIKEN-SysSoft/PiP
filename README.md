# PiP [![Build Status](https://travis-ci.org/RIKEN-SysSoft/PiP.svg?branch=pip-1)](https://travis-ci.org/RIKEN-SysSoft/PiP)

Process-in-Process

## Description

PiP is a user-level library to have the best of the both worlds
of multi-process and multi-thread parallel execution models. PiP
allows a process to create sub-processes into the same virtual address
space where the parent process runs. The parent process and
sub-processes share the same address space, however, each process has
its own variable set. So, each process runs independently from the
other process. If some or all processes agree, then data own by a
process can be accessed by the other processes. Those processes share
the same address space, just like pthreads, and each process has its
own variables like a process. The parent process is called PiP process
and a sub-process are called a PiP task. 

## License

This project is licensed under the 2-clause simplified BSD License -
see the [LICENSE](LICENSE) file for details. 

## Installation

### Software Packages

* [PiP-glibc](https://github.com/RIKEN-SysSoft/PiP-glibc) - patched GNU libc for PiP
* [PiP](https://github.com/RIKEN-SysSoft/PiP) - Process in Process (this package)
* [PiP-gdb](https://github.com/RIKEN-SysSoft/PiP-gdb) - patched gdb to debug PiP root and PiP tasks.
* [MPICH over PIP environment](https://github.com/pmodels/mpich-pip/wiki) - patched MPICH for PiP

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

* pipcc(1) command  

    You can use pipcc(1) command to compile and link your PiP programs.  
    e.g.

        $ pipcc -Wall -O2 -g -c pipmodule.c
        $ pipcc -Wall -O2 -g -o pipprog pipprog.c

### To run your PiP programs

* pip-exec(1) command

    Let's assume your that have a non-PiP program(s) and wnat to run as PiP 
    tasks. All you have to do is to compile your program by using the above 
    pipcc(1) command and to use the pip-exec(1) command to run your program 
    as PiP tasks.

        $ pipcc myprog.c -o myprog
	$ pip-exec -n 8 ./myprog
	$ ./myprog

    In this case, the pip-exec(1) command becomes the PiP root and your program
    runs as 8 PiP tasks. Your program can also run as a normal (non-PiP) program
    without using the pip-exec(1) command. Note that the 'myprog.c' may or may not 
    call any PiP functions. 

    You may write your own PiP programs whcih includes the PiP root programming.
    In this case, your program can run without using the pip-exec(1) command.

    If you get the following message when you try to run your program;

        PiP-ERR(19673) './myprog' is not PIE

    Then this means that the 'myprog' is not compiled by using the pipcc(1) command
    properly. You may check if your program(s) can run as a PiP root and/or PiP task
    by using the pip-check(1) command;

        $ pip-check a.out
	a.out : Root&Task

    Above example shows that the 'a.out' program can run as a PiP root and PiP tasks.

* pips(1) command

  You can check if your PiP program is running or not by using the pips(1) 
  command.

    list the PiP tasks via the 'ps' command;
        $ pips -l [<command>]

    or, show the activities of PiP tasks via the 'top' command;
        $ pips -t [<command>]

    Here <command> is the name of PiP program you are running.

  Additionally you can kill your PiP tasks by using the same pips(1) command;

        $ pips -s KILL [<command>]

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

    pipman <topic>
    man -M <PIP_INSTALL_DIR>/share/man libpip

### HTML

HTML documents will be installed at <PIP_INSTALL_DIR>/doc/pip.

(In case of the RPM binary distribution, <PIP_INSTALL_DIR> is "/usr")

## Reference

A. Hori, M. Si, B. Gerofi, M. Takagi, J. Dayal, P. Balaji, and Y. Ishikawa. "Process-in-process: techniques for practical address-space sharing," In Proceedings of the 27th International Symposium on High-Performance Parallel and Distributed Computing (HPDC '18). ACM, New York, NY, USA, 131-143. DOI: https://doi.org/10.1145/3208040.3208045

## Presentation Slides

* [HPDC'18](presentation/HPDC18-PiP.key.pdf)
* [ROSS'18](presentation/Ross-2018-PiP.key.pdf)

## Query

Send e-mails to pip@ml.riken.jp

Enjoy !

Atsushi Hori (Riken CCS, Japan)
