# How to build this RPM:
#
#	$ rpm -Uvh gdb-7.6.1-94.el7.src.rpm
#	$ cd .../$SOMEWHERE/.../PiP-gdb
#	$ git diff origin/centos/7.6.1-94.el7-branch pip-centos7 >~/rpmbuild/SOURCES/gdb-pip.patch
#	$ rpmbuild -bb ~/rpmbuild/SPECS/pip-gdb.spec
#

%define	pip_gdb_release		pip0
%define pip_glibc_libdir	/opt/pip/lib
%define scl_prefix		pip-

# rpmbuild parameters:
# --with testsuite: Run the testsuite (biarch if possible).  Default is without.
# --with buildisa: Use %%{?_isa} for BuildRequires
# --without python: No python support.
# --with profile: gcc -fprofile-generate / -fprofile-use: Before better
#                 workload gets run it decreases the general performance now.
# --define 'scl somepkgname': Independent packages by scl-utils-build.

# RHEL-5 was the last not providing `/etc/rpm/macros.dist'.
%if 0%{!?dist:1}
%global rhel 5
%global dist .el5
%global el5 1
%endif
# RHEL-5 Brew does not set %{el5}, BZ 1002198 tps-srpmtest does not set %{rhel}.
%if "%{?dist}" == ".el5"
%global rhel 5
%global el5 1
%endif

%{?scl:%scl_package gdb}
%{!?scl:
#%global pkg_name %{name}
 %global pkg_name gdb
 %global _root_prefix %{_prefix}
 %global _root_datadir %{_datadir}
 %global _root_bindir %{_bindir}
 %global _root_libdir %{_libdir}
}

Summary: A GNU source-level debugger for C, C++, Fortran, Go and other languages for PiP (Process in Process)
Name: %{?scl_prefix}gdb
Requires: gdb

#global snap       20130423
# Freeze it when GDB gets branched
%global snapsrc    20130312
# See timestamp of source gnulib installed into gdb/gnulib/ .
%global snapgnulib 20121213
Version: 7.6.1

# The release always contains a leading reserved number, start it at 1.
# `upstream' is not a part of `name' to stay fully rpm dependencies compatible for the testing.
Release: 94%{?dist}.%{pip_gdb_release}

License: GPLv3+ and GPLv3+ with exceptions and GPLv2+ and GPLv2+ with exceptions and GPL+ and LGPLv2+ and BSD and Public Domain
Group: Development/Debuggers
# Do not provide URL for snapshots as the file lasts there only for 2 days.
# ftp://sourceware.org/pub/gdb/releases/gdb-%{version}.tar.bz2
Source: ftp://sourceware.org/pub/gdb/releases/gdb-%{version}.tar.bz2
Buildroot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
BuildRequires: pip
URL: http://gnu.org/software/gdb/

%if "%{scl}" == "devtoolset-1.1"
Obsoletes: devtoolset-1.0-%{pkg_name}
%endif

# For our convenience
%global gdb_src %{pkg_name}-%{version}
%global gdb_build build-%{_target_platform}
%global gdb_docdir %{_docdir}/%{name}-doc-%{version}

# Make sure we get rid of the old package gdb64, now that we have unified
# support for 32-64 bits in one single 64-bit gdb.
%ifarch ppc64
Obsoletes: gdb64 < 5.3.91
%endif

%global have_inproctrace 0
%ifarch %{ix86} x86_64
# RHEL-5.i386: [int foo, bar; bar = __sync_val_compare_and_swap(&foo, 0, 1);]
#              undefined reference to `__sync_val_compare_and_swap_4'
%if 0%{!?el5:1}
%global have_inproctrace 1
%endif # 0%{!?el5:1}
%endif # %{ix86} x86_64

# gdb-add-index cannot be run even for SCL package on RHEL<=6.
%if 0%{!?rhel:1} || 0%{?rhel} > 6
# eu-strip: -g recognizes .gdb_index as a debugging section. (#631997)
Conflicts: elfutils < 0.149
%endif

# https://fedorahosted.org/fpc/ticket/43 https://fedorahosted.org/fpc/ticket/109
Provides: bundled(libiberty) = %{snapsrc}
Provides: bundled(gnulib) = %{snapgnulib}
Provides: bundled(binutils) = %{snapsrc}
# https://fedorahosted.org/fpc/ticket/130
Provides: bundled(md5-gcc) = %{snapsrc}

# https://fedoraproject.org/wiki/Packaging:Guidelines#BuildRequires_and_.25.7B_isa.7D
%global buildisa %{?_with_buildisa:%{?_isa}}

%global librpmver 3
%if 0%{?__isa_bits} == 64
%global librpmname librpm.so.%{librpmver}()(64bit)
%else
%global librpmname librpm.so.%{librpmver}
%endif
BuildRequires: rpm-libs%{buildisa}
%if 0%{?_with_buildisa:1}
BuildRequires: %{librpmname}
%endif

# GDB patches have the format `gdb-<version>-bz<red-hat-bz-#>-<desc>.patch'.
# They should be created using patch level 1: diff -up ./gdb (or gdb-6.3/gdb).

#=
#push=Should be pushed upstream.
#maybepush=Should be pushed upstream unless it got obsoleted there.
#fedora=Should stay as a Fedora patch.
#ia64=Drop after RHEL-5 rebases and rebuilds are no longer meaningful.
#fedoratest=Keep it in Fedora only as a regression test safety.
#+ppc=Specific for ppc32/ppc64/ppc*
#+work=Requires some nontrivial work.

# Cleanup any leftover testsuite processes as it may stuck mock(1) builds.
#=push
Source2: gdb-orphanripper.c

# Man page for gstack(1).
#=push
Source3: gdb-gstack.man

# /etc/gdbinit (from Debian but with Fedora compliant location).
#=fedora
Source4: gdbinit

# libstdc++ pretty printers from GCC SVN HEAD (4.5 experimental).
%global libstdcxxpython gdb-libstdc++-v3-python-r155978
Source5: %{libstdcxxpython}.tar.bz2

# Provide gdbtui for RHEL-5 and RHEL-6 as it is removed upstream (BZ 797664).
Source6: gdbtui

# Work around out-of-date dejagnu that does not have KFAIL
#=drop: That dejagnu is too old to be supported.
Patch1: gdb-6.3-rh-dummykfail-20041202.patch

# Match the Fedora's version info.
#=fedora
Patch2: gdb-6.3-rh-testversion-20041202.patch

# Check that libunwind works - new test then fix
#=ia64
Patch3: gdb-6.3-rh-testlibunwind-20041202.patch

# Better parse 64-bit PPC system call prologues.
#=maybepush+ppc: Write new testcase.
Patch105: gdb-6.3-ppc64syscall-20040622.patch

# Include the pc's section when doing a symbol lookup so that the
# correct symbol is found.
#=maybepush: Write new testcase.
Patch111: gdb-6.3-ppc64displaysymbol-20041124.patch

# Fix upstream `set scheduler-locking step' vs. upstream PPC atomic seqs.
#=push+work: It is a bit difficult patch, a part is ppc specific.
Patch112: gdb-6.6-scheduler_locking-step-sw-watchpoints2.patch
# Make upstream `set scheduler-locking step' as default.
#=push+work: How much is scheduler-locking relevant after non-stop?
Patch260: gdb-6.6-scheduler_locking-step-is-default.patch

