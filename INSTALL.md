# Process-in-Process (PiP) Installation

## Overview and GLIBC issue

PiP is a library to create processes in a process's virtual address
space implemented at user-level. Unfortunately the current GLIBC,
specifically libpthreads.so, has a problem and PiP is unable to
create sub-processes. To fix this issue, you have to install our
patched GLIBC before building the PiP library.

## Installation Procedure

The following is the procedure to install PiP from the source code.
If you are using CentOS 7 or RHEL 7, RPM packages and their yum repository
are available.

### Bulding PiP-glibc

You can download the patched GLIBC from the following URL.

    $ git clone https://github.com/RIKEN-SysSoft/PiP-glibc.git

With the GLIBC which Linux distribution provides, the number of PiP
tasks (see below) is limited to 15 or so. This limitation comes from
the fact that the number of name spaces is hard-coded as 16. In
addition to this, the current PiP library cannot run with some GLIBC
libraries. To avoid these issues, we recommend you to have our
patched GLIBC.

Here is the way to build the patched GLIBC.

Note that You should NOT specify "/", "/usr" or "/usr/local"
as <GLIBC_INSTALL_DIR>.

    $ mkdir PiP-glibc.build
    $ cd PiP-glibc.build
    $ ../PiP-glibc/build.sh <GLIBC_INSTALL_DIR>

### Building PiP

At the PiP configuration, you must specify the installed PiP-glibc.

    $ git clone https://github.com/RIKEN-SysSoft/PiP.git
    $ cd PiP
    $ ./configure --prefix=<PIP_INSTALL_DIR> --with-glibc-libdir=<GLIBC_INSTALL_DIR>/lib
    $ make
    $ make install

After the successful PiP installation, you must do the following,

    $ <PIP_INSTALL_DIR>/bin/piplnlibs

This command creates a number of symbolic links to the SOLIBs which
are not installed by the patched GLIBC installation.

### Building PiP-gdb

The following procedure installs the pip-gdb(1) command
as <PIP_GDB_INSTALL_DIR>/bin/pip-gdb :

    $ git clone https://github.com/RIKEN-SysSoft/PiP-gdb
    $ cd PiP-gdb
    $ ./build.sh --prefix=<PIP_GDB_INSTALL_DIR> --with-pip=<PIP_INSTALL_DIR>