# Add a wrapper script to GDB that implements pstack using the
# --readnever option.
#=push+work: with gdbindex maybe --readnever should no longer be used.
Patch118: gdb-6.3-gstack-20050411.patch

# VSYSCALL and PIE
#=fedoratest
Patch122: gdb-6.3-test-pie-20050107.patch
#=push: May get obsoleted by Tom's unrelocated objfiles patch.
Patch389: gdb-archer-pie-addons.patch
#=push+work: Breakpoints disabling matching should not be based on address.
Patch394: gdb-archer-pie-addons-keep-disabled.patch

# Get selftest working with sep-debug-info
#=fedoratest
Patch125: gdb-6.3-test-self-20050110.patch

# Test support of multiple destructors just like multiple constructors
#=fedoratest
Patch133: gdb-6.3-test-dtorfix-20050121.patch

# Fix to support executable moving
#=fedoratest
Patch136: gdb-6.3-test-movedir-20050125.patch

# Fix gcore for threads
#=ia64
Patch140: gdb-6.3-gcore-thread-20050204.patch

# Test sibling threads to set threaded watchpoints for x86 and x86-64
#=fedoratest
Patch145: gdb-6.3-threaded-watchpoints2-20050225.patch

# Do not issue warning message about first page of storage for ia64 gcore
#=ia64
Patch153: gdb-6.3-ia64-gcore-page0-20050421.patch

# IA64 sigtramp prev register patch
#=ia64
Patch158: gdb-6.3-ia64-sigtramp-frame-20050708.patch

# IA64 gcore speed-up patch
#=ia64
Patch160: gdb-6.3-ia64-gcore-speedup-20050714.patch

# Notify observers that the inferior has been created
#=fedoratest
Patch161: gdb-6.3-inferior-notification-20050721.patch

# Fix ia64 info frame bug
#=ia64
Patch162: gdb-6.3-ia64-info-frame-fix-20050725.patch

# Verify printing of inherited members test
#=fedoratest
Patch163: gdb-6.3-inheritancetest-20050726.patch

# Add readnever option
#=push
Patch164: gdb-6.3-readnever-20050907.patch

# Fix ia64 gdb problem with user-specified SIGILL handling
#=ia64
Patch169: gdb-6.3-ia64-sigill-20051115.patch

# Fix debuginfo addresses resolving for --emit-relocs Linux kernels (BZ 203661).
#=push+work: There was some mail thread about it, this patch may be a hack.
Patch188: gdb-6.5-bz203661-emit-relocs.patch

# Support TLS symbols (+`errno' suggestion if no pthread is found) (BZ 185337).
#=push+work: It should be replaced by existing uncommitted Roland's glibc patch for TLS without libpthreads.
Patch194: gdb-6.5-bz185337-resolve-tls-without-debuginfo-v2.patch

# Fix TLS symbols resolving for shared libraries with a relative pathname.
# The testsuite needs `gdb-6.5-tls-of-separate-debuginfo.patch'.
#=fedoratest+work: One should recheck if it is really fixed upstream.
Patch196: gdb-6.5-sharedlibrary-path.patch

# Suggest fixing your target architecture for gdbserver(1) (BZ 190810).
# FIXME: It could be autodetected.
#=push+work: There are more such error cases that can happen.
Patch199: gdb-6.5-bz190810-gdbserver-arch-advice.patch

# Testcase for deadlocking on last address space byte; for corrupted backtraces.
#=fedoratest
Patch211: gdb-6.5-last-address-space-byte-test.patch

# Improved testsuite results by the testsuite provided by the courtesy of BEA.
#=fedoratest+work: For upstream it should be rewritten as a dejagnu test, the test of no "??" was useful.
Patch208: gdb-6.5-BEA-testsuite.patch

# Fix readline segfault on excessively long hand-typed lines.
#=fedoratest
Patch213: gdb-6.5-readline-long-line-crash-test.patch

# Fix bogus 0x0 unwind of the thread's topmost function clone(3) (BZ 216711).
#=fedora
Patch214: gdb-6.5-bz216711-clone-is-outermost.patch

# Test sideeffects of skipping ppc .so libs trampolines (BZ 218379).
#=fedoratest
Patch216: gdb-6.5-bz218379-ppc-solib-trampoline-test.patch

# Fix lockup on trampoline vs. its function lookup; unreproducible (BZ 218379).
#=fedora
Patch217: gdb-6.5-bz218379-solib-trampoline-lookup-lock-fix.patch

# Find symbols properly at their original (included) file (BZ 109921).
#=fedoratest
Patch225: gdb-6.5-bz109921-DW_AT_decl_file-test.patch

# Update PPC unwinding patches to their upstream variants (BZ 140532).
#=fedoratest+ppc
Patch229: gdb-6.3-bz140532-ppc-unwinding-test.patch

# Testcase for exec() from threaded program (BZ 202689).
#=fedoratest
Patch231: gdb-6.3-bz202689-exec-from-pthread-test.patch

# Backported fixups post the source tarball.
#Xdrop: Just backports.
Patch232: gdb-upstream.patch
Patch828: gdb-upstream-man-gcore-1of2.patch
Patch829: gdb-upstream-man-gcore-2of2.patch
# Backported Python frame filters (Phil Muldoon).
Patch836: gdb-upstream-framefilters-1of2.patch
Patch837: gdb-upstream-framefilters-2of2.patch

# Testcase for PPC Power6/DFP instructions disassembly (BZ 230000).
#=fedoratest+ppc
Patch234: gdb-6.6-bz230000-power6-disassembly-test.patch

# Temporary support for shared libraries >2GB on 64bit hosts. (BZ 231832)
#=push+work: Upstream should have backward compat. API: libc-alpha: <20070127104539.GA9444@.*>
Patch235: gdb-6.3-bz231832-obstack-2gb.patch

# Allow running `/usr/bin/gcore' with provided but inaccessible tty (BZ 229517).
#=fedoratest
Patch245: gdb-6.6-bz229517-gcore-without-terminal.patch

# Notify user of a child forked process being detached (BZ 235197).
#=push: This is more about discussion if/what should be printed.
Patch247: gdb-6.6-bz235197-fork-detach-info.patch

# Avoid too long timeouts on failing cases of "annota1.exp annota3.exp".
#=fedoratest
Patch254: gdb-6.6-testsuite-timeouts.patch

# Support for stepping over PPC atomic instruction sequences (BZ 237572).
#=fedoratest
Patch258: gdb-6.6-bz237572-ppc-atomic-sequence-test.patch

# Test kernel VDSO decoding while attaching to an i386 process.
#=fedoratest
Patch263: gdb-6.3-attach-see-vdso-test.patch

# Test leftover zombie process (BZ 243845).
#=fedoratest
Patch271: gdb-6.5-bz243845-stale-testing-zombie-test.patch

# New locating of the matching binaries from the pure core file (build-id).
#=push
Patch274: gdb-6.6-buildid-locate.patch
# Fix loading of core files without build-ids but with build-ids in executables.
#=push
Patch659: gdb-6.6-buildid-locate-solib-missing-ids.patch
#=push
Patch353: gdb-6.6-buildid-locate-rpm.patch
#=push
Patch415: gdb-6.6-buildid-locate-core-as-arg.patch
# Workaround librpm BZ 643031 due to its unexpected exit() calls (BZ 642879).
#=push
Patch519: gdb-6.6-buildid-locate-rpm-librpm-workaround.patch
# [SCL] Skip deprecated .gdb_index warning for Red Hat built files (BZ 953585).
Patch833: gdb-6.6-buildid-locate-rpm-scl.patch

# Add kernel vDSO workaround (`no loadable ...') on RHEL-5 (kernel BZ 765875).
#=push
Patch276: gdb-6.6-bfd-vdso8k.patch

# Fix displaying of numeric char arrays as strings (BZ 224128).
#=fedoratest: But it is failing anyway, one should check the behavior more.
Patch282: gdb-6.7-charsign-test.patch

# Test PPC hiding of call-volatile parameter register.
#=fedoratest+ppc
Patch284: gdb-6.7-ppc-clobbered-registers-O2-test.patch

# Testsuite fixes for more stable/comparable results.
#=fedoratest
Patch287: gdb-6.7-testsuite-stable-results.patch

# Test ia64 memory leaks of the code using libunwind.
#=fedoratest
Patch289: gdb-6.5-ia64-libunwind-leak-test.patch

# Test hiding unexpected breakpoints on intentional step commands.
#=fedoratest
Patch290: gdb-6.5-missed-trap-on-step-test.patch

# Support DW_TAG_interface_type the same way as DW_TAG_class_type (BZ 426600).
#=fedoratest
Patch294: gdb-6.7-bz426600-DW_TAG_interface_type-test.patch

# Test gcore memory and time requirements for large inferiors.
#=fedoratest
Patch296: gdb-6.5-gcore-buffer-limit-test.patch

# Test debugging statically linked threaded inferiors (BZ 239652).
#  - It requires recent glibc to work in this case properly.
#=fedoratest
Patch298: gdb-6.6-threads-static-test.patch

# Test GCORE for shmid 0 shared memory mappings.
#=fedoratest: But it is broken anyway, sometimes the case being tested is not reproducible.
Patch309: gdb-6.3-mapping-zero-inode-test.patch

# Test a crash on `focus cmd', `focus prev' commands.
#=fedoratest
Patch311: gdb-6.3-focus-cmd-prev-test.patch

# Test various forms of threads tracking across exec() (BZ 442765).
#=fedoratest
Patch315: gdb-6.8-bz442765-threaded-exec-test.patch

# Silence memcpy check which returns false positive (sparc64)
#=push: But it is just a GCC workaround, look up the existing GCC PR for it.
Patch317: gdb-6.8-sparc64-silence-memcpy-check.patch

# Test a crash on libraries missing the .text section.
#=fedoratest
Patch320: gdb-6.5-section-num-fixup-test.patch

# Fix register assignments with no GDB stack frames (BZ 436037).
#=push+work: This fix is incorrect.
Patch330: gdb-6.8-bz436037-reg-no-longer-active.patch

# Make the GDB quit processing non-abortable to cleanup everything properly.
#=fedora: It was useful only after gdb-6.8-attach-signalled-detach-stopped.patch .
Patch331: gdb-6.8-quit-never-aborts.patch

# [RHEL5] Workaround kernel for detaching SIGSTOPped processes (BZ 809382).
#=fedora
Patch335: gdb-rhel5-compat.patch

# [RHEL5,RHEL6] Fix attaching to stopped processes.
#=fedora
Patch337: gdb-6.8-attach-signalled-detach-stopped.patch

# Test the watchpoints conditionals works.
#=fedoratest
Patch343: gdb-6.8-watchpoint-conditionals-test.patch

# Fix resolving of variables at locations lists in prelinked libs (BZ 466901).
#=fedoratest
Patch348: gdb-6.8-bz466901-backtrace-full-prelinked.patch

# The merged branch `archer-jankratochvil-fedora15' of:
# http://sourceware.org/gdb/wiki/ProjectArcher
#=push+work
Patch349: gdb-archer.patch

# Fix parsing elf64-i386 files for kdump PAE vmcore dumps (BZ 457187).
# - Turn on 64-bit BFD support, globally enable AC_SYS_LARGEFILE.
#=fedoratest
Patch360: gdb-6.8-bz457187-largefile-test.patch

# New test for step-resume breakpoint placed in multiple threads at once.
#=fedoratest
Patch381: gdb-simultaneous-step-resume-breakpoint-test.patch

# Fix GNU/Linux core open: Can't read pathname for load map: Input/output error.
# Fix regression of undisplayed missing shared libraries caused by a fix for.
#=push+work: It should be in glibc: libc-alpha: <20091004161706.GA27450@.*>
Patch382: gdb-core-open-vdso-warning.patch

# Fix syscall restarts for amd64->i386 biarch.
#=push
Patch391: gdb-x86_64-i386-syscall-restart.patch

# Fix stepping with OMP parallel Fortran sections (BZ 533176).
#=push+work: It requires some better DWARF annotations.
Patch392: gdb-bz533176-fortran-omp-step.patch

# Use gfortran44 when running the testsuite on RHEL-5.
#=fedoratest
Patch393: gdb-rhel5-gcc44.patch

# Fix regression by python on ia64 due to stale current frame.
#=push
Patch397: gdb-follow-child-stale-parent.patch

# Workaround ccache making lineno non-zero for command-line definitions.
#=fedoratest: ccache is rarely used and it is even fixed now.
Patch403: gdb-ccache-workaround.patch

# Testcase for "Do not make up line information" fix by Daniel Jacobowitz.
#=fedoratest
Patch407: gdb-lineno-makeup-test.patch

# Test power7 ppc disassembly.
#=fedoratest+ppc
Patch408: gdb-ppc-power7-test.patch

# Fix i386+x86_64 rwatch+awatch before run, regression against 6.8 (BZ 541866).
# Fix i386 rwatch+awatch before run (BZ 688788, on top of BZ 541866).
#=push+work: It should be fixed properly instead.
Patch417: gdb-bz541866-rwatch-before-run.patch

# Workaround non-stop moribund locations exploited by kernel utrace (BZ 590623).
#=push+work: Currently it is still not fully safe.
Patch459: gdb-moribund-utrace-workaround.patch

# Fix follow-exec for C++ programs (bugreported by Martin Stransky).
#=fedoratest
Patch470: gdb-archer-next-over-throw-cxx-exec.patch

# Backport DWARF-4 support (BZ 601887, Tom Tromey).
#=fedoratest
Patch475: gdb-bz601887-dwarf4-rh-test.patch

# [delayed-symfile] Test a backtrace regression on CFIs without DIE (BZ 614604).
#=fedoratest
Patch490: gdb-test-bt-cfi-without-die.patch

# Provide /usr/bin/gdb-add-index for rpm-build (Tom Tromey).
#=fedora: Re-check against the upstream version.
Patch491: gdb-gdb-add-index-script.patch

# Out of memory is just an error, not fatal (uninitialized VLS vars, BZ 568248).
#=drop+work: Inferior objects should be read in parts, then this patch gets obsoleted.
Patch496: gdb-bz568248-oom-is-error.patch

# Verify GDB Python built-in function gdb.solib_address exists (BZ # 634108).
#=fedoratest
Patch526: gdb-bz634108-solib_address.patch

# New test gdb.arch/x86_64-pid0-core.exp for kernel PID 0 cores (BZ 611435).
#=fedoratest
Patch542: gdb-test-pid0-core.patch

# [archer-tromey-delayed-symfile] New test gdb.dwarf2/dw2-aranges.exp.
#=fedoratest
Patch547: gdb-test-dw2-aranges.patch

# [archer-keiths-expr-cumulative+upstream] Import C++ testcases.
#=fedoratest
Patch548: gdb-test-expr-cumulative-archer.patch

# Toolchain on sparc is slightly broken and debuginfo files are generated
# with non 64bit aligned tables/offsets.
# See for example readelf -S ../Xvnc.debug.
#
# As a consenquence calculation of sectp->filepos as used in
# dwarf2_read_section (gdb/dwarf2read.c:1525) will return a non aligned buffer
# that cannot be used directly as done with MMAP.
# Usage will result in a BusError.
#
# While we figure out what's wrong in the toolchain and do a full archive
# rebuild to fix it, we need to be able to use gdb :)
#=push+work
Patch579: gdb-7.2.50-sparc-add-workaround-to-broken-debug-files.patch

# Fix dlopen of libpthread.so, patched glibc required (Gary Benson, BZ 669432).
# Fix crash regression from the dlopen of libpthread.so fix (BZ 911712).
# Fix performance regression when inferior opens many libraries (Gary Benson).
#=drop
Patch718: gdb-dlopen-stap-probe-1of9.patch
Patch719: gdb-dlopen-stap-probe-2of9.patch
Patch720: gdb-dlopen-stap-probe-3of9.patch
Patch721: gdb-dlopen-stap-probe-4of9.patch
Patch722: gdb-dlopen-stap-probe-5of9.patch
Patch723: gdb-dlopen-stap-probe-6of9.patch
Patch822: gdb-dlopen-stap-probe-7of9.patch
Patch827: gdb-dlopen-stap-probe-8of9.patch
Patch619: gdb-dlopen-stap-probe-9of9.patch

# Work around PR libc/13097 "linux-vdso.so.1" warning message.
#=push
Patch627: gdb-glibc-vdso-workaround.patch

# Hack for proper PIE run of the testsuite.
#=fedoratest
Patch634: gdb-runtest-pie-override.patch

# Work around readline-6.2 incompatibility not asking for --more-- (BZ 701131).
#=fedora
Patch642: gdb-readline62-ask-more-rh.patch

# Print reasons for failed attach/spawn incl. SELinux deny_ptrace (BZ 786878).
#=push
Patch653: gdb-attach-fail-reasons-5of5.patch
#=fedora
Patch657: gdb-attach-fail-reasons-5of5configure.patch

# Workaround crashes from stale frame_info pointer (BZ 804256).
#=fedora
Patch661: gdb-stale-frame_info.patch

# Workaround PR libc/14166 for inferior calls of strstr.
#=fedora: Compatibility with RHELs (unchecked which ones).
Patch690: gdb-glibc-strstr-workaround.patch

# Include testcase for `Unable to see a variable inside a module (XLF)' (BZ 823789).
#=fedoratest
#+ppc
Patch698: gdb-rhel5.9-testcase-xlf-var-inside-mod.patch

# Testcase for `Setting solib-absolute-prefix breaks vDSO' (BZ 818343).
#=fedoratest
Patch703: gdb-rhbz-818343-set-solib-absolute-prefix-testcase.patch

# Fix `GDB cannot access struct member whose offset is larger than 256MB'
# (RH BZ 795424).
#=push+work
Patch811: gdb-rhbz795424-bitpos-20of25.patch
Patch812: gdb-rhbz795424-bitpos-21of25.patch
Patch813: gdb-rhbz795424-bitpos-22of25.patch
Patch814: gdb-rhbz795424-bitpos-23of25.patch
Patch816: gdb-rhbz795424-bitpos-25of25.patch
Patch817: gdb-rhbz795424-bitpos-25of25-test.patch
Patch818: gdb-rhbz795424-bitpos-lazyvalue.patch

# Import regression test for `gdb/findvar.c:417: internal-error:
# read_var_value: Assertion `frame' failed.' (RH BZ 947564) from RHEL 6.5.
#=fedoratest
Patch832: gdb-rhbz947564-findvar-assertion-frame-failed-testcase.patch

# Fix gcore for vDSO (on ppc64).
#=drop
Patch834: gdb-vdso-gcore.patch

# Fix needless expansion of non-gdbindex symtabs (Doug Evans).
#=drop
Patch835: gdb-psymtab-expand.patch

# [ppc] Support Power8 CPU (IBM, BZ 731875).
#=drop
Patch840: gdb-power8-1of2.patch
Patch841: gdb-power8-2of2.patch

# Fix crash on 'enable count' (Simon Marchi, BZ 993118).
Patch843: gdb-enable-count-crash.patch

# Fix the case when GDB leaks memory because value_struct_elt
# does not call check_typedef.  (Doug Evans, BZ 15695, filed as
# RH BZ 1013453).
Patch844: gdb-rhbz1013453-value-struct-elt-memory-leak.patch

# Fix explicit Class:: inside class scope (BZ 874817, Keith Seitz).
Patch845: gdb-implicit-this.patch

# Fix crash of -readnow /usr/lib/debug/usr/bin/gnatbind.debug (BZ 1069211).
Patch850: gdb-gnat-dwarf-crash-1of3.patch
Patch851: gdb-gnat-dwarf-crash-2of3.patch
Patch852: gdb-gnat-dwarf-crash-3of3.patch

# Port GDB to PPC64LE (ELFv2) (RH BZ 1125820).
# Work mostly done by Ulrich Weigand and Alan Modra, from IBM.
Patch928: gdb-rhbz1125820-ppc64le-enablement-01of37.patch
Patch929: gdb-rhbz1125820-ppc64le-enablement-02of37.patch
Patch930: gdb-rhbz1125820-ppc64le-enablement-03of37.patch
Patch931: gdb-rhbz1125820-ppc64le-enablement-04of37.patch
Patch932: gdb-rhbz1125820-ppc64le-enablement-05of37.patch
Patch933: gdb-rhbz1125820-ppc64le-enablement-06of37.patch
Patch934: gdb-rhbz1125820-ppc64le-enablement-07of37.patch
Patch935: gdb-rhbz1125820-ppc64le-enablement-08of37.patch
Patch936: gdb-rhbz1125820-ppc64le-enablement-09of37.patch
Patch937: gdb-rhbz1125820-ppc64le-enablement-10of37.patch
Patch938: gdb-rhbz1125820-ppc64le-enablement-11of37.patch
Patch939: gdb-rhbz1125820-ppc64le-enablement-12of37.patch
Patch940: gdb-rhbz1125820-ppc64le-enablement-13of37.patch
Patch941: gdb-rhbz1125820-ppc64le-enablement-14of37.patch
Patch942: gdb-rhbz1125820-ppc64le-enablement-15of37.patch
Patch943: gdb-rhbz1125820-ppc64le-enablement-16of37.patch
Patch944: gdb-rhbz1125820-ppc64le-enablement-17of37.patch
Patch945: gdb-rhbz1125820-ppc64le-enablement-18of37.patch
Patch946: gdb-rhbz1125820-ppc64le-enablement-19of37.patch
Patch947: gdb-rhbz1125820-ppc64le-enablement-20of37.patch
Patch948: gdb-rhbz1125820-ppc64le-enablement-21of37.patch
Patch949: gdb-rhbz1125820-ppc64le-enablement-22of37.patch
Patch950: gdb-rhbz1125820-ppc64le-enablement-23of37.patch
Patch951: gdb-rhbz1125820-ppc64le-enablement-24of37.patch
Patch952: gdb-rhbz1125820-ppc64le-enablement-25of37.patch
Patch953: gdb-rhbz1125820-ppc64le-enablement-26of37.patch
Patch954: gdb-rhbz1125820-ppc64le-enablement-27of37.patch
Patch955: gdb-rhbz1125820-ppc64le-enablement-28of37.patch
Patch956: gdb-rhbz1125820-ppc64le-enablement-29of37.patch
Patch957: gdb-rhbz1125820-ppc64le-enablement-30of37.patch
Patch958: gdb-rhbz1125820-ppc64le-enablement-31of37.patch
Patch959: gdb-rhbz1125820-ppc64le-enablement-32of37.patch
Patch960: gdb-rhbz1125820-ppc64le-enablement-33of37.patch
Patch961: gdb-rhbz1125820-ppc64le-enablement-34of37.patch
Patch962: gdb-rhbz1125820-ppc64le-enablement-35of37.patch
Patch969: gdb-rhbz1125820-ppc64le-enablement-36of37.patch
Patch974: gdb-rhbz1125820-ppc64le-enablement-37of37.patch

# Fix 'incomplete backtraces from core dumps from crash in signal
# handler' (RH BZ 1086894).
Patch963: gdb-rhbz1086894-incomplete-bt-coredump-signal-handler.patch

# Fix 'gdb for aarch64 needs the ability to single step through atomic
# sequences' (RH BZ 1093259, Kyle McMartin).
Patch964: gdb-rhbz1093259-aarch64-single-step-atomic-seq.patch

# Fix '[RHEL7] Can't access TLS variables in statically linked
# binaries' (RH BZ 1080657, Jan Kratochvil).
Patch965: gdb-rhbz1080657-tls-variable-static-linked-binary-1of3.patch
Patch966: gdb-rhbz1080657-tls-variable-static-linked-binary-2of3.patch
Patch967: gdb-rhbz1080657-tls-variable-static-linked-binary-3of3.patch

# Include testcase for 'Global variable nested by another dso shows up
# incorrectly in gdb' (RH BZ 809179).  The fix for this bug is already
# included in the RHEL-7.1 GDB.
Patch968: gdb-rhbz809179-global-variable-nested-dso.patch

# Fix 'Slow gstack performance' (RH BZ 1103894, Jan Kratochvil).
Patch972: gdb-rhbz1103894-slow-gstack-performance.patch

# Fix '[RFE] please add add-auto-load-scripts-directory command' (RH
# BZ 1163339, Jan Kratochvil).
Patch976: gdb-rhbz1163339-add-auto-load-scripts-directory.patch

# Fix 'gdb/linux-nat.c:1411: internal-error:
# linux_nat_post_attach_wait: Assertion `pid == new_pid' failed.'
# (Pedro Alves, RH BZ 1210135).
Patch994: gdb-rhbz1210135-internal-error-linux_nat_post_attach_wait.patch

# Fix 'gdb internal error' [Threaded program calls clone, which
# crashes GDB] (Pedro Alves, RH BZ 1210889).
Patch995: gdb-rhbz1210889-thread-call-clone.patch

# Fix '`catch syscall' doesn't work for parent after `fork' is called'
# (Philippe Waroquiers, RH BZ 1149207).
Patch996: gdb-rhbz1149207-catch-syscall-after-fork.patch

# Fix 'ppc64: segv in `gdb -q
# /lib/modules/3.10.0-215.el7.ppc64/kernel/fs/nfsd/nfsd.ko`' (Jan
# Kratochvil, RH BZ 1190506).
Patch997: gdb-rhbz1190506-segv-in-ko-ppc64.patch

# Fix 'apply_frame_filter() wrong backport' (Tom Tromey, RH BZ
# 1197665).
Patch998: gdb-rhbz1197665-fix-applyframefilter-backport.patch

# Implement '[7.2 FEAT] Enable reverse debugging support for PowerPC
# on GDB' (Wei-cheng Wang, RH BZ 1218710, thanks to Edjunior Machado
# for backporting).
Patch999: gdb-rhbz1218710-reverse-debugging-ppc-1of7.patch
Patch1000: gdb-rhbz1218710-reverse-debugging-ppc-2of7.patch
Patch1001: gdb-rhbz1218710-reverse-debugging-ppc-3of7.patch
Patch1002: gdb-rhbz1218710-reverse-debugging-ppc-4of7.patch
Patch1003: gdb-rhbz1218710-reverse-debugging-ppc-5of7.patch
Patch1004: gdb-rhbz1218710-reverse-debugging-ppc-6of7.patch
Patch1005: gdb-rhbz1218710-reverse-debugging-ppc-7of7.patch

# Fix 'gdb invoked oom-killer analyzing a vmcore on aarch64' (Pedro
# Alves, Tom Tromey, RH BZ 1225569).
Patch1006: gdb-rhbz1225569-oom-killer-aarch64-frame-same-id-1of8.patch
Patch1007: gdb-rhbz1225569-oom-killer-aarch64-frame-same-id-2of8.patch
Patch1008: gdb-rhbz1225569-oom-killer-aarch64-frame-same-id-3of8.patch
Patch1009: gdb-rhbz1225569-oom-killer-aarch64-frame-same-id-4of8.patch
Patch1010: gdb-rhbz1225569-oom-killer-aarch64-frame-same-id-5of8.patch
Patch1011: gdb-rhbz1225569-oom-killer-aarch64-frame-same-id-6of8.patch
Patch1012: gdb-rhbz1225569-oom-killer-aarch64-frame-same-id-7of8.patch
Patch1013: gdb-rhbz1225569-oom-killer-aarch64-frame-same-id-8of8.patch

# Fix 'gdb output a internal-error: Assertion `num_lwps (GET_PID
# (inferior_ptid)) == 1' failed' (Pedro Alves, RH BZ 1184724).
Patch1014: gdb-rhbz1184724-gdb-internal-error-num-lwps.patch

# Fix '[7.2 FEAT] GDB Transactional Diagnostic Block support on System
# z' (Andreas Arnez, RH BZ 1105165).
Patch1015: gdb-rhbz1105165-ibm-tdb-support-system-z-1of9.patch
Patch1016: gdb-rhbz1105165-ibm-tdb-support-system-z-2of9.patch
Patch1017: gdb-rhbz1105165-ibm-tdb-support-system-z-3of9.patch
Patch1018: gdb-rhbz1105165-ibm-tdb-support-system-z-4of9.patch
Patch1019: gdb-rhbz1105165-ibm-tdb-support-system-z-5of9.patch
Patch1020: gdb-rhbz1105165-ibm-tdb-support-system-z-6of9.patch
Patch1021: gdb-rhbz1105165-ibm-tdb-support-system-z-7of9.patch
Patch1022: gdb-rhbz1105165-ibm-tdb-support-system-z-8of9.patch
Patch1023: gdb-rhbz1105165-ibm-tdb-support-system-z-9of9.patch

# Fix '[ppc64] and [s390x] wrong prologue skip on -O2 -g code' (Jan
# Kratochvil, RH BZ 1084404).
Patch1024: gdb-rhbz1084404-ppc64-s390x-wrong-prologue-skip-O2-g-1of3.patch
Patch1025: gdb-rhbz1084404-ppc64-s390x-wrong-prologue-skip-O2-g-2of3.patch
Patch1026: gdb-rhbz1084404-ppc64-s390x-wrong-prologue-skip-O2-g-3of3.patch

# Fix 'Backport upstream GDB AArch64 SDT probe support to RHEL GDB'
# (Sergio Durigan Junior, RH BZ 1247107).
Patch1027: gdb-rhbz1247107-backport-aarch64-stap-sdt-support-1of2.patch
Patch1028: gdb-rhbz1247107-backport-aarch64-stap-sdt-support-2of2.patch

# [ppc64le] Use skip_entrypoint for skip_trampoline_code (RH BZ 1260558).
Patch1030: gdb-rhbz1260558-ppc64le-skip_trampoline_code.patch

# Support for /proc/PID/coredump_filter (Sergio Durigan Junior, RH BZ 1265351).
Patch1047: gdb-rhbz1085906-coredump_filter-1of2.patch
Patch1048: gdb-rhbz1085906-coredump_filter-2of2.patch

# Fix DWARF compatibility with icc (RH BZ 1268831).
Patch1055: gdb-rhbz1268831-icc-compat.patch

# Fix crash on Python frame filters with unreadable arg (RH BZ 1281351).
Patch1057: gdb-rhbz1281351-python-unreadable-arg.patch

# Add MPX instructions decoding (Intel, binutils RH BZ was 1138642).
# Add clflushopt instruction decode (Intel, RH BZ 1262471).
Patch1071: binutils-2.23.52.0.1-avx512-mpx-sha.patch
Patch1072: gdb-opcodes-clflushopt.patch
Patch1073: gdb-opcodes-clflushopt-test.patch

# [aarch64] Fix hardware watchpoints (RH BZ 1261564).
Patch1108: gdb-rhbz1261564-aarch64-hw-watchpoint-1of5.patch
Patch1109: gdb-rhbz1261564-aarch64-hw-watchpoint-2of5.patch
Patch1110: gdb-rhbz1261564-aarch64-hw-watchpoint-3of5.patch
Patch1111: gdb-rhbz1261564-aarch64-hw-watchpoint-4of5.patch
Patch1112: gdb-rhbz1261564-aarch64-hw-watchpoint-5of5.patch
Patch1113: gdb-rhbz1261564-aarch64-hw-watchpoint-test.patch 

# [s390] Import IBM z13 support (RH BZ 1182151).
Patch1080: gdb-rhbz1182151-ibm-z13-01of22.patch
Patch1081: gdb-rhbz1182151-ibm-z13-02of22.patch
Patch1082: gdb-rhbz1182151-ibm-z13-03of22.patch
Patch1083: gdb-rhbz1182151-ibm-z13-04of22.patch
Patch1084: gdb-rhbz1182151-ibm-z13-05of22.patch
Patch1085: gdb-rhbz1182151-ibm-z13-06of22.patch
Patch1086: gdb-rhbz1182151-ibm-z13-07of22.patch
Patch1087: gdb-rhbz1182151-ibm-z13-08of22.patch
Patch1088: gdb-rhbz1182151-ibm-z13-09of22.patch
Patch1089: gdb-rhbz1182151-ibm-z13-10of22.patch
Patch1090: gdb-rhbz1182151-ibm-z13-11of22.patch
Patch1091: gdb-rhbz1182151-ibm-z13-12of22.patch
Patch1092: gdb-rhbz1182151-ibm-z13-13of22.patch
Patch1093: gdb-rhbz1182151-ibm-z13-14of22.patch
Patch1094: gdb-rhbz1182151-ibm-z13-15of22.patch
Patch1095: gdb-rhbz1182151-ibm-z13-16of22.patch
Patch1096: gdb-rhbz1182151-ibm-z13-17of22.patch
Patch1097: gdb-rhbz1182151-ibm-z13-18of22.patch
Patch1098: gdb-rhbz1182151-ibm-z13-19of22.patch
Patch1099: gdb-rhbz1182151-ibm-z13-20of22.patch
Patch1100: gdb-rhbz1182151-ibm-z13-21of22.patch
Patch1101: gdb-rhbz1182151-ibm-z13-22of22.patch

# Fix Python "Cannot locate object file for block." (Tom Tromey, RH BZ 1325795).
patch1121: gdb-rhbz1325795-framefilters-1of2.patch
patch1122: gdb-rhbz1325795-framefilters-2of2.patch
patch1123: gdb-rhbz1325795-framefilters-test.patch

# Never kill PID on: gdb exec PID (Jan Kratochvil, RH BZ 1326476).
Patch1053: gdb-bz1326476-attach-kills.patch

# Implement gdbserver support for containers (Gary Benson, RH BZ 1186918).
Patch1103: gdb-rhbz1186918-gdbserver-in-container-1of8.patch
Patch1104: gdb-rhbz1186918-gdbserver-in-container-2of8.patch
Patch1105: gdb-rhbz1186918-gdbserver-in-container-3of8.patch
Patch1106: gdb-rhbz1186918-gdbserver-in-container-4of8.patch
Patch1114: gdb-rhbz1186918-gdbserver-in-container-5of8.patch
Patch1115: gdb-rhbz1186918-gdbserver-in-container-6of8.patch
Patch1116: gdb-rhbz1186918-gdbserver-in-container-7of8.patch
Patch1118: gdb-rhbz1186918-gdbserver-in-container-8of8.patch

# Import bare DW_TAG_lexical_block (RH BZ 1325396).
Patch1128: gdb-bare-DW_TAG_lexical_block-1of2.patch
Patch1129: gdb-bare-DW_TAG_lexical_block-2of2.patch

# Fix 'info type-printers' Python error (Clem Dickey, RH BZ 1350436).
Patch992: gdb-rhbz1350436-type-printers-error.patch

# [aarch64] Fix ARMv8.1/v8.2 for hw watchpoint and breakpoint
# (Andrew Pinski, RH BZ 1363635).
Patch1141: gdb-rhbz1363635-aarch64-armv8182.patch

Patch9999: gdb-pip.patch

%if 0%{!?rhel:1} || 0%{?rhel} > 6
# RL_STATE_FEDORA_GDB would not be found for:
# Patch642: gdb-readline62-ask-more-rh.patch
# --with-system-readline
BuildRequires: readline-devel%{buildisa} >= 6.2-4
%endif # 0%{!?rhel:1} || 0%{?rhel} > 6

BuildRequires: ncurses-devel%{buildisa} texinfo gettext flex bison
BuildRequires: expat-devel%{buildisa}
%if 0%{!?rhel:1} || 0%{?rhel} > 6
BuildRequires: xz-devel%{buildisa}
%endif
%if 0%{!?el5:1}
# dlopen() no longer makes rpm-libs%{buildisa} (it's .so) a mandatory dependency.
BuildRequires: rpm-devel%{buildisa}
%endif # 0%{!?el5:1}
BuildRequires: zlib-devel%{buildisa} libselinux-devel%{buildisa}
%if 0%{!?_without_python:1}
%if 0%{?el5:1}
# This RHEL-5.6 python version got split out python-libs for ppc64.
# RHEL-5 rpm does not support .%{_arch} dependencies.
Requires: python-libs-%{_arch} >= 2.4.3-32.el5
%endif
BuildRequires: python-devel%{buildisa}
%if 0%{?rhel:1} && 0%{?rhel} <= 6
# Temporarily before python files get moved to libstdc++.rpm
# libstdc++%{bits_other} is not present in Koji, the .spec script generating
# gdb/python/libstdcxx/ also does not depend on the %{bits_other} files.
BuildRequires: libstdc++%{buildisa}
%endif # 0%{?rhel:1} && 0%{?rhel} <= 6
%endif # 0%{!?_without_python:1}
# gdb-doc in PDF:
BuildRequires: texinfo-tex
# Permit rebuilding *.[0-9] files even if they are distributed in gdb-*.tar:
BuildRequires: /usr/bin/pod2man
# PDF doc workaround, see: # https://bugzilla.redhat.com/show_bug.cgi?id=919891
%if 0%{!?rhel:1} || 0%{?rhel} > 6
BuildRequires: texlive-ec texlive-cm-super
%endif

# BuildArch would break RHEL-5 by overriding arch and not building noarch.
%if 0%{?el5:1}
ExclusiveArch: noarch i386 x86_64 ppc ppc64 ia64 s390 s390x
%endif # 0%{?el5:1}

%if 0%{?_with_testsuite:1}

# Ensure the devel libraries are installed for both multilib arches.
%global bits_local %{?_isa}
%global bits_other %{?_isa}
%if 0%{!?el5:1}
%ifarch s390x
%global bits_other (%{__isa_name}-32)
%else #!s390x
%ifarch ppc
%global bits_other (%{__isa_name}-64)
%else #!ppc
%ifarch sparc64 ppc64 s390x x86_64
%global bits_other (%{__isa_name}-32)
%endif #sparc64 ppc64 s390x x86_64
%endif #!ppc
%endif #!s390x
%endif #!el5

BuildRequires: sharutils dejagnu
# gcc-objc++ is not covered by the GDB testsuite.
BuildRequires: gcc gcc-c++ gcc-gfortran gcc-objc
%if 0%{!?rhel:1} || 0%{?rhel} < 7
BuildRequires: gcc-java libgcj%{bits_local} libgcj%{bits_other}
%endif
%if 0%{!?rhel:1} || 0%{?rhel} > 6
# These Fedoras do not yet have gcc-go built.
%ifnarch ppc64le aarch64
BuildRequires: gcc-go
%endif
%endif
# archer-sergiodj-stap-patch-split
BuildRequires: systemtap-sdt-devel
# Copied from prelink-0.4.2-3.fc13.
%ifarch %{ix86} alpha sparc sparcv9 sparc64 s390 s390x x86_64 ppc ppc64
# Prelink is broken on sparcv9/sparc64.
%ifnarch sparc sparcv9 sparc64
BuildRequires: prelink
%endif
%endif
%if 0%{!?rhel:1}
# Fedora arm does not yet have fpc built.
%ifnarch %{arm}
BuildRequires: fpc
%endif
%endif
%if 0%{?el5:1}
BuildRequires: gcc44 gcc44-gfortran
%endif
# Copied from gcc-4.1.2-32.
%ifarch %{ix86} x86_64 ia64 ppc alpha
BuildRequires: gcc-gnat
BuildRequires: libgnat%{bits_local} libgnat%{bits_other}
%endif
BuildRequires: glibc-devel%{bits_local} glibc-devel%{bits_other}
BuildRequires: libgcc%{bits_local} libgcc%{bits_other}
# libstdc++-devel of matching bits is required only for g++ -static.
BuildRequires: libstdc++%{bits_local} libstdc++%{bits_other}
%if 0%{!?rhel:1} || 0%{?rhel} > 6
# These Fedoras do not yet have gcc-go built.
%ifnarch ppc64le aarch64
BuildRequires: libgo-devel%{bits_local} libgo-devel%{bits_other}
%endif
%endif
%if 0%{!?el5:1}
BuildRequires: glibc-static%{bits_local}
%endif
# multilib glibc-static is open Bug 488472:
#BuildRequires: glibc-static%{bits_other}
# for gcc-java linkage:
BuildRequires: zlib-devel%{bits_local} zlib-devel%{bits_other}
# Copied from valgrind-3.5.0-1.
%ifarch %{ix86} x86_64 ppc ppc64
BuildRequires: valgrind%{bits_local} valgrind%{bits_other}
%endif
%if 0%{!?rhel:1} || 0%{?rhel} > 6
BuildRequires: xz
%endif

%endif # 0%{?_with_testsuite:1}

%ifarch ia64
%if 0%{!?el5:1}
BuildRequires: libunwind-devel >= 0.99-0.1.frysk20070405cvs
Requires: libunwind >= 0.99-0.1.frysk20070405cvs
%else
BuildRequires: libunwind >= 0.96-3
Requires: libunwind >= 0.96-3
%endif
%endif

%{?scl:Requires:%scl_runtime}

%description
PiP-GDB, the GNU debugger, allows you to debug programs written in C, C++,
Java, and other languages, by executing them in a controlled fashion
and printing their data.

# It would break RHEL-5 by leaving excessive files for the doc subpackage.
%ifnarch noarch

%package gdbserver
Summary: A standalone server for GDB (the GNU source-level debugger)
Group: Development/Debuggers

%if "%{scl}" == "devtoolset-1.1"
Obsoletes: devtoolset-1.0-%{pkg_name}-gdbserver
%endif

%description gdbserver
GDB, the GNU debugger, allows you to debug programs written in C, C++,
Java, and other languages, by executing them in a controlled fashion
and printing their data.

This package provides a program that allows you to run GDB on a different
machine than the one which is running the program being debugged.

# It would break RHEL-5 by leaving excessive files for the doc subpackage.
%endif # !noarch
%if 0%{!?el5:1} || "%{_target_cpu}" == "noarch"

%package doc
Summary: Documentation for GDB (the GNU source-level debugger)
License: GFDL
Group: Documentation
# It would break RHEL-5 by overriding arch and not building noarch separately.
%if 0%{!?el5:1}
BuildArch: noarch
%endif # 0%{!?el5:1}

%if "%{scl}" == "devtoolset-1.1"
Obsoletes: devtoolset-1.0-%{pkg_name}-doc
%endif

%description doc
GDB, the GNU debugger, allows you to debug programs written in C, C++,
Java, and other languages, by executing them in a controlled fashion
and printing their data.

This package provides INFO, HTML and PDF user manual for GDB.

Requires(post): /sbin/install-info
Requires(preun): /sbin/install-info

%endif # 0%{!?el5:1} || "%{_target_cpu}" == "noarch"

%prep
%setup -q -n %{gdb_src}

%if 0%{?rhel:1} && 0%{?rhel} <= 6
# libstdc++ pretty printers.
tar xjf %{SOURCE5}
%endif # 0%{?rhel:1} && 0%{?rhel} <= 6

# Files have `# <number> <file>' statements breaking VPATH / find-debuginfo.sh .
rm -f gdb/ada-exp.c gdb/ada-lex.c gdb/c-exp.c gdb/cp-name-parser.c gdb/f-exp.c
rm -f gdb/jv-exp.c gdb/m2-exp.c gdb/objc-exp.c gdb/p-exp.c gdb/go-exp.c

# *.info* is needlessly split in the distro tar; also it would not get used as
# we build in %{gdb_build}, just to be sure.
find -name "*.info*"|xargs rm -f

# Apply patches defined above.

# Match the Fedora's version info.
%patch2 -p1

%patch349 -p1
%patch232 -p1
%patch828 -p1
%patch829 -p1
%patch1 -p1
%patch3 -p1

%patch105 -p1
%patch111 -p1
%patch112 -p1
%patch118 -p1
%patch122 -p1
%patch125 -p1
%patch133 -p1
%patch136 -p1
%patch140 -p1
%patch145 -p1
%patch153 -p1
%patch158 -p1
%patch160 -p1
%patch161 -p1
%patch162 -p1
%patch163 -p1
%patch164 -p1
%patch169 -p1
%patch188 -p1
%patch194 -p1
%patch196 -p1
%patch199 -p1
%patch208 -p1
%patch211 -p1
%patch213 -p1
%patch214 -p1
%patch216 -p1
%patch217 -p1
%patch225 -p1
%patch229 -p1
%patch231 -p1
%patch234 -p1
%patch235 -p1
%patch245 -p1
%patch247 -p1
%patch254 -p1
%patch258 -p1
%patch260 -p1
%patch263 -p1
%patch271 -p1
%patch274 -p1
%patch659 -p1
%patch353 -p1
%patch276 -p1
%patch282 -p1
%patch284 -p1
%patch287 -p1
%patch289 -p1
%patch290 -p1
%patch294 -p1
%patch296 -p1
%patch298 -p1
%patch309 -p1
%patch311 -p1
%patch315 -p1
%patch317 -p1
%patch320 -p1
%patch330 -p1
%patch343 -p1
%patch348 -p1
%patch360 -p1
%patch381 -p1
%patch382 -p1
%patch391 -p1
%patch392 -p1
%patch397 -p1
%patch403 -p1
%patch389 -p1
%patch394 -p1
%patch407 -p1
%patch408 -p1
%patch417 -p1
%patch459 -p1
%patch470 -p1
%patch475 -p1
%patch415 -p1
%patch519 -p1
%patch490 -p1
%patch491 -p1
%patch496 -p1
%patch526 -p1
%patch542 -p1
%patch547 -p1
%patch548 -p1
%patch579 -p1
%patch718 -p1
%patch719 -p1
%patch720 -p1
%patch721 -p1
%patch722 -p1
%patch723 -p1
%patch822 -p1
%patch827 -p1
%patch619 -p1
%patch627 -p1
%patch634 -p1
%patch653 -p1
%patch657 -p1
%patch661 -p1
%patch690 -p1
%patch698 -p1
%patch703 -p1
%patch811 -p1
%patch812 -p1
%patch813 -p1
%patch814 -p1
%patch816 -p1
%patch817 -p1
%patch818 -p1
%patch832 -p1
%patch834 -p1
%patch835 -p1
%patch840 -p1
%patch841 -p1
%patch843 -p1
%patch844 -p1
%patch845 -p1
%patch850 -p1
%patch851 -p1
%patch852 -p1
%patch928 -p1
%patch929 -p1
%patch930 -p1
%patch931 -p1
%patch932 -p1
%patch933 -p1
%patch934 -p1
%patch935 -p1
%patch936 -p1
%patch937 -p1
%patch938 -p1
%patch939 -p1
%patch940 -p1
%patch941 -p1
%patch942 -p1
%patch943 -p1
%patch944 -p1
%patch945 -p1
%patch946 -p1
%patch947 -p1
%patch948 -p1
%patch949 -p1
%patch950 -p1
%patch951 -p1
%patch952 -p1
%patch953 -p1
%patch954 -p1
%patch955 -p1
%patch956 -p1
%patch957 -p1
%patch958 -p1
%patch959 -p1
%patch960 -p1
%patch961 -p1
%patch962 -p1
%patch969 -p1
%patch974 -p1
%patch963 -p1
%patch964 -p1
%patch965 -p1
%patch966 -p1
%patch967 -p1
%patch968 -p1
%patch972 -p1
%patch976 -p1
%patch992 -p1
%patch994 -p1
%patch995 -p1
%patch996 -p1
%patch997 -p1

%patch836 -p1
%patch837 -p1
%patch998 -p1

%patch999 -p1
%patch1000 -p1
%patch1001 -p1
%patch1002 -p1
%patch1003 -p1
%patch1004 -p1
%patch1005 -p1
%patch1006 -p1
%patch1007 -p1
%patch1008 -p1
%patch1009 -p1
%patch1010 -p1
%patch1011 -p1
%patch1012 -p1
%patch1013 -p1
%patch1014 -p1

%patch1015 -p1
%patch1016 -p1
%patch1017 -p1
%patch1018 -p1
%patch1019 -p1
%patch1020 -p1
%patch1021 -p1
%patch1022 -p1
%patch1023 -p1
%patch1024 -p1
%patch1025 -p1
%patch1026 -p1
%patch1027 -p1
%patch1028 -p1
%patch1030 -p1
%patch1047 -p1
%patch1048 -p1
%patch1055 -p1
%patch1057 -p1
%patch1071 -p1
%patch1072 -p1
%patch1073 -p1
%patch1108 -p1
%patch1109 -p1
%patch1110 -p1
%patch1111 -p1
%patch1112 -p1
%patch1113 -p1
%patch1080 -p1
%patch1081 -p1
%patch1082 -p1
%patch1083 -p1
%patch1084 -p1
%patch1085 -p1
%patch1086 -p1
%patch1087 -p1
%patch1088 -p1
%patch1089 -p1
%patch1090 -p1
%patch1091 -p1
%patch1092 -p1
%patch1093 -p1
%patch1094 -p1
%patch1095 -p1
%patch1096 -p1
%patch1097 -p1
%patch1098 -p1
%patch1099 -p1
%patch1100 -p1
%patch1101 -p1
%patch1121 -p1
%patch1122 -p1
%patch1123 -p1
%patch1053 -p1
%patch1103 -p1
%patch1104 -p1
%patch1105 -p1
%patch1106 -p1
%patch1114 -p1
%patch1115 -p1
%patch1116 -p1
%patch1118 -p1
%patch1128 -p1
%patch1129 -p1
%patch1141 -p1

%if 0%{?scl:1}
%patch836 -p1 -R
%patch837 -p1 -R
%patch998 -p1 -R
%endif
%patch393 -p1
%if 0%{!?el5:1} || 0%{?scl:1}
%patch393 -p1 -R
%endif
%patch833 -p1
%if 0%{!?el6:1} || 0%{!?scl:1}
%patch833 -p1 -R
%endif
%patch642 -p1
%if 0%{?rhel:1} && 0%{?rhel} <= 6
%patch642 -p1 -R
%patch337 -p1
%patch331 -p1
%patch335 -p1
%endif

%patch9999 -p1

find -name "*.orig" | xargs rm -f
! find -name "*.rej" # Should not happen.

%build
rm -rf %{buildroot}

# RL_STATE_FEDORA_GDB would not be found for:
# Patch642: gdb-readline62-ask-more-rh.patch
# --with-system-readline
rm -rf readline/*

DESTDIR="$RPM_BUILD_ROOT" sh ./build.sh -b \
	--prefix=%{_prefix} \
	--with-glibc-libdir=%{pip_glibc_libdir} \
	--with-pip=%{_prefix}

%install

mkdir -p "$RPM_BUILD_ROOT"/%{_bindir}
cp gdb/gdb "$RPM_BUILD_ROOT"/%{_bindir}/pip-gdb

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root)
%{_bindir}/pip-gdb
