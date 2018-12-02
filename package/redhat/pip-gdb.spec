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
 %global pkg_name %{name}
 %global _root_prefix %{_prefix}
 %global _root_datadir %{_datadir}
 %global _root_bindir %{_bindir}
 %global _root_libdir %{_libdir}
}

Summary: A GNU source-level debugger for C, C++, Fortran, Go and other languages
Name: %{?scl_prefix}gdb

#global snap       20130423
# Freeze it when GDB gets branched
%global snapsrc    20130312
# See timestamp of source gnulib installed into gdb/gnulib/ .
%global snapgnulib 20121213
Version: 7.6.1

# The release always contains a leading reserved number, start it at 1.
# `upstream' is not a part of `name' to stay fully rpm dependencies compatible for the testing.
Release: 94%{?dist}

License: GPLv3+ and GPLv3+ with exceptions and GPLv2+ and GPLv2+ with exceptions and GPL+ and LGPLv2+ and BSD and Public Domain
Group: Development/Debuggers
# Do not provide URL for snapshots as the file lasts there only for 2 days.
# ftp://sourceware.org/pub/gdb/releases/gdb-%{version}.tar.bz2
Source: ftp://sourceware.org/pub/gdb/releases/gdb-%{version}.tar.bz2
Buildroot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
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
GDB, the GNU debugger, allows you to debug programs written in C, C++,
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

find -name "*.orig" | xargs rm -f
! find -name "*.rej" # Should not happen.

# Change the version that gets printed at GDB startup, so it is RH specific.
cat > gdb/version.in << _FOO
%if 0%{!?rhel:1}
Fedora %{version}-%{release}
%else # !0%{!?rhel:1} 
Red Hat Enterprise Linux %{version}-%{release}
%endif # !0%{!?rhel:1} 
_FOO

# Remove the info and other generated files added by the FSF release
# process.
rm -f libdecnumber/gstdint.h
rm -f bfd/doc/*.info
rm -f bfd/doc/*.info-*
rm -f gdb/doc/*.info
rm -f gdb/doc/*.info-*

%if 0%{!?rhel:1} || 0%{?rhel} > 6
# RL_STATE_FEDORA_GDB would not be found for:
# Patch642: gdb-readline62-ask-more-rh.patch
# --with-system-readline
mv -f readline/doc readline-doc
rm -rf readline/*
mv -f readline-doc readline/doc
%endif # 0%{!?rhel:1} || 0%{?rhel} > 6

%build
rm -rf %{buildroot}

test -e %{_root_libdir}/librpm.so.%{librpmver}

# Identify the build directory with the version of gdb as well as the
# architecture, to allow for mutliple versions to be installed and
# built.
# Initially we're in the %{gdb_src} directory.

for fprofile in %{?_with_profile:-fprofile} ""
do

mkdir %{gdb_build}$fprofile
cd %{gdb_build}$fprofile

export CFLAGS="$RPM_OPT_FLAGS"
export LDFLAGS="%{?__global_ldflags}"

../configure							\
	--prefix=%{_prefix}					\
	--libdir=%{_libdir}					\
	--sysconfdir=%{_sysconfdir}				\
	--mandir=%{_mandir}					\
	--infodir=%{_infodir}					\
	--htmldir=%{gdb_docdir}					\
	--pdfdir=%{gdb_docdir}					\
	--with-system-gdbinit=%{_sysconfdir}/gdbinit		\
	--with-gdb-datadir=%{_datadir}/gdb			\
	--enable-gdb-build-warnings=,-Wno-unused		\
%ifnarch %{ix86} alpha ia64 ppc s390 s390x x86_64 ppc64 ppc64le sparc sparcv9 sparc64
	--disable-werror					\
%else
	--enable-werror						\
%endif
	--with-separate-debug-dir=/usr/lib/debug		\
	--disable-sim						\
	--disable-rpath						\
%if 0%{!?rhel:1} || 0%{?rhel} > 6
	--with-system-readline					\
%else
	--without-system-readline				\
%endif
	--with-expat						\
$(: ppc64 host build crashes on ppc variant of libexpat.so )	\
	--without-libexpat-prefix				\
	--enable-tui						\
%if 0%{!?_without_python:1}
	--with-python						\
%else
	--without-python					\
%endif
$(: Workaround rpm.org#76, BZ 508193 on recent OSes. )		\
$(: RHEL-5 librpm has incompatible API. )			\
%if 0%{?el5:1}
	--without-rpm						\
%else
%if 0%{?el6:1}
	--with-rpm=librpm.so.1					\
%else
	--with-rpm=librpm.so.3					\
%endif
%endif
%if 0%{!?rhel:1} || 0%{?rhel} > 6
	--with-lzma						\
%else
	--without-lzma						\
%endif
%ifarch ia64
	--with-libunwind					\
%else
	--without-libunwind					\
%endif
%ifarch sparc sparcv9 sparc64
	--without-mmap						\
%endif
	--enable-64-bit-bfd					\
%if %{have_inproctrace}
	--enable-inprocess-agent				\
%else
	--disable-inprocess-agent				\
%endif
$(: %{_bindir}/mono-gdb.py is workaround for mono BZ 815501. )										\
	      --with-auto-load-dir='$debugdir:$datadir/auto-load%{?scl::%{_root_datadir}/gdb/auto-load}'				\
	--with-auto-load-safe-path='$debugdir:$datadir/auto-load%{?scl::%{_root_datadir}/gdb/auto-load}:%{_root_bindir}/mono-gdb.py'	\
%ifarch sparc sparcv9
	sparc-%{_vendor}-%{_target_os}%{?_gnu}
%else
$(: It breaks RHEL-5 by %{_target_platform} being noarch-redhat-linux-gnu ) \
%ifarch noarch
	$(:)
%else
	--enable-targets=s390-linux-gnu,powerpc-linux-gnu,powerpcle-linux-gnu	\
	%{_target_platform}
%endif
%endif

if [ -z "%{!?_with_profile:no}" ]
then
  # Run all the configure tests being incompatible with $FPROFILE_CFLAGS.
  make %{?_smp_mflags} configure-host configure-target
  make %{?_smp_mflags} clean

  # Workaround -fprofile-use:
  # linux-x86-low.c:2225: Error: symbol `start_i386_goto' is already defined
  make %{?_smp_mflags} -C gdb/gdbserver linux-x86-low.o
fi

# Global CFLAGS would fail on:
# conftest.c:1:1: error: coverage mismatch for function 'main' while reading counter 'arcs'
if [ "$fprofile" = "-fprofile" ]
then
  FPROFILE_CFLAGS='-fprofile-generate'
elif [ -z "%{!?_with_profile:no}" ]
then
  FPROFILE_CFLAGS='-fprofile-use'
  # We cannot use -fprofile-dir as the bare filenames clash.
  (cd ../%{gdb_build}-fprofile;
   # It was 333 on x86_64.
   test $(find -name "*.gcda"|wc -l) -gt 300
   find -name "*.gcda" | while read -r i
   do
     ln $i ../%{gdb_build}/$i
   done
  )
else
  FPROFILE_CFLAGS=""
fi

# Prepare gdb/config.h first.
make %{?_smp_mflags} CFLAGS="$CFLAGS $FPROFILE_CFLAGS" LDFLAGS="$LDFLAGS $FPROFILE_CFLAGS" maybe-configure-gdb
perl -i.relocatable -pe 's/^(D\[".*_RELOCATABLE"\]=" )1(")$/${1}0$2/' gdb/config.status

make %{?_smp_mflags} CFLAGS="$CFLAGS $FPROFILE_CFLAGS" LDFLAGS="$LDFLAGS $FPROFILE_CFLAGS"

! grep '_RELOCATABLE.*1' gdb/config.h
grep '^#define HAVE_LIBSELINUX 1$' gdb/config.h
grep '^#define HAVE_SELINUX_SELINUX_H 1$' gdb/config.h

if [ "$fprofile" = "-fprofile" ]
then
  cd gdb
  cp -p gdb gdb-withindex
  PATH="$PWD:$PATH" sh ../../gdb/gdb-add-index $PWD/gdb-withindex
  ./gdb -nx -ex q ./gdb-withindex
  ./gdb -nx -readnow -ex q ./gdb-withindex
  cd ..
fi

cd ..

done	# fprofile

cd %{gdb_build}

make \
$(: There was a race on RHEL-5: ) \
$(: fmtutil: format directory '/builddir/.texmf-var/web2c' does not exist. ) \
%if 0%{?el5:1}
     -j1 \
%else
     %{?_smp_mflags} \
%endif
     -C gdb/doc {gdb,annotate}{.info,/index.html,.pdf} MAKEHTMLFLAGS=--no-split MAKEINFOFLAGS=--no-split

grep '#define HAVE_ZLIB_H 1' gdb/config.h

# Copy the <sourcetree>/gdb/NEWS file to the directory above it.
cp $RPM_BUILD_DIR/%{gdb_src}/gdb/NEWS $RPM_BUILD_DIR/%{gdb_src}

%check
# Initially we're in the %{gdb_src} directory.
cd %{gdb_build}

%if 0%{!?_with_testsuite:1}
echo ====================TESTSUITE DISABLED=========================
%else
echo ====================TESTING=========================
cd gdb
gcc -o ./orphanripper %{SOURCE2} -Wall -lutil -ggdb2
# Need to use a single --ignore option, second use overrides first.
# No `%{?_smp_mflags}' here as it may race.
# WARNING: can't generate a core file - core tests suppressed - check ulimit
# "readline-overflow.exp" - Testcase is broken, functionality is OK.
(
  # ULIMIT required for `gdb.base/auxv.exp'.
  ulimit -H -c
  ulimit -c unlimited || :

  # Setup $CHECK as `check//unix/' or `check//unix/-m64' for explicit bitsize.
  # Never use two different bitsizes as it fails on ppc64.
  echo 'int main (void) { return 0; }' >biarch.c
  CHECK=""
  for BI in -m64 -m32 -m31 ""
  do
    # Do not use size-less options if any of the sizes works.
    # On ia64 there is no -m64 flag while we must not leave a bare `check' here
    # as it would switch over some testing scripts to the backward compatibility
    # mode: when `make check' was executed from inside the testsuite/ directory.
    if [ -z "$BI" -a -n "$CHECK" ];then
      continue
    fi
    # Do not use $RPM_OPT_FLAGS as the other non-size options will not be used
    # in the real run of the testsuite.
    if ! gcc $BI -o biarch biarch.c
    then
      continue
    fi
    CHECK="$CHECK check//unix/$BI"
  done
  # Do not try -m64 inferiors for -m32 GDB as it cannot handle inferiors larger
  # than itself.
  # s390 -m31 still uses the standard ELF32 binary format.
  gcc $RPM_OPT_FLAGS -o biarch biarch.c
  RPM_SIZE="$(file ./biarch|sed -n 's/^.*: ELF \(32\|64\)-bit .*$/\1/p')"
  if [ "$RPM_SIZE" != "64" ]
  then
    CHECK="$(echo " $CHECK "|sed 's# check//unix/-m64 # #')"
  fi

  # Disable some problematic testcases.
  # RUNTESTFLAGS='--ignore ...' is not used below as it gets separated by the
  # `check//...' target spawn and too much escaping there would be dense.
  for test in				\
    gdb.base/readline-overflow.exp	\
    gdb.base/bigcore.exp		\
  ; do
    mv -f ../../gdb/testsuite/$test ../gdb/testsuite/$test-DISABLED || :
  done

  # Run all the scheduled testsuite runs also in the PIE mode.
  # See also: gdb-runtest-pie-override.exp
  CHECK="$(echo $CHECK|sed 's#check//unix/[^ ]*#& &/-fPIC/-pie#g')"

  ./orphanripper make %{?_smp_mflags} -k $CHECK || :
)
for t in sum log
do
  for file in testsuite*/gdb.$t
  do
    suffix="${file#testsuite.unix.}"
    suffix="${suffix%/gdb.$t}"
    ln $file gdb-%{_target_platform}$suffix.$t || :
  done
done
# `tar | bzip2 | uuencode' may have some piping problems in Brew.
tar cjf gdb-%{_target_platform}.tar.bz2 gdb-%{_target_platform}*.{sum,log}
uuencode gdb-%{_target_platform}.tar.bz2 gdb-%{_target_platform}.tar.bz2
cd ../..
echo ====================TESTING END=====================
%endif

%install
# Initially we're in the %{gdb_src} directory.
cd %{gdb_build}
rm -rf $RPM_BUILD_ROOT

# It would break RHEL-5 by leaving excessive files for the doc subpackage.
%ifnarch noarch

make %{?_smp_mflags} install DESTDIR=$RPM_BUILD_ROOT

# Provide gdbtui for RHEL-5 and RHEL-6 as it is removed upstream (BZ 797664).
%if 0%{?rhel:1} && 0%{?rhel} <= 6
test ! -e $RPM_BUILD_ROOT%{_prefix}/bin/gdbtui
install -m 755 %{SOURCE6} $RPM_BUILD_ROOT%{_prefix}/bin/gdbtui
ln -sf gdb.1 $RPM_BUILD_ROOT%{_mandir}/man1/gdbtui.1
%endif # 0%{?rhel:1} && 0%{?rhel} <= 6

mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/gdbinit.d
touch -r %{SOURCE4} $RPM_BUILD_ROOT%{_sysconfdir}/gdbinit.d
sed 's#%%{_sysconfdir}#%{_sysconfdir}#g' <%{SOURCE4} >$RPM_BUILD_ROOT%{_sysconfdir}/gdbinit
touch -r %{SOURCE4} $RPM_BUILD_ROOT%{_sysconfdir}/gdbinit

for i in `find $RPM_BUILD_ROOT%{_datadir}/gdb/python/gdb -name "*.py"`
do
  # Files could be also patched getting the current time.
  touch -r $RPM_BUILD_DIR/%{gdb_src}/gdb/ChangeLog $i
done

%if 0%{?_enable_debug_packages:1} && 0%{!?_without_python:1}
mkdir -p $RPM_BUILD_ROOT/usr/lib/debug%{_bindir}
cp -p $RPM_BUILD_DIR/%{gdb_src}/gdb/gdb-gdb.py $RPM_BUILD_ROOT/usr/lib/debug%{_bindir}/
for pyo in "" "-O";do
  # RHEL-5: AttributeError: 'module' object has no attribute 'compile_file'
  python $pyo -c 'import compileall, re, sys; sys.exit (not compileall.compile_dir("'"$RPM_BUILD_ROOT/usr/lib/debug%{_bindir}"'", 1, "'"/usr/lib/debug%{_bindir}"'"))'
done
%endif # 0%{?_enable_debug_packages:1} && 0%{!?_without_python:1}

mkdir $RPM_BUILD_ROOT%{_datadir}/gdb/auto-load
%if 0%{!?_without_python:1}
%if 0%{?rhel:1} && 0%{?rhel} <= 6
# Temporarily now:
for LIB in lib lib64;do
  LIBPATH="$RPM_BUILD_ROOT%{_datadir}/gdb/auto-load%{_root_prefix}/$LIB"
  mkdir -p $LIBPATH
  # basename is being run only for the native (non-biarch) file.
  sed -e 's,@pythondir@,%{_datadir}/gdb/python,'		\
      -e 's,@toolexeclibdir@,%{_root_prefix}/'"$LIB,"		\
      < $RPM_BUILD_DIR/%{gdb_src}/%{libstdcxxpython}/hook.in	\
      > $LIBPATH/$(basename %{_root_prefix}/%{_lib}/libstdc++.so.6.*)-gdb.py
  # Test the filename 'libstdc++.so.6.*' has matched.
  test -f $LIBPATH/libstdc++.so.6.[0-9]*-gdb.py
done
test ! -e $RPM_BUILD_ROOT%{_datadir}/gdb/python/libstdcxx
cp -a $RPM_BUILD_DIR/%{gdb_src}/%{libstdcxxpython}/libstdcxx	\
      $RPM_BUILD_ROOT%{_datadir}/gdb/python/libstdcxx
for i in `find $RPM_BUILD_ROOT%{_datadir}/gdb/python -name "*.py"` \
         `find $RPM_BUILD_ROOT%{_datadir}/gdb/auto-load%{_prefix} -name "*.py"` \
; do
  # Files come from gdb-archer.patch and can be also further patched.
  touch -r $RPM_BUILD_DIR/%{gdb_src}/gdb/ChangeLog $i
done
%else # 0%{!?rhel:1} || 0%{?rhel} > 6
# BZ 999645: /usr/share/gdb/auto-load/ needs filesystem symlinks
mkdir -p $RPM_BUILD_ROOT%{_datadir}/gdb/auto-load
for i in $(echo bin lib $(basename %{_libdir}) sbin|tr ' ' '\n'|sort -u);do
  ln -s $(echo %{_root_prefix}|sed 's#^/*##')/$i \
        $RPM_BUILD_ROOT%{_datadir}/gdb/auto-load/$i
done
%endif # 0%{!?rhel:1} || 0%{?rhel} > 6
%endif # 0%{!?_without_python:1}

# gdb-add-index cannot be run even for SCL package on RHEL<=6.
%if 0%{?rhel:1} && 0%{?rhel} <= 6
rm -f $RPM_BUILD_ROOT%{_bindir}/gdb-add-index
%endif

# Remove the files that are part of a gdb build but that are owned and
# provided by other packages.
# These are part of binutils

rm -rf $RPM_BUILD_ROOT%{_datadir}/locale/
rm -f $RPM_BUILD_ROOT%{_infodir}/bfd*
rm -f $RPM_BUILD_ROOT%{_infodir}/standard*
rm -f $RPM_BUILD_ROOT%{_infodir}/configure*
rm -rf $RPM_BUILD_ROOT%{_includedir}
rm -rf $RPM_BUILD_ROOT/%{_libdir}/lib{bfd*,opcodes*,iberty*}

# pstack obsoletion

cp -p %{SOURCE3} $RPM_BUILD_ROOT%{_mandir}/man1/gstack.1
ln -s gstack.1 $RPM_BUILD_ROOT%{_mandir}/man1/pstack.1
ln -s gstack $RPM_BUILD_ROOT%{_bindir}/pstack

# Packaged GDB is not a cross-target one.
(cd $RPM_BUILD_ROOT%{_datadir}/gdb/syscalls
 rm -f mips*.xml
%ifnarch sparc sparcv9 sparc64
 rm -f sparc*.xml
%endif
%ifnarch x86_64
 rm -f amd64-linux.xml
%endif
%ifnarch %{ix86} x86_64
 rm -f i386-linux.xml
%endif
%ifnarch ppc ppc64
 rm -f ppc{,64}-linux.xml
%endif
)

# It would break RHEL-5 by leaving excessive files for the doc subpackage.
%if 0%{?el5:1}
rm -f $RPM_BUILD_ROOT%{_infodir}/annotate.info*
rm -f $RPM_BUILD_ROOT%{_infodir}/gdb.info*
%endif # 0%{?el5:1} 
%else # noarch
# -j1: There is some race resulting in:
# /usr/bin/texi2dvi: texinfo.tex appears to be broken, quitting.
make -j1 -C gdb/doc install DESTDIR=$RPM_BUILD_ROOT
rm -rf $RPM_BUILD_ROOT%{_mandir}
%endif # noarch

# Documentation only for development; keep 'rm's here after "noarch" above.
rm -f $RPM_BUILD_ROOT%{_infodir}/gdbint*
rm -f $RPM_BUILD_ROOT%{_infodir}/stabs*

# Delete this too because the dir file will be updated at rpm install time.
# We don't want a gdb specific one overwriting the system wide one.

rm -f $RPM_BUILD_ROOT%{_infodir}/dir

%clean
rm -rf $RPM_BUILD_ROOT

# It would break RHEL-5 by leaving excessive files for the doc subpackage.
%ifnarch noarch

%files
%defattr(-,root,root)
%doc COPYING3 COPYING COPYING.LIB README NEWS
%{_bindir}/gcore
%{_bindir}/gdb
%config(noreplace) %{_sysconfdir}/gdbinit
%{_sysconfdir}/gdbinit.d
%{_mandir}/*/gdbinit.5*
%{_mandir}/*/gdb.1*
%{_mandir}/*/gcore.1*
%{_mandir}/*/gdb-add-index.1*
%{_bindir}/gstack
%{_mandir}/*/gstack.1*
# Provide gdbtui for RHEL-5 and RHEL-6 as it is removed upstream (BZ 797664).
%if 0%{?rhel:1} && 0%{?rhel} <= 6
%{_bindir}/gdbtui
%{_mandir}/*/gdbtui.1*
%endif # 0%{?rhel:1} && 0%{?rhel} <= 6
# gdb-add-index cannot be run even for SCL package on RHEL<=6.
%if 0%{!?rhel:1} || 0%{?rhel} > 6
%{_bindir}/gdb-add-index
%endif
%{_bindir}/pstack
%{_mandir}/*/pstack.1*
%{_datadir}/gdb

# don't include the files in include, they are part of binutils

%ifnarch sparc sparcv9
%files gdbserver
%defattr(-,root,root)
%{_bindir}/gdbserver
%{_mandir}/*/gdbserver.1*
%if %{have_inproctrace}
%{_libdir}/libinproctrace.so
%endif # %{have_inproctrace}
%endif

%pre
for i in $(echo bin lib $(basename %{_libdir}) sbin|tr ' ' '\n'|sort -u);do
  src="%{_datadir}/gdb/auto-load/$i"
  dst="%{_datadir}/gdb/auto-load/%{_root_prefix}/$i"
  if test -d $src -a ! -L $src;then
    if ! rmdir 2>/dev/null $src;then
      mv -n $src/* $dst/
      rmdir $src
    fi
  fi
done

# It would break RHEL-5 by leaving excessive files for the doc subpackage.
%endif # !noarch
%if 0%{!?el5:1} || "%{_target_cpu}" == "noarch"

%files doc
%doc %{gdb_build}/gdb/doc/{gdb,annotate}.{html,pdf}
%defattr(-,root,root)
%{_infodir}/annotate.info*
%{_infodir}/gdb.info*

%post doc
# This step is part of the installation of the RPM. Not to be confused
# with the 'make install ' of the build (rpmbuild) process.

# For --excludedocs:
if [ -e %{_infodir}/gdb.info.gz ]
then
  /sbin/install-info --info-dir=%{_infodir} %{_infodir}/annotate.info.gz || :
  /sbin/install-info --info-dir=%{_infodir} %{_infodir}/gdb.info.gz || :
fi

%preun doc
if [ $1 = 0 ]
then
  # For --excludedocs:
  if [ -e %{_infodir}/gdb.info.gz ]
  then
    /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/annotate.info.gz || :
    /sbin/install-info --delete --info-dir=%{_infodir} %{_infodir}/gdb.info.gz || :
  fi
fi

%endif # 0%{!?el5:1} || "%{_target_cpu}" == "noarch"

%changelog
* Wed Aug  3 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-94.el7
- [aarch64] Fix ARMv8.1/v8.2 for hw watchpoint and breakpoint
  (Andrew Pinski, RH BZ 1363635).

* Mon Jun 27 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-93.el7
- Fix 'info type-printers' Python error (Clem Dickey, RH BZ 1350436).

* Mon May 30 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-92.el7
- Import bare DW_TAG_lexical_block (RH BZ 1325396).

* Tue May  3 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-91.el7
- Fix messages suggesting more recent RHEL gdbserver (RH BZ 1186918).

* Sun May  1 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-90.el7
- Implement gdbserver support for containers (upstream patches by Gary Benson,
  RH BZ 1186918).

* Sat Apr 23 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-89.el7
- Never kill PID on: gdb exec PID (Jan Kratochvil, RH BZ 1326476).

* Fri Apr 22 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-88.el7
- Fix Python "Cannot locate object file for block." (Tom Tromey, RH BZ 1325795).

* Sat Mar 19 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-87.el7
- [s390] Import IBM z13 support (RH BZ 1182151,
  backport fix up by Andreas Arnez, IBM).

* Tue Mar 15 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-86.el7
- [aarch64] Fix hardware watchpoints (RH BZ 1261564).

* Thu Feb 18 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-85.el7
- Add --with buildisa, remove %%{?_isa} from BuildRequires (RH BZ 1286777):
  https://github.com/msimacek/koschei/issues/54

* Sat Jan 16 2016 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-84.el7
- Add MPX instructions decoding (Intel, binutils RH BZ was 1138642).
- Add clflushopt instruction decode (Intel, RH BZ 1262471).

* Fri Nov 27 2015 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-83.el7
- Support for /proc/PID/coredump_filter (Sergio Durigan Junior, RH BZ 1265351).

* Fri Nov 27 2015 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-82.el7
- Fix crash on Python frame filters with unreadable arg (RH BZ 1281351).

* Thu Oct 29 2015 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-81.el7
- Fix DWARF compatibility with icc (RH BZ 1268831).

* Fri Sep 11 2015 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-80.el7
- [testsuite] Fix gcc-go BuildRequires for --with testsuite.

* Thu Sep 10 2015 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-79.el7
- [ppc64le] Use skip_entrypoint for skip_trampoline_code (RH BZ 1260558).

* Wed Aug 19 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-78.el7
- Fix 'Backport upstream GDB AArch64 SDT probe support to RHEL GDB'
  (Sergio Durigan Junior, RH BZ 1247107).

* Tue Jul 07 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-77.el7
- Address issues from Coverity Scan related to '[7.2 FEAT] Enable
  reverse debugging support for PowerPC on GDB' (RH BZ 1218710).

* Wed Jul 01 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-76.el7
- Fix '[ppc64] and [s390x] wrong prologue skip on -O2 -g code' (Jan
  Kratochvil, RH BZ 1084404).

* Wed Jul 01 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-75.el7
- Fix typo/thinko on
  'gdb-rhbz1105165-ibm-tdb-support-system-z-3of9.patch', where the
  type of the 'hwcap' variable in the 's390_core_read_description'
  function should be 'CORE_ADDR', and not 'unsigned log'.  This caused
  a build breakage on s390 (31-bit) (RH BZ 1105165).

* Tue Jun 30 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-74.el7
- Fix '[7.2 FEAT] GDB Transactional Diagnostic Block support on System
  z' (Andreas Arnez, RH BZ 1105165).

* Mon Jun 29 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-73.el7
- Remove wrongly backported patches (5of7, 6of7, 7of7) from RH BZ
  1225569 backport.  Add new patches necessary to fix failures.

* Fri Jun 19 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-72.el7
- Fix 'gdb output a internal-error: Assertion `num_lwps (GET_PID
  (inferior_ptid)) == 1' failed' (Pedro Alves, RH BZ 1184724).

* Fri Jun 19 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-71.el7
- Fix 'gdb invoked oom-killer analyzing a vmcore on aarch64' (Pedro
  Alves, Tom Tromey, RH BZ 1225569).

* Tue May 26 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-70.el7
- Implement '[7.2 FEAT] Enable reverse debugging support for PowerPC
  on GDB' (Wei-cheng Wang, RH BZ 1218710, thanks to Edjunior Machado
  for backporting).

* Mon May 25 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-69.el7
- Fix 'apply_frame_filter() wrong backport' (Tom Tromey, RH BZ
  1197665).

* Fri May 08 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-68.el7
- Fix 'ppc64: segv in `gdb -q
  /lib/modules/3.10.0-215.el7.ppc64/kernel/fs/nfsd/nfsd.ko`' (Jan
  Kratochvil, RH BZ 1190506).

* Thu May 07 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-67.el7
- Fix '`catch syscall' doesn't work for parent after `fork' is called'
  (Philippe Waroquiers, RH BZ 1149207).

* Wed May 06 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-66.el7
- Fix 'gdb internal error' [Threaded program calls clone, which
  crashes GDB] (Pedro Alves, RH BZ 1210889).

* Wed May 06 2015 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-65.el7
- Fix 'gdb/linux-nat.c:1411: internal-error:
  linux_nat_post_attach_wait: Assertion `pid == new_pid' failed.'
  (Pedro Alves, RH BZ 1210135).

* Sat Nov 22 2014 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-64.el7
- Fix '[FJ7.0 Bug]: gdb: fail to refer to errno on a core dump file'
  (RH BZ 1166549, Jan Kratochvil).

* Fri Nov 14 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-63.el7
- Fix '[RFE] please add add-auto-load-scripts-directory command' (RH
  BZ 1163339, Jan Kratochvil).

* Wed Oct 15 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-62.el7
- Include patch to fix internal-error when stack pointer is invalid
  (RH BZ 1125820, Edjunior Barbosa Machado).  Rename PPC64LE patches
  to reflect the addition of the patch.

* Fri Oct 03 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-61.el7
- Fix 'Slow gstack performance' (RH BZ 1103894, Jan Kratochvil).

* Tue Sep 30 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-60.el7
- Adjust gdb-add-index script to work with binaries in the CWD (RH BZ
  1104029).

* Mon Sep 08 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-59.el7
- Adjust previous patch to build on GDB 7.6.1, where we do not have
  multi-arch + multi-target on GDBserver yet (RH BZ 1125820).

* Mon Sep 08 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-58.el7
- Include patch to fix ppc_collect/supply_ptrace_register GDBserver
  functions, related to PPC64LE support (RH BZ 1125820, Edjunior
  Barbosa Machado).  Rename PPC64LE patches to reflect the addition of
  the patch.

* Fri Sep 05 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-57.el7
- Include testcase for 'Global variable nested by another dso shows up
  incorrectly in gdb' (RH BZ 809179).  The fix for this bug is already
  included in the RHEL-7.1 GDB.

* Wed Aug 27 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-56.el7
- Fix '[RHEL7] Can't access TLS variables in statically linked
  binaries' (RH BZ 1080657, Jan Kratochvil).

* Fri Aug 22 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-55.el7
- Fix 'gdb for aarch64 needs the ability to single step through atomic
  sequences' (RH BZ 1093259, Kyle McMartin).

* Thu Aug 21 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-54.el7
- Make sure the testcase from the last fix only runs on AArch64.

* Thu Aug 21 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-53.el7
- Fix 'incomplete backtraces from core dumps from crash in signal
  handler' (RH BZ 1086894, Hui Zhu and Yao Qi).

* Mon Aug 18 2014 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-52.el7
- Port PPC64LE to RHEL-7.1 GDB (RH BZ 1125820, Ulrich Weigand and Alan Modra)

* Mon Feb 24 2014 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-51.el7
- Merge from Fedora 19.

* Mon Feb 24 2014 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-50.fc19
- Fix crash of -readnow /usr/lib/debug/usr/bin/gnatbind.debug (BZ 1069211).

* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 7.6.1-50.el7
- Mass rebuild 2014-01-24

* Thu Jan 23 2014 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-49.fc19
- [s390*,ppc*] Enable secondary targets s390* and ppc* (BZ 1056259).

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 7.6.1-46.el7
- Mass rebuild 2013-12-27

* Wed Dec 25 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-48.fc19
- [aarch64] Backport two breakpoint/watchpoint fixes.

* Mon Nov 18 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-47.fc19
- [rhel7] [--with testsuite] Remove gcc-java&co. BuildRequires.

* Sat Nov  9 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-46.fc19
- Fix explicit Class:: inside class scope (BZ 874817, Keith Seitz).

* Tue Nov  5 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-45.fc19
- [aarch64] Backport two fixes (BZ 1026484).

* Sun Nov  3 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-44.fc19
- Fix %{_bindir}gdb-add-index to also use -iex 'set auto-load no'.

* Wed Oct 30 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-43.fc19
- [rhel5] Fix /etc/gdbinit compatibility with python-2.4.

* Mon Sep 30 2013 Sergio Durigan Junior <sergiodj@redhat.com> - 7.6.1-42.fc19
- Fix the case when GDB leaks memory because value_struct_elt does not call
  check_typedef.  (Doug Evans, BZ 15695, filed as RH BZ 1013453).

* Mon Sep  9 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-41.fc19
- Fix the version string to be GNU standards compliant (BZ 1004949).

* Sat Aug 31 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-40.fc19
- Remove --disablerepo='*' from BZ 554152 as it conflicts with BZ 981154.
- [ppc] Support Power8 CPU (IBM, BZ 731875).
- [rhel5] tps-srpmtest does not set %%{rhel} (BZ 1002198, Miroslav Franc).

* Fri Aug 30 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-39.fc19
- Load /etc/gdbinit.d/*.{gdb,py} files automatically (BZ 981520).

* Fri Aug 30 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6.1-38.fc19
- Rebase to FSF GDB 7.6.1.

* Fri Aug 30 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-37.fc19
- Fix false warnings of new %%pre during future upgrades (BZ 999645).

* Fri Aug 30 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-36.fc19
- New %%pre to fix failed upgrade of the previous commit (BZ 999645).

* Wed Aug 28 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-35.fc19
- Fix /usr/share/gdb/auto-load/ need of filesystem symlinks (BZ 999645).
  It needs: yum remove gdb-heap; yum reinstall gdb; yum install gdb-heap

* Tue Aug  6 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-34.fc19
- Make the previous commit rpm buildable.

* Tue Aug  6 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-33.fc19
- Fix yum install command when the binary RPM is missing (BZ 981154).
- Fix crash on 'enable count' (Simon Marchi, BZ 993118).
- Fix the changlog entry formatting for 6.3.0.0-0.1.

* Mon Jun 10 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-32.fc19
- [scl] Disable Python frame filters on scl.
- Update libraries opening performance fix from upstream.
- Fix C++ lookups performance regression (Doug Evans, BZ 972677).

* Tue May 28 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-31.fc19
- [ppc] Backport hardware watchpoints fix (Edjunior Machado, BZ 967915).

* Tue May 21 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-30.fc19
- Backported Python frame filters (Phil Muldoon).
- Backported breakpoint conditions crash fix (Sergio Durigan Junior).

* Sun May 19 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-29.fc19
- Fix performance regression opening many libraries (Gary Benson, BZ 965106).

* Thu May  9 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-28.fc19
- Fix needless expansion of non-gdbindex symtabs (Doug Evans).

* Mon May  6 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-27.fc19
- [testsuite] [RHEL-5] Fix gdb-orphanripper.c runtime error.

* Fri May  3 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-26.fc19
- Fix gcore for vDSO (on ppc64).

* Sat Apr 27 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-25.fc19
- Fix false "Unknown error 512" on x32 (H.J. Lu, BZ 956883).

* Fri Apr 26 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.6-24.fc19
- Rebase to FSF GDB 7.6.

* Wed Apr 24 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.91.20130423-23.fc19
- Fix man page BuildRequires (for BZ 881892).

* Tue Apr 23 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.91.20130423-22.fc19
- [spec] Fix virtual bundles after GDB has been branched.

* Tue Apr 23 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.91.20130423-21.fc19
- Rebase to FSF GDB 7.5.91.20130423 (pre-7.6 snapshot).

* Mon Apr 22 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.91.20130407-20.fc19
- [SCL] Skip deprecated .gdb_index warning for Red Hat built files (BZ 953585).

* Mon Apr 22 2013 Sergio Durigan Junior <sergiodj@redhat.com> - 7.5.91.20130407-19.fc19
- [RHEL-6] Regression test for RH BZ 947564.

* Thu Apr 11 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.91.20130407-18.fc19
- Provide man page for gcore.1 and gdb-add-index.1 (BZ 881892).

* Sun Apr  7 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.91.20130407-17.fc19
- [RHEL-5] Fix noarch doc build.

* Sun Apr  7 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.91.20130407-16.fc19
- Rebase to FSF GDB 7.5.91.20130407 (pre-7.6 snapshot).
- [SCL] Remove BuildRequires of gcc-go on SCL (Miroslav Franc, BZ 948982).
- Provide man page for gdbinit.5 (BZ 881892), document gdb -p in man (BZ 659000).

* Tue Apr  2 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.91.20130402-15.fc19
- Rebase to FSF GDB 7.5.91.20130402 (pre-7.6 snapshot).

* Sat Mar 23 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.91.20130323-14.fc19
- Rebase to FSF GDB 7.5.91.20130310 (pre-7.6 snapshot).
- Fix crash regression from the dlopen of libpthread.so fix (BZ 911712).

* Mon Mar 11 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130310-13.fc19
- [RHEL-5] Import build regression fix.

* Sun Mar 10 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130310-12.fc19
- Add workaround of PDF gdb-doc build (filed as RH BZ 919891).

* Sun Mar 10 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130310-11.fc19
- Re-enable (again) PDF in gdb-doc after texinfo RH BZ 876710 has been fixed.

* Sun Mar 10 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130310-10.fc19
- Rebase to FSF GDB 7.5.50.20130310 (pre-7.6 snapshot).
- Fix various entry-values sub-optimal results.

* Mon Feb 25 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130215-9.fc19
- testsuite: Fix gdb.arch/powerpc-power6.exp testcase (IBM, RH BZ 890900).

* Tue Feb 19 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130215-8.fc19
- Temporarily disable PDF in gdb-doc for still unavailable texinfo RH BZ 876710.

* Mon Feb 18 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130215-7.fc19
- Rebase to FSF GDB 7.5.50.20130215 (pre-7.6 snapshot).

* Fri Feb  8 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130118-6.fc19
- Re-enable PDF in gdb-doc after texinfo RH BZ 876710 has been fixed.

* Mon Feb  4 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130118-5.fc19
- Release bump only.

* Fri Feb  1 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130118-4.fc19
- Fix assert crashes with minidebuginfo (BZ 903522).

* Fri Jan 25 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130118-3.fc19
- Release bump only.

* Mon Jan 21 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130118-2.fc19
- [RHEL] Reintroduce gdb-6.8-quit-never-aborts.patch.

* Sat Jan 19 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.50.20130118-1.fc19
- Rebase to FSF GDB 7.5.50.20130118 (pre-7.6 snapshot).

* Sun Jan 13 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.1-35.fc18
- [testsuite] Fix gdb-orphanripper.c lockup on F-17 (/dev/pts/* glibc chown).

* Tue Jan  8 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.1-34.fc18
- Update dlopen to support map_failed probe of glibc (Gary Benson, BZ 886516).

* Thu Jan  3 2013 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.1-33.fc18
- [ppc*] Fix PowerPC disassembly regression (Alan Modra, Edjunior Machado).

* Thu Dec 13 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.1-32.fc18
- 'dwz -m' parsing fix (Tom Tromey).

* Mon Dec  3 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.1-31.fc18
- Fix DW_OP_GNU_implicit_pointer offset bug (Tom Tromey).

* Sun Dec  2 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.1-30.fc18
- Temporarily disable PDF in gdb-doc before texinfo RH BZ 876710 gets fixed.

* Thu Nov 29 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.1-29.fc18
- Provide Source URL when it is a release.

* Thu Nov 29 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.1-28.fc18
- Fix (unsplit) split info doc.

* Thu Nov 29 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.1-27.fc18
- Rebase to FSF GDB 7.5.1 (7.5 stable branch).

* Fri Nov  9 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.0.20120926-26.fc18
- Fix `GDB cannot access struct member whose offset is larger than 256MB'
  (RH BZ 871066).

* Fri Oct  5 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.0.20120926-25.fc18
- entry values: Fix resolving in inlined frames.

* Thu Sep 27 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.0.20120926-24.fc18
- Permit passing pointers as address number even for C++ methods (Keith Seitz).

* Thu Sep 27 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.0.20120926-23.fc18
- Fix crash printing classes (BZ 849357, Tom Tromey).

* Wed Sep 26 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.0.20120926-22.fc18
- Fix .spec 'bundled' Provides for the stable branch rebase.

* Wed Sep 26 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5.0.20120926-21.fc18
- [ppc32] Fix stepping over symbol-less code crash regression (BZ 860696).
- Rebase to FSF GDB 7.5.0.20120926 (7.5 stable branch).
  - Remove the .spec Source keyword URL as not valid now.

* Fri Sep 14 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5-20.fc18
- [RHEL-6] Disable no longer valid workaround of man pages .gz suffix.

* Sat Aug 18 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.5-19.fc18
- Rebase to FSF GDB 7.5.
- Update dlopen to support two variants of glibc (Gary Benson, BZ 669432).

* Fri Aug 17 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.91.20120801-18.fc18
- Drop Source URL for snapshots.
- Separate %%{snapgnulib} from %%{snap}.
- Fix %%{libstdcxxpython} to be %%{name}-prefixed.
- Fix debug info for go-exp.y and go-exp.c.
- Include RHEL-5 compatible %%{buildroot} cleanup.
- Use %%__global_ldflags.

* Wed Aug  1 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.91.20120801-17.fc18
- Rebase to FSF GDB 7.4.91.20120801.
- [dwz] Rebase it from FSF GDB HEAD.

* Thu Jul 19 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 7.4.50.20120714-16.fc18
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Mon Jul 16 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120714-15.fc18
- [devtoolset] Include Obsoletes of devtoolset-1.0-* by devtoolset-1.1-*.

* Sun Jul 15 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120714-14.fc18
- Rebase to FSF GDB 7.4.50.20120714.
  - Fix entryval feature crash on some .debug files optimized by dwz (BZ 839596).
- Fix another stale frame_info * (PR 11914, like PR 13866).

* Fri Jul  6 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120703-13.fc18
- [RHEL] Disable MiniDebugInfo F-18 feature on RHEL <= 6 (BZ 834068).

* Fri Jul  6 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120703-12.fc18
- Fix .spec metadata for the MiniDebugInfo F-18 feature (BZ 834068).

* Fri Jul  6 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120703-11.fc18
- [archer-tromey-dwz-multifile-rebase] Fix DWARF files reading (Tom Tromey).

* Fri Jul  6 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120703-10.fc18
- Fix build-id-core-loads internal error (BZ 837870).

* Thu Jul  5 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120703-9.fc18
- Implement MiniDebugInfo F-18 Feature consumer (Alexander Larsson, BZ 834068).

* Tue Jul  3 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120703-8.fc18
- Rebase to FSF GDB 7.4.50.20120703.
- [archer-tromey-dwz-multifile-rebase] Merge new branch (Tom Tromey).
- [arm] <--with testsuite>: Disable fpc BuildRequires as it is not yet built.
- Revert function returning pointer fix (PR 9514) regressing Fedora errno patch.

* Thu Jun 21 2012 Sergio Durigan Junior <sergiodj@redhat.com> - 7.4.50.20120603-7.fc18
- Include testcase for BZ 818343.

* Tue Jun 19 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120603-6.fc18
- Fix regression of undisplayed missing shared libraries caused by a fix for:
  GNU/Linux core open: Can't read pathname for load map: Input/output error.

* Sun Jun 17 2012 Sergio Durigan Junior <sergiodj@redhat.com> - 7.4.50.20120603-5.fc18
- Include testcase for BZ 823789.

* Thu Jun 14 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120603-4.fc18
- Support DW_OP_GNU_parameter_ref for -O2 -g inferiors (BZ 827375).

* Wed Jun  6 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120603-3.fc18
- Disable -lmcheck in the development builds.
- Fix assertion on some files as glibc-2.15.90-8.fc18 (Doug Evans).

* Sun Jun  3 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120603-2.fc18
- Fix Release.
- Make yum --enablerepo compatible with at least mock-1.1.21-1.fc16 Rawhide cfg.

* Sun Jun  3 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120602-1.fc18
- Rebase to FSF GDB 7.4.50.20120602.
- [testsuite] BuildRequire gcc-go.
- Drop printing 2D C++ vectors as matrices which no longer worked (BZ 562763).
- Fix dejagnu-1.5-4.fc17 compatibility for Go (for BZ 635651).
- Use librpm.so.3 for rpm-4.10.0 in Fedora 18.
- Revert recent breakage of UNIX objfiles order for symbols lookup.

* Sat Jun  2 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-48.fc17
- [ppc] Fix hardware watchpoints on PowerPC (BZ 827600, Edjunior Machado).

* Mon May 28 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-47.fc17
- Workaround PR libc/14166 for inferior calls of strstr.

* Mon May 14 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-46.fc17
- [RHEL5] Workaround doc build race.

* Mon May 14 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-45.fc17
- Rename "set auto-load" patchset variable $ddir to $datadir.

* Wed May  9 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-44.fc17
- Fix in "set auto-load" patchset for SCL scripts inheritance (BZ 815910).

* Wed Apr 25 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-43.fc17
- [RHEL5] Workaround kernel for detaching SIGSTOPped processes (BZ 809382).

* Tue Apr 24 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-42.fc17
- Update "set auto-load" patchset and the --with-auto-load-safe-path setting.
- [RHEL] Disable gdb-add-index even on RHEL-6 as RHEL-6.0 had too old elfutils.

* Wed Apr 18 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-41.fc17
- [RHEL] Fix --with-auto-load-safe-path systems prior to /usr move.

* Wed Apr 18 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-40.fc17
- Security fix for loading untrusted inferiors, see "set auto-load" (BZ 756117).

* Fri Apr 13 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-39.fc17
- [RHEL7] Fix/remove readline-devel BuildRequires redundant distro suffic .fc17.

* Wed Apr  4 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-38.fc17
- Workaround crashes from stale frame_info pointer (BZ 804256).

* Wed Apr  4 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-37.fc17
- testsuite: Fix break-interp.exp expections for updated glibc (BZ 752834).

* Wed Apr  4 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-36.fc17
- [RHEL5,RHEL6] Reintroduce fix attaching to stopped processes.

* Fri Mar 30 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-35.fc17
- Fix performance regressions with .gdb_index (Tom Tromey, BZ 805274).

* Fri Mar 30 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-34.fc17
- Fixup %{_datadir}/gdb timestamps for multilib conflicts on RHELs.

* Mon Mar 26 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-33.fc17
- [vla] Fix regression on no type for subrange from IBM XLF Fortran (BZ 806920).

* Sat Mar 17 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-32.fc17
- Fix loading of core files without build-ids but with build-ids in executables.

* Fri Mar  9 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-31.fc17
- Fix an implied regression by the inferior calls fix below (BZ 799531).

* Fri Mar  9 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-30.fc17
- Fix SELinux deny_ptrace .spec build rules (BZ 786878).

* Tue Mar  6 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-29.fc17
- Fix inferior calls, particularly uncaught thrown exceptions (BZ 799531).
- Fix DWARF DIEs CU vs. section relative offsets (Joel Brobecker, me).

* Tue Mar  6 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-28.fc17
- Print reasons for failed attach/spawn incl. SELinux deny_ptrace (BZ 786878).

* Sun Mar  4 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-27.fc17
- [rhel5] Fix up the previous commit (BZ 799318).

* Sun Mar  4 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-26.fc17
- [rhel5] Fix up the previous commit (BZ 799318).

* Sun Mar  4 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-25.fc17
- [rhel5] Workaround rpmbuild to make the doc subpkg noarch again (BZ 799318).

* Fri Mar  2 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-24.fc17
- [vla] Fix crash for dynamic.exp with gcc-gfortran-4.1.2-51.el5.x86_64.
- Reintroduce RHEL-5 glibc workaround for bt-clone-stop.exp.
- testsuite: Update/fix rh634108-solib_address.exp for the upstreamed API.

* Wed Feb 29 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-23.fc17
- Add kernel vDSO workaround (`no loadable ...') on RHEL-5 (kernel BZ 765875).
- Fix skipping of prologues on RHEL-5 gcc-4.1 -O2 -g code (BZ 797889).
- Fix breakpoint warning during 'next' over exit() (Tom Tromey, BZ 797892).

* Tue Feb 28 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-22.fc17
- testsuite: Fix gdb.base/macscp.exp ccache workaround in SCL mode.
- Adjust the RHEL/F version string automatically (BZ 797651, BZ 797646).
- Provide gdbtui for RHEL-5 and RHEL-6 as it is removed upstream (BZ 797664).

* Fri Feb 24 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-21.fc17
- testsuite: Do not use gcc44/gfortran44 on RHEL-5 if in SCL mode.

* Wed Feb 22 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-20.fc17
- Fix libinproctrace.so build on RHEL-5 i386 (disable it on RHEL-5).

* Wed Feb 22 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-19.fc17
- Implement SCL (scl-utils-build) macros.

* Tue Feb 21 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-18.fc17
- Fix debuginfo gdb-gdb.py build without redhat-rpm-config and on RHEL-5.
- Provide precompiled variants of gdb-gdb.py.

* Mon Feb 13 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-17.fc17
- gstack: Turn off --readnever (suggested by Oliver Henshaw).

* Fri Feb 10 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-16.fc17
- [RHELs] Drop simulation of legacy behavior - new GDB should behave as new GDB.

* Fri Feb 10 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-15.fc17
- Simplify %%setup .spec rule.

* Fri Feb 10 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-14.fc17
- Drop --with upstream .spec rules.

* Fri Feb 10 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-13.fc17
- Drop --with debug .spec rules.

* Thu Feb  9 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-12.fc17
- Improve performance for C++ symbols expansion (Tom Tromey, BZ 787487).
- Install also gdb-gdb.py pretty printers.

* Thu Feb  9 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-11.fc17
- Fix possible NULL crash in find_charset_names (Tom Tromey, BZ 786091).
- [ppc*] Fix build failure due to GCC aliasing warning (BZ 786504).

* Sat Jan 21 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120120-10.fc17
- Rebase to FSF GDB 7.4.50.20120120.
- Drop the g77 .spec provisioning as it has been fixed in FSF GDB.

* Thu Jan 19 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120103-9.fc17
- Enable smaller %{_bindir}/gdb in future by no longer using -rdynamic.
- Make --enablerepo to use '*-debug*' for RHEL compatibility (BZ 781571).
- On older RHELs make readline bundled again (BZ 701131).
- Fix build compatibility with RHEL-5 due to false noarch build.

* Wed Jan 11 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120103-8.fc17
- Disable unexpected GDB directories relocatability.

* Wed Jan 11 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120103-7.fc17
- Fix BuildRequires for RHEL compatibility (BZ 701131).

* Wed Jan 11 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120103-6.fc17
- Provide %%snap timestamp for: Provides: bundled(librarypackage)
- Replace %%define by %%global.
- Replace Java in Summary with Fortran (only GCC-compiled Java is supported).
- Unbundle readline-6.2 with a workaround of skipped "ask" (BZ 701131).
- Work around readline-6.2 incompatibility not asking for --more-- (BZ 701131).

* Sat Jan  7 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120103-5.fc17
- Mark %{_sysconfdir}/gdbinit as %%config(noreplace).
- Add appropriate: Provides: bundled(librarypackage).
- Remove excessive explicit Requires: librarypackage.

* Thu Jan  5 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120103-4.fc17
- Fix linking on non-x86* (such as s390*) after libgdb.a removal.

* Wed Jan  4 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120103-3.fc17
- Reinclude gdb-dlopen-stap-probe.patch (missing in Fedora glibc - BZ 752476).

* Tue Jan  3 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120103-2.fc17
- Fix SystemTap support regression on i686 (Sergio Durigan Junior).

* Tue Jan  3 2012 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.4.50.20120103-1.fc17
- Rebase to FSF GDB 7.4.50.20120103.

* Mon Nov 28 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.3.50.20110722-11.fc17
- No longer build bundled libstdc++ pretty printers on RHELs >= 7.

* Sat Nov 5 2011 Sergio Durigan Junior <sergiodj@redhat.com> - 7.3.50.20110722-10.fc16
- Backport fix for crash in cp_scan_for_anonymous_namespace
  (Aleksandar Ristovski, BZ 750341).

* Fri Oct 14 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.3.50.20110722-9.fc16
- Backport `info os processes' crash fix - for Eclipse (Pedro Alves, BZ 746294).

* Tue Oct 11 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.3.50.20110722-8.fc16
- Fix PIE testsuite run; new lib/future.exp hack and use -fPIC instead of -fPIE.

* Mon Sep 26 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.3.50.20110722-7.fc16
- [vla] Fix VLA arrays displayed in `bt full' (BZ 738482).
- Fix DW_OP_GNU_implicit_pointer for DWARF32 v3+ on 64-bit arches.
- Fix internal error on some optimized-out values.

* Tue Aug 16 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.3.50.20110722-6.fc16
- Python command/function auto-loading (Phil Muldoon, BZ 730976).
- Work around PR libc/13097 "linux-vdso.so.1" warning message.
- [TUI] Fix stepi on stripped code.
- Add BuildRequires: systemtap-sdt-devel for archer-sergiodj-stap-patch-split.

* Wed Aug 10 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.3.50.20110722-5.fc16
- Fix dlopen of libpthread.so, patched glibc required (Gary Benson, BZ 669432).

* Tue Aug  9 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.3.50.20110722-4.fc16
- Improve GDB performance on inferior dlopen calls (Gary Benson, BZ 698001).
- [python] Fix crash when pretty printer fails (Phil Muldoon, BZ 712715).
- Fix crash on invalid C++ mangled names (BZ 729283).

* Fri Jul 29 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.3.50.20110722-3.fc16
- Fix regression from VLA merge affecting -O0 -g watchpoints.

* Fri Jul 29 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.3.50.20110722-2.fc16
- Include gcc -g3 .debug_macro implementation by Tom Tromey.

* Sat Jul 23 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.3.50.20110722-1.fc16
- Rebase to FSF GDB 7.3.50.20110722.
- Improve gcc-4.6 stdarg false prologue end workaround (GDB PR 12435 + GCC PR 47471).

* Sun Jul  3 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.90.20110703-40.fc15
- Rebase to FSF GDB 7.2.90.20110703 (which is a 7.3 pre-release).
  - Adjust the `print errno' patch due to the DW_AT_linkage_name following again.

* Fri Jun 24 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.90.20110525-39.fc15
- Fix install-info for the gdb-doc subpackage (BZ 715228).

* Wed May 25 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.90.20110525-38.fc15
- Rebase to FSF GDB 7.2.90.20110525 (which is a 7.3 pre-release).
- [stap] Fix double free (Sergio Durigan Junior).

* Tue May  3 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.90.20110429-37.fc15
- Search also for .<seqno> files in /usr/lib/debug/.build-id (BZ 641377).

* Mon May  2 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.90.20110429-36.fc15
- Bundle readline-6.2 with a workaround of skipped "ask" (BZ 701131).
  - Use --without-system-readline, disable Requires and BuildRequires of readline.
  - Drop gdb-6.5-readline-long-line-crash.patch and gdb-readline-6.0-signal.patch.

* Fri Apr 29 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.90.20110429-35.fc15
- Rebase to FSF GDB 7.2.90.20110429 (which is a 7.3 pre-release).
- Fix -O2 -g breakpoints internal error + prologue skipping (BZ 612253).
- Fix case insensitive symbols for Fortran by iFort (BZ 645773).
- Fix physname-related CU expansion issue for C++ (PR 12708).
- Fix Python access to inlined frames (BZ 694824).

* Mon Apr 11 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.90.20110411-34.fc15
- Rebase to FSF GDB 7.2.90.20110411 (which is a 7.3 pre-release).
- Include the proper fix for anonymous struct typedefs (Tom Tromey, BZ 672230).

* Wed Mar 30 2011 Fabio M. Di Nitto <fdinitto@redhat.com> - 7.2.50.20110328-33.fc15
- Cleanup spec file to add sparc|sparcv9|sparc64.
- Add sparc specific workarounds to toolchain badness:
  - disable mmap in bdf/ via --without-mmap configure option.
  - add patch to not build mmap support on sparc for gdb/.
  - gdb code is NOT at fault, but we need a working gdb while we sort out
    the toolchain and rebuild all packages. this workaround is NOT for upstream.

* Tue Mar 29 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110328-32.fc15
- Fix occasional crash on `print errno' with no -pthread and no -g3 (BZ 690908).

* Mon Mar 28 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110328-31.fc15
- Rebase to FSF GDB 7.2.50.20110328 (which is a 7.3 pre-release).
- Bundle %%{libstdcxxpython}.tar.bz2 unconditionally - for rebulds on RHELs.

* Sun Mar 20 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110320-30.fc15
- Fix threading internal error on corrupted memory (BZ 677654).
- Fix i386 rwatch+awatch before run (BZ 688788, on top of BZ 541866).

* Sun Mar 20 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110320-29.fc15
- Rebase to FSF GDB 7.2.50.20110320 (which is a 7.3 pre-release).
- Merge archer-sergiodj-stap, the SystemTap probes breakpoints feature.
  - [stap] Fix -O2 warnings.
- Fix Ada support crash on uninitialized gdbarch.

* Sat Mar  5 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110305-28.fc15
- Rebase to FSF GDB 7.2.50.20110305 (which is a 7.3 pre-release).

* Fri Feb 25 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110222-27.fc15
- Include doc also in the PDF form; new BuildRequires: texinfo-tex.

* Wed Feb 23 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110222-26.fc15
- Rebase to FSF GDB 7.2.50.20110222 (which is a 7.3 pre-release).
- Fix attach/core-load of {,un}prelinked i386 libs (bugreport by Michal Toman).

* Mon Feb 21 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110218-25.fc15
- Drop %%{_datadir}/gdb/syscalls/* for unsupported arches.

* Fri Feb 18 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110218-24.fc15
- Rebase to FSF GDB 7.2.50.20110218 (which is a 7.3 pre-release).
- [vla] Fox Fortran vector slices for allocated arrays (for BZ 609782).

* Tue Feb 15 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110213-23.fc15
- Move the GFDL License to gdb-doc.

* Tue Feb 15 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110213-22.fc15
- Fix gdb-doc Group to be Documentation, also provide it as noarch.

* Tue Feb 15 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110213-21.fc15
- Drop non-user (gdbint) and obsolete (stabs) documentation.
- Install also HTML files besides the INFO file.
- Create new subpackage gdb-doc for both INFO and HTML files.

* Sun Feb 13 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110213-20.fc15
- Rebase to FSF GDB 7.2.50.20110213 (which is a 7.3 pre-release).
- Fix occasionall unfound source lines (affecting at least glibc debugging).
- Fix const/volatile qualifiers of C++ types (PR c++/12328).
- Be backward compatible for --rebuild with <=fc14 librpm.so.1.

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 7.2.50.20110206-19.fc15
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Sun Feb  6 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110206-18.fc15
- Fix regressions on C++ names resolving (PR 11734, PR 12273, Keith Seitz).

* Sun Feb  6 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110206-17.fc15
- Rebase to FSF GDB 7.2.50.20110206 (which is a 7.3 pre-release).

* Thu Jan 27 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110125-16.fc15
- Fix Python new-backtrace command (BZ 672235, Phil Muldoon).

* Wed Jan 26 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110125-15.fc15
- Temporary fix of F15 gcc-4.6 child DIEs of DW_TAG_typedef (BZ 672230).
- Workaround gcc-4.6 stdarg false prologue end (GDB PR 12435 + GCC PR 47471).

* Tue Jan 25 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110125-14.fc15
- Rebase to FSF GDB 7.2.50.20110125 (which is a 7.3 pre-release).
- Fix discontiguous address ranges in .gdb_index - v3->v4 (BZ 672281).
- Fix DWARF-3+ DW_AT_accessibility default assumption for F15 gcc-4.6.

* Thu Jan 20 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110117-13.fc15
- Use librpm.so.2 for rpm-4.9.0 in Fedora 15.

* Mon Jan 17 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110117-12.fc15
- Use %%{?dist} for sanity checking tools compliance (suggested by Petr Muller).

* Mon Jan 17 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110117-11.fc15
- Rebase to FSF GDB 7.2.50.20110117 (which is a 7.3 pre-release).
- Fix callback-mode readline-6.0 regression for CTRL-C (for RHEL-6.0).
  - Fix occasional NULL dereference of the readline-6.0 workaround (BZ 575516).

* Sat Jan 15 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110107-10.fc15
- [delayed-symfile] Test a backtrace regression on CFIs without DIE (BZ 614604).
- [archer-tromey-delayed-symfile] New test gdb.dwarf2/dw2-aranges.exp.
- [archer-keiths-expr-cumulative+upstream] Import C++ testcases.
  - testsuite: Fix gdb-test-expr-cumulative-archer.patch compatibility.

* Fri Jan  7 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110107-9.fc15
- Remove --with-pythondir as no longer valid.
- Provide %{_bindir}gdb-add-index even on RHEL-5.
- Provide again libstdc++ pretty printers for any RHEL.

* Fri Jan  7 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110107-8.fc15
- Rebase to FSF GDB 7.2.50.20110107 (which is a 7.3 pre-release).
- Import archer-tromey-python (BZ 666177, branch update by Phil Muldoon).

* Tue Jan  4 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20110104-7.fc15
- Rebase to FSF GDB 7.2.50.20110104 (which is a 7.3 pre-release).
- New testcase py-prettyprint.exp:print hint_error (for BZ 611569, BZ 629236).
- New test gdb.arch/x86_64-pid0-core.exp for kernel PID 0 cores (BZ 611435).

* Sat Jan  1 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20101231-6.fc15
- Fix --with-system-readline doc build upstream regression.

* Sat Jan  1 2011 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20101231-5.fc15
- Rebase to FSF GDB 7.2.50.20101231 (which is a 7.3 pre-release).
- Remove gdb-6.3-bt-past-zero-20051201.patch, gdb-archer-ada.patch and
  gdb-6.3-framepczero-20040927.patch already removed from .spec before.
- Remove gdb-6.5-dwarf-stack-overflow.patch, upstreamed (Tom Tromey).
- Remove gdb-6.6-bz225783-gdb-debuginfo-paths.patch, upstreamed (Tom Tromey).
- Remove gdb-6.6-readline-system.patch, reimplemented upstream (Tom Tromey).
- Remove gdb-bz642879-elfread-sigint-stale.patch, upstreamed (Jan Kratochvil).
- Remove gdb-next-over-throw.patch, upstreamed (Tom Tromey).

* Mon Dec 27 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20101117-4.fc15
- Provide stub %%{_sysconfdir}/gdbinit (BZ 651232).

* Mon Dec 27 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20101117-3.fc15
- Fix ppc* compilation of PRPSINFO in the core files (BZ 662995, for BZ 254229).
- Fix (disable) non-x86* compilation of libinproctrace.so (for BZ 662995).

* Thu Nov 18 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20101117-2.fc15
- Drop gdb-6.8-glibc-headers-compat.patch: GNU/Linux irrelevant (Tom Tromey).
- Drop gdb-6.3-terminal-fix-20050214.patch: The bug is not reproducible.
- Drop gdb-6.7-kernel-headers-compat.patch: kernel-headers seem to be fixed.
- Drop gdb-archer-ada.patch: No longer needed for Ada (Keith Seitz).
- New PR backtrace/12237, drop gdb-6.3-framepczero-20040927.patch
  gdb-6.3-bt-past-zero-20051201.patch as they already had no effect.
- Drop gdb-6.8-gcc35998-ada-memory-trash.patch as a different fix is upstream.
- Drop gdb-6.3-inheritance-20050324.patch: the call is redundent (Tom Tromey).
- Drop gdb-6.3-large-core-20051206.patch: obsoleted by MAX_COPY_BYTES.

* Thu Nov 18 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2.50.20101117-1.fc15
- Rebase to FSF GDB 7.2.50.20101117 (which is a 7.3 pre-release).

* Sun Nov  7 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-25.fc14
- iFort compat. - case insensitive DWARF not in lowercase (BZ 645773).

* Thu Oct 14 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-24.fc14
- Add gdb.spec comments on the *.patch files upstream merge status.

* Thu Oct 14 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-23.fc14
- Workaround librpm BZ 643031 due to its unexpected exit() calls (BZ 642879).
- Fix crash on CTRL-C while reading an ELF symbol file (BZ 642879).

* Tue Oct 12 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-22.fc14
- testsuite: Provide missing lib/gdb-python.exp (for BZ 639089).

* Tue Oct 12 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-21.fc14
- Fix python stale error state, also fix its save/restore (BZ 639089).
- Fix inferior exec of new PIE x86_64 (BZ 638979).

* Tue Oct 12 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-20.fc14
- Fixup Release for 20.fc14.

* Tue Oct 12 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-19.fc14
- Use .gdb_index v3 to fix excessive resources rqmnts (BZ 640634, Tom Tromey).

* Wed Oct  6 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-18.fc14
- Fix false warning: non-absolute filename: <the main exec. file> (BZ 640648).

* Thu Sep 30 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-17.fc14
- New Conflicts: elfutils < 0.149 due to the .gdb_index .debug support.

* Wed Sep 29 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-16.fc14
- [ifunc] Fix crash on deleting watchpoint of an autovariable (BZ 637770).

* Mon Sep 27 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-15.fc14
- Revert the -O0 switch formerly to workaround GCC BZ 634757 (cmove bug).
- Remove no longer used BuildRequires: libstdc++.
- Remove commented out python libstdc++ .spec code.

* Sat Sep 25 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-14.fc14
- Fixup %{_datadir}/gdb/python/gdb timestamps for multilib conflicts.

* Sat Sep 25 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-13.fc14
- Fix .gdb_index for big-endian hosts (Tom Tromey).

* Sat Sep 25 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-12.fc14
- Fix lost siginfo_t in linux-nat (BZ 592031).

* Sat Sep 25 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-11.fc14
- Fix infinite loop crash on self-referencing class (BZ 627432).

* Thu Sep 23 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-10.fc14
- gcore/-Wl,-z,relro: Always write out all the pages until kernel gets a fix.

* Wed Sep 22 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-9.fc14
- Fix gcore writer for -Wl,-z,relro (PR corefiles/11804).

* Wed Sep 22 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-8.fc14
- Enable python by default even in Brew and on all the arches (BZ 609157).

* Wed Sep 22 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-7.fc14
- python: load *-gdb.py for shlibs during attach (BZ 634660).
- Fix double free crash during overload resolution (PR 12028, Sami Wagiaalla).

* Sat Sep 18 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-6.fc14
- Fix python gdb.solib_address (BZ 634108, fix by Phil Muldoon).
- Temporarily build with -O0 to workaround GCC BZ 634757 (cmove bug).

* Tue Sep 14 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-5.fc14
- Fix Ada regression when any .gdb_index library is present.

* Sat Sep 11 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-4.fc14
- Fix symbol lookup misses methods of current class (BZ 631158, Sami Wagiaalla).
- Fix python gdb.execute-to_string redirection (BZ 627506, with Paul Bolle).

* Wed Sep  8 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-3.fc14
- Do not crash on broken separate debuginfo due to old elfutils (BZ 631575).

* Sat Sep 04 2010 Dennis Gilmore <dennis@ausil.us> - 7.2-2.fc14
- libinproctrace doesnt exist on sparc arches

* Fri Sep  3 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.2-1.fc14
- Formal update to the final FSF GDB release.

* Tue Aug 24 2010 Dan Horák <dan[at]danny.cz> - 7.1.90.20100806-12.fc14
- libinproctrace doesn't exist on s390(x)

* Thu Aug 19 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1.90.20100806-11.fc14
- Fix crash on MI variable calling inferior function (BZ 610986).

* Tue Aug 10 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1.90.20100806-10.fc14
- Fix /usr/bin/gdb-add-index missing -nx for gdb.
- New option --with profile (disabled by default - missing workload, BZ 615603).

* Sat Aug  7 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1.90.20100806-9.fc14
- Fix python gdb.execute to_string pagination (BZ 620930).

* Fri Aug  6 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1.90.20100806-8.fc14
- Out of memory is just an error, not fatal (uninitialized VLS vars, BZ 568248).

* Fri Aug  6 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1.90.20100806-7.fc14
- Fix gcore from very small terminal windows (BZ 555076).
- Fix false `filesystem' debuginfo rpm request (BZ 599598).

* Wed Jul 28 2010 Mamoru Tasaka <mtasaka@ioa.s.u-tokyo.ac.jp> - 7.1.90.20100721-6.fc14
- Rebuild against python 2.7

* Thu Jul 22 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1.90.20100721-5.fc14
- Fix /usr/bin/gdb-add-index $d -> $dir typo.

* Thu Jul 22 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1.90.20100721-4.fc14
- Import archer-tromey-python.
- Import archer-tromey-optional-psymtab (as present in FSF GDB post-7.2).
  - Provide /usr/bin/gdb-add-index for rpm-build (Tom Tromey).

* Thu Jul 22 2010 David Malcolm <dmalcolm@redhat.com> - 7.1.90.20100721-3.fc14
- Rebuilt for https://fedoraproject.org/wiki/Features/Python_2.7/MassRebuild

* Wed Jul 21 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1.90.20100721-2.fc14
- Fix prelinked executables with sepdebug and copy relocations (BZ 614659).

* Wed Jul 21 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1.90.20100721-1.fc14
- Rebase to FSF GDB 7.1.90.20100721 (which is 7.2 pre-release).

* Tue Jul 13 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-29.fc14
- Disable temporarily Python files before the new rebase is done (BZ 613710).

* Sun Jul 11 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-28.fc14
- Rebuild for Fedora 14.

* Wed Jun 30 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-28.fc13
- Print 2D C++ vectors as matrices (BZ 562763, sourceware10659, Chris Moller).

* Wed Jun 30 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-27.fc13
- Fix obstack corruptions on C++ (BZ 606185, Chris Moller, Jan Kratochvil).
- Improve support for typedefs in classes (BZ 602314).
- Fix `set print object on' for some non-dynamic classes (BZ 606660).

* Wed Jun  9 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-26.fc13
- Backport DWARF-4 support (BZ 601887, Tom Tromey).

* Wed Jun  9 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-25.fc13
- Fix ADL anonymous type crash (BZ 600746, Sami Wagiaalla).

* Tue Jun  1 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-24.fc13
- Fix crash on /proc/PID/stat race during inferior exit (BZ 596751).
- testsuite: gdb.threads/watchthreads-reorder.exp kernel-2.6.33 compat. fix.

* Sun May 30 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-23.fc13
- Fix and support DW_OP_*piece (Tom Tromey, BZ 589467).
- Fix follow-exec for C++ programs (bugreported by Martin Stransky).

* Mon May 24 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-22.fc13
- Remove core file when starting a process (BZ 594560).
- Fix lock up on loops in the solib chain (BZ 593926).
- Import fix of TUI layout internal error (BZ 595475).

* Sun May 16 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-21.fc13
- Make gdb-6.8-bz254229-gcore-prpsinfo.patch RHEL-5 /usr/bin/patch compatible
  (bugreported by Jonas Maebe).

* Thu May 13 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-20.fc13
- Fix crash on VLA bound referencing an optimized-out variable (BZ 591879).
- Re-enable the BZ 575292 and BZ 585445 C++ fix using an updated patch.

* Wed May 12 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-19.fc13
- Backport <tab>-completion bug on anonymous structure fields (BZ 590648).
- testsuite: Fix gdb.base/vla-overflow.exp FAILing on s390x (BZ 590635).
- Workaround non-stop moribund locations exploited by kernel utrace (BZ 590623).

* Thu Apr 29 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-18.fc13
- Make _Unwind_DebugHook independent from step-resume breakpoint (Tom Tromey).

* Tue Apr 27 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-17.fc13
- Fail gracefully if the _Unwind_DebugHook arg. is optimized out (Tom Tromey).

* Tue Apr 27 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-16.fc13
- Temporarily workaround the crash of BZ 575292 as there was now BZ 585445.

* Mon Apr 26 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-15.fc13
- Fix crash when using GNU IFUNC call from breakpoint condition.
- Avoid internal error by disabling the previous BZ 575292 fix (BZ 585445).

* Thu Apr 22 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-14.fc13
- Fix crash on C++ types in some debug info files (BZ 575292, Keith Seitz).
- Pretty printers not well documented (BZ 570635, Tom Tromey, Jan Kratochvil).

* Fri Apr 16 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-13.fc13
- archer-jankratochvil-fedora13 commit: 39998c496988faaa1509cc6ab76b5c4777659bf4
- [vla] Fix boundaries for arrays on -O2 -g (support bound-ref->var->loclist).
- [vla] Fix copy_type_recursive for unavailable variables (Joost van der Sluis).

* Sun Apr 11 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-12.fc13
- Fix crash on trying to load invalid executable (BZ 581215).

* Thu Apr  8 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-11.fc13
- testsuite: Fix gdb.base/gstack.exp also for ppc64 inferiors (for BZ 579793).

* Thu Apr  8 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-10.fc13
- Fix s390 --with testsuite Buildrequiers to be (s390-32) (BZ 580347, Cai Qian).

* Wed Apr  7 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-9.fc13
- Fix gstack to print even the frame #0.  New gdb.base/gstack.exp.  (BZ 579793)
- Merge gdb-6.3-gstack-without-path-20060414.p* into gdb-6.3-gstack-20050411.p*,
  no real code change.

* Mon Apr  5 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-8.fc13
- Fix breakpoint at *_start (BZ 162775, bugreport by John Reiser).

* Sat Apr  3 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-7.fc13
- Fix ppc build of the AVX registers support (for BZ 578250).

* Sat Apr  3 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-6.fc13
- Support AVX registers (BZ 578250).

* Sat Apr  3 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-5.fc13
- Fix dangling displays in separate debuginfo (BZ 574483).

* Wed Mar 31 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-4.fc13
- Remove gdb-readline-6.0-signal.patch with a bug causing crash while no longer
  required with F-13 readline-6.1 (BZ 575516)

* Mon Mar 29 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-3.fc13
- [expr-cumulative] using-directive: Fix memory leak (Sami Wagiaalla).

* Mon Mar 29 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-2.fc13
- Drop obsoleted `gdb-archer-pie-0315-breakpoint_address_match.patch'.
- Do not consider memory error on reading _r_debug->r_map as fatal (BZ 576742).
  - PIE: Attach binary even after re-prelinked underneath.
  - PIE: Attach binary even after ld.so re-prelinked underneath.
  - PIE: Fix occasional error attaching i686 binary (BZ 576742).
- testsuite: Fix unstable results of gdb.base/prelink.exp.

* Thu Mar 25 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.1-1.fc13
- Update to new FSF GDB release.

* Mon Mar 15 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.90.20100312-24.fc13
- Drop gdb-6.5-bz218379-ppc-solib-trampoline-fix.patch having false symbols
  resolving (related to BZ 573277).

* Fri Mar 12 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.90.20100312-23.fc13
- Update to new FSF GDB snapshot.
- Fix double-free on std::terminate handler (Tom Tromey, BZ 562975).

* Wed Mar 10 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.90.20100306-22.fc13
- Another License update.

* Wed Mar 10 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.90.20100306-21.fc13
- Update License for all the licenses contained in .src.rpm.

* Mon Mar  8 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.90.20100306-20.fc13
- Remove unapplied: gdb-6.8-inlining-addon.patch gdb-6.8-inlining-by-name.patch

* Mon Mar  8 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.90.20100306-19.fc13
- Include also %%doc COPYING3 (review by Petr Machata).
- Remove URL for Source (review by Matej Cepl).

* Sun Mar  7 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.90.20100306-18.fc13
- archer-jankratochvil-fedora13 commit: 59c35a31f0981a0f0b884b32c91ae6325b2126cd

* Sun Feb 28 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100203-17.fc13
- Fix false warning: section .gnu.liblist not found in ...
- Fix crash on stale addrinfo->sectindex (more sensitive due to the PIE patch).

* Fri Feb 26 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100203-16.fc13
- Fix ia64 part of the bt-clone-stop.exp fix.
- Fix gdb.ada/* regressions (Keith Seitz).
- Remove false gdb_assert on $sp underflow.

* Mon Feb  8 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100203-15.fc13
- Fix i386+x86_64 rwatch+awatch before run, regression against 6.8 (BZ 541866).

* Wed Feb  3 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100203-14.fc13
- Rediff gdb-6.8-bz254229-gcore-prpsinfo.patch for older patch(1) compatibility.

* Wed Feb  3 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100203-13.fc13
- archer-jankratochvil-fedora13 commit: 59c35a31f0981a0f0b884b32c91ae6325b2126cd
- Fortran: Fix regression on setting breakpoint at toplevel symbols (BZ 559291;
  David Moore, Intel).

* Mon Feb  1 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100128-12.fc13
- archer-jankratochvil-fedora13 commit: 5a573e8b26a2f0a6947d4c0249e43e5456610860
- Remove ExcludeArch on ia64 as it is now fixed up.

* Sun Jan 31 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100128-11.fc13
- Fix failed gdb_assert due to the PIE patchset (BZ 559414).

* Thu Jan 28 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100128-10.fc13
- Upgrade to the FSF GDB snapshot: 7.0.50.20100128
- archer-jankratochvil-fedora13 commit: 39c5a8b75fad3acd7204903db5dee025055a4594
  - Fix a regression on "AAA::ALPHA" test due to a merge from FSF GDB.
- Fix a regression of previous release due to false identification as core file.
- Move ifunc .patch into the GIT-managed archer-jankratochvil-fedora13 branch.
- Update gdb.pie/corefile.exp from 2007-01-26 FSF GDB commit by Andreas Schwab.

* Mon Jan 25 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100121-9.fc13
- Enable loading a core file just as a single argument to /usr/bin/gdb.

* Sun Jan 24 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100121-8.fc13
- testsuite: Fix gdb.arch/i386-bp_permanent.exp regression

* Sun Jan 24 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100121-7.fc13
- Update gdb.arch/powerpc-power7.exp for current binutils HEAD.

* Fri Jan 22 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100121-6.fc13
- Disable addon (finish) due to inline-cmds.exp: up from outer_inline2 assert.
- Fix gdb.arch/powerpc-power7.exp compatibility.

* Fri Jan 22 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100121-5.fc13
- Disable break-by-name on inlined functions due to a regression on parameters
  of inlined functions falsely <optimized out> (BZ 556975 Comment 8).

* Fri Jan 22 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100121-4.fc13
- Adjust BuildRequires for RHELs, add ExcludeArch on ia64.
- Disable one PIE-introduced assertion on RHELs.

* Thu Jan 21 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100121-3.fc13
- Revert FSF GDB gdbserver tracepoints as incomplete now.

* Thu Jan 21 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100121-2.fc13
- archer-jankratochvil-fedora13 commit: 21e418c04290aa5d2e75543d31fe3fe5d70d6d41
- [expr-cumulative] Fix "break expr if (cond)" regression.

* Thu Jan 21 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100121-1.fc13
- Upgrade to the FSF GDB snapshot: 7.0.50.20100121
- archer-jankratochvil-fedora13 commit: ccde1530479cc966374351038057b9dda90aa251
- [expr-cumulative] Archer branch is now included.

* Tue Jan 19 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100118-2.fc13
- Fix false PASS->FAIL of gdb.arch/i386-biarch-core.exp.

* Tue Jan 19 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100118-1.fc13
- Upgrade to the FSF GDB snapshot: 7.0.50.20100118
- Upgrade libstdc++-v3-python to r155978 (Phil Muldoon).

* Sat Jan 16 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.50.20100116-1.fc13
- Upgrade to the FSF GDB snapshot: 7.0.50.20100116
- archer-jankratochvil-fedora13 commit: 81810a20b2d2c3bf18e151de3cddfc96445b3c46
- [expr-cumulative] Archer branch is missing in this release.
- Update rpm.org#76 workaround for rpm-4.8 using librpm.so.1.
- Dissect archer-jankratochvil-misc into Patch403...Patch408.
- Some regressions exist in this release.

* Tue Jan 12 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.1-25.fc12
- non-librpm missing debuginfo yumcommand now prints also --disablerepo='*'
  to save some bandwidth by yum (Robin Green, BZ 554152).

* Sun Jan 10 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.1-24.fc12
- testsuite: BuildRequires also valgrind.

* Fri Jan  8 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.1-23.fc12
- Workaround missing libstdc++%%{bits_other} in Koji.

* Fri Jan  8 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.1-22.fc12
- Comply with new package review:
  - Fix .spec Source as this is not a snapshot now.
  - Convert all spaces to tabs.
  - Fix missing %%defattr at %%files for gdbserver.
  - Replace all hardcoded-library-path by variants of %%{_isa}.
- Include %%{_isa} for appropriate Requires and BuildRequires.

* Thu Jan  7 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.1-21.fc12
- [vla] Fix regression on fields of structs in internal vars (BZ 553338).
- archer-jankratochvil-fedora12 commit: 6e73988f653ba986e8742f208f17ec084292cbd5

* Thu Jan  7 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.1-20.fc12
- Fix crash reading broken stabs (it377671).

* Sun Jan  3 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.1-19.fc12
- testsuite: Fixup false FAILs for gdb.cp/constructortest.exp.

* Sat Jan  2 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.1-18.fc12
- Fix regression of gdb-7.0 (from 6.8) crashing on typedefed bitfields.
- Fix related_breakpoint stale ref crash.

* Fri Jan  1 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0.1-17.fc12
- Formal upgrade to the FSF GDB release gdb-7.0.1.
  - Fix regression of gdb-7.0.1 not preserving typedef of a field.

* Fri Jan  1 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-16.fc12
- More RHEL-5 compatibility updates.
  - Disable the build-id support by default.
  - Bundle back gdbserver to the base gdb package.
  - Remove bundled pstack.
  - Drop the BuildRequires of rpm-devel.

* Fri Jan  1 2010 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-15.fc12
- Fix error on a sw watchpoint active at function epilogue (hit on s390x).
- testsuite: Fix false MI "unknown output after running" regression.
- testsuite: Update ia64-sigtramp.exp for recent GDB.
- Implement bt-clone-stop.exp fix also for ia64.
- testsuite: Upstream condbreak.exp results stability fix (Daniel Jacobowitz).

* Thu Dec 24 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-14.fc12
- testsuite: Fix constructortest.exp and expand-sals.exp for gcc-4.4.2-20.fc12.

* Mon Dec 21 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-13.fc12
- [pie] Fix a race in testcase gdb.base/valgrind-db-attach.exp.
- Fix regression by python on ia64 due to stale current frame.
- Disable python iff RHEL-5 && (Brew || ppc64).

* Mon Dec 21 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-12.fc12
- Workaround build on native ppc64 host.
- More RHEL-5 compatibility updates.
  - Disable warning messages new for gdb-6.8+ for RHEL-5 backward compatibility.
  - Workaround RHEL-5 kernels for detaching SIGSTOPped processes (BZ 498595).
  - Serialize the testsuite output to keep the order for regression checks.
  - Re-enable python for all non-ppc* arches.
  - More gcc44 stack exceptions when running the testsuite on RHEL-5.
- Fix backward compatibility with G++ 4.1 namespaces "::".
- Fix regression on re-setting the single ppc watchpoint slot.
- Update snapshot of FSF gdb-7.0.x branch.
  - Backport fix of dcache invalidation locking up GDB on ppc64 targets.

* Fri Dec 18 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-11.fc12
- [pie] Fix general ppc64 regression due to a function descriptors bug.
- [pie] Fix also keeping breakpoints disabled in PIE mode.
- Import upstream <tab>-completion crash fix.
- Drop some unused patches from the repository.
- More RHEL-5 build compatibility updates.
  - Use gfortran44 when running the testsuite on RHEL-5.
  - Disable python there due to insufficient ppc multilib.
- Fix orphanripper hangs and thus enable it again.

* Mon Dec 14 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-10.fc12
- Make gdb-6.3-rh-testversion-20041202.patch to accept both RHEL and Fedora GDB.
- Adjust BuildRequires for Fedora-12, RHEL-6 and RHEL-5 builds.
- [vla] Fix compatibility of dynamic arrays with iFort (BZ 514287).
- Fix stepping through OMP parallel Fortran sections (BZ 533176).
- New fix of bp conditionals [bp_location-accel] regression (BZ 538626).

* Mon Dec  7 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-9.fc12
- Replace the PIE (Position Indepdent Executable) support patch by a new one.
- Drop gdb-6.3-nonthreaded-wp-20050117.patch as fuzzy + redundant.
- Fix callback-mode readline-6.0 regression for CTRL-C.
- Fix syscall restarts for amd64->i386 biarch.
- Various testsuite results stability fixes.
- Fix crash on reading stabs on 64bit (BZ 537837).
- archer-jankratochvil-fedora12 commit: 16276c1aad1366b92e687c72cab30192280e1906
- archer-jankratochvil-pie-fedora12 ct: 2ae60b5156d43aabfe5757940eaf7b4370fb05d2

* Thu Dec  3 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-8.fc12
- Fix slowness/hang when printing some variables (Sami Wagiaalla, BZ 541093).
- archer-jankratochvil-fedora12 commit: 6817a81cd411acc9579f04dcc105e9bce72859ff

* Wed Nov 25 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-7.fc12
- Support GNU IFUNCs - indirect functions (BZ 539590).
- Fix bp conditionals [bp_location-accel] regression (Phil Muldoon, BZ 538626).
- Fix missed breakpoint location [bp_location-accel] regression (upstream).

* Fri Oct 30 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-6
- Fix missing zlib-devel BuildRequires to support compressed DWARF sections.
- Include post-7.0 FSF GDB fixes.

* Fri Oct 23 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-5
- Make the package buildable on RHEL-5/CentOS-5 (without librpm there).
- archer-jankratochvil-fedora12 commit: 5b73ea6a0f74e63db3b504792fc1d37f548bdf5c

* Fri Oct 23 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-4
- Fix rpm --excludedocs (BZ 515998).

* Thu Oct 22 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-3
- Support multiple directories for `set debug-file-directory' (BZ 528668).

* Mon Oct 19 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-2
- Sync the .spec with RHEL/CentOS without EPEL, do not BuildRequires: fpc there.

* Wed Oct  7 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 7.0-1
- Formal upgrade to the final FSF GDB release gdb-7.0.
- Fix GNU/Linux core open: Can't read pathname for load map: Input/output error.
- archer-jankratochvil-fedora12 commit: ce4ead356654b951a49ca78d81ebfff95e758bf5

* Wed Sep 30 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.91.20090930-2
- Bump release.

* Wed Sep 30 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.91.20090930-1
- Fix broken python "help()" command "modules" (BZ 526552).
- Upgrade to the FSF GDB gdb-7.0 snapshot: 6.8.91.20090930
- archer-jankratochvil-fedora12 commit: 7cb860f03e2437c97239334ebe240d06f45723e0

* Sun Sep 27 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.91.20090925-3
- New test for step-resume breakpoint placed in multiple threads at once.

* Fri Sep 25 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.91.20090925-2
- Fix buildid-loading libs w/matching name but different build-id (BZ 524572).

* Fri Sep 25 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.91.20090925-1
- Upgrade to the FSF GDB gdb-7.0 snapshot: 6.8.91.20090925
- archer-jankratochvil-fedora12 commit: 4338ea85c798007c32594032f602db9fd230eba9
  - [python] Don't directly reference self.frame (Tom Tromey).
  - [expr] Updates from branch (Keith Seitz).

* Mon Sep 21 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.91.20090921-1
- Upgrade to the FSF GDB gdb-7.0 snapshot: 6.8.91.20090921
- archer-jankratochvil-fedora12 commit: 0d5c38dd89050c0ee1cf049656f177c170d675d4
  - [expr] Check has_stack_frames before calling find_pc_line (Sami Wagiaalla).

* Thu Sep 17 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.91.20090917-2
- Include bundled libstdc++ python; it will be in libstdc++-devel since gcc-4.5.

* Thu Sep 17 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.91.20090917-1
- Upgrade to the FSF GDB gdb-7.0 branch and snapshot: 6.8.91.20090917
- archer-jankratochvil-fedora12 commit: 16f3f01cc2cbc15283462eaabdfcde92cf42cdc6
- Drop the qsort_cmp workaround as resolved in FSF GDB now (BZ 515434).

* Thu Sep 10 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090910-1
- Upgrade to the FSF GDB gdb-6.8.50 snapshot: 6.8.50.20090910
- archer-jankratochvil-fedora12 commit: 941eb487a42933e442cb4d11344cda96ecb8a04d
  - [next-over-throw] Fix exceptions thrown during next (Tom Tromey).
  - [bp_location-accel] Do not (much) slow down on 500+ breakpoints (me).

* Thu Sep  3 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090818-8
- archer-jankratochvil-fedora12 commit: a081d2f12945e9468edd5f4341d3e945bd0fefe9
  - [expr] Fix too slow lookups in large C++ programs (Sami Wagiaalla).
  - [python] Fix varobj changed values reporting (GDB PR 10584, Tom Tromey).

* Tue Sep  1 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090818-7
- archer-jankratochvil-fedora12 commit: d25596676e8811b03f8c9aba6bbd04ebaa9ff5db
  - [call-frame-cfa] Fix parsing CFA-relative frames (BZ 516627, Tom Tromey).
  - [vla] variable length Fortran strings for -O -g code (part of BZ 508406, me).
  - [python] varobj + general fixes (Tom Tromey).

* Fri Aug 28 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090818-6
- Real upstream fixup of qsort_cmp (BZ 515434).
- Revert bitfields regression (BZ 520129).

* Tue Aug 25 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090818-5
- Temporarily disable assertion checks crashing in qsort_cmp (BZ 515434).

* Wed Aug 19 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090818-4
- Fixup "bad type" internal error, import from FSF GDB.
- archer-jankratochvil-fedora12 commit: 2ba2bc451eb832182ef84c3934115de7a329da7c

* Tue Aug 18 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090818-3
- archer-jankratochvil-fedora12 commit: 850e3cb38a25cb7fdfa4cef667626ffbde51bcac
- Fix the hardware watchpoints.

* Tue Aug 18 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090818-2
- Fix patch fuzz 0.

* Tue Aug 18 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090818-1
- Upgrade to the FSF GDB gdb-6.8.50 snapshot: 6.8.50.20090818
- archer-jankratochvil-fedora12 commit: 5e0d1cc74f119391a2c3ae25ef5749fc28674f06

* Wed Aug 12 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090811-4
- Fix minor regressions introduced by the rebase from F-11 (6.8.50.20090302).

* Tue Aug 11 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090811-3
- archer-jankratochvil-fedora12 commit: 2888fafe63889757c6fd27ccc2f25661d43fd1a4
- Drop archer-jankratochvil-vla VAROBJ invalidate/revalidate split to fix
  regressions against FSF GDB HEAD.

* Tue Aug 11 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090811-2
- archer-jankratochvil-fedora12 commit: 93f5e942bdcdcc376ece452c309bedabae71def9
- Fix "can't compute CFA for this frame" (by Tom Tromey, BZ 516627).

* Tue Aug 11 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090811-1
- Support constant DW_AT_data_member_location by GCC PR debug/40659 (BZ 515377).
- Fix .spec URL.
- archer-jankratochvil-fedora12 commit: 81de3c6abae4f7e3738aa9bcc0ab2f8725cce252

* Mon Aug 10 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090810-2
- Upgrade to the FSF GDB gdb-6.8.50 snapshot: 6.8.50.20090810
- archer-jankratochvil-fedora12 commit: 93ec16e6f5000dd64d433d86674e820ed0f35b72

* Tue Aug  4 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090803-2
- Drop the bundled libstdc++ python - it should be packaged on its own now.

* Tue Aug  4 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090803-1
- Upgrade to the FSF GDB gdb-6.8.50 snapshot: 6.8.50.20090803
- archer-jankratochvil-fedora12 commit: 0222cb1f4ddd1eda32965e464cb60b1e44e110b2

* Fri Jul 31 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-42
- Release bump only.

* Fri Jul 31 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-41
- Fix compatibility of --with-system-readline and readline-6.0+.
- Temporarily disabled orphanripper on Fedora 12.

* Fri Jul 24 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 6.8.50.20090302-40
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Mon Jul  6 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-39
- testsuite: Fix multiple runs in parallel on a single host.
- testsuite: Remove the rpmbuild option: --with parallel
- testsuite: Run the testsuite with default rpm _smp_mflags.

* Mon Jul  6 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-38
- Archer update to the snapshot: 17bfc0488f54aeeb7a9e20ef3caa7e31e8e985fb
- Archer backport: de9c5190034b84b0a5fb4b98b05b304cda187700
  - [vla] Fix a crash regression on constant DW_AT_data_member_location.

* Mon Jun 29 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-37
- Replace the fix of cloned-TIDs with no pthread from upstream (BZ 471819).
- Fix a parallel testsuite runs incompatibility in gdb.base/gcore-shmid0.exp.

* Mon Jun 29 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-36
- Fix GDB crash on cloned-TIDs with no associated pthread (BZ 471819).
- Workaround rpm.org#76 rpm-devel requirement for debuginfo names (BZ 508193).

* Mon Jun 22 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-35
- Accelerate sorting blocks on reading a file (found on WebKit) (BZ 507267).

* Mon Jun 22 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-34
- Fix backtraces from core files with the executable found+loaded via build-id.
  - Due to F-11 GCC no longer needlessly duplicating .eh_frame as .debug_frame.

* Tue Jun 16 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-33
- Archer update to the snapshot: 05c402a02716177c4ddd272a6e312cbd2908ed68
- Archer backport: 05c402a02716177c4ddd272a6e312cbd2908ed68
  - Remove the [archer-pmuldoon-exception-rewind-master] branch.
  - Include this functionality as a FSF GDB accepted patchset.

* Mon Jun 15 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-32
- Fix crash on pending breakpoints with PIE (position-indep.-exec.) (BZ 505943).

* Fri Jun 12 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-31
- Fix an occasional crash during printing of missing debuginfo rpms (BZ 505401).

* Fri Jun 12 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-30
- Implement DW_OP_call_frame_cfa (for recent GCC).

* Thu Jun 11 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-29
- Archer update to the snapshot: 30c13da4efe18f43ee34aa4b29bc86e1a53de548
- Archer backport: 30c13da4efe18f43ee34aa4b29bc86e1a53de548
  - Fix dereferencing unbound C arrays (BZ 505163).

* Wed Jun 10 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-28
- Archer update to the snapshot: 000db8b7bfef8581ef099ccca8689cfddfea1be8
- Archer backport: b8d3bea36b137effc929e02c4dadf73716cb330b
  - Ignore explicit die representing global scope '::' (gcc 4.1 bug).
- Archer backport: c2d5c4a39b10994d86d8f2f90dfed769e8f216f3
  - Fix parsing DW_AT_const_value using DW_FORM_string
- Archer backport: 8d9ab68fc0955c9de6320bec2821a21e3244600d
		 + db41e11ae0a3aec7120ad6ce86450d838af74dd6
  - Fix Fortran modules/namespaces parsing (but no change was visible in F11).
- Archer backport: 000db8b7bfef8581ef099ccca8689cfddfea1be8
  - Fix "some Python error when displaying some C++ objects" (BZ 504356).
- testsuite: Support new rpmbuild option: --with parallel
- testsuite: gdb-orphanripper.c: Fix uninitialized `termios.c_line'.
- Fix crashes due to (missing) varobj revalidation, for VLA (for BZ 377541).
- Archer backport: 58dcda94ac5d6398f47382505e9d3d9d866d79bf
		 + f3de7bbd655337fe6705aeaafcc970deff3dd5d5
  - Implement Fortran modules namespaces (BZ 466118).
- Fix crash in the charset support.

* Thu Apr 30 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-27
- Fix race in the ia64 testcase `gdb-6.3-rh-testlibunwind-20041202.patch'.

* Mon Apr 27 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-26
- Support a new rpmbuild option: --without python

* Mon Apr 27 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-25
- The Koji build failures may have been by forgotten check-in of the Patch360.

* Mon Apr 27 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-24
- Another new Koji build fix attempt now by: BuildPreReq: python

* Mon Apr 27 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-23
- Fix BuildRequires for new Koji.

* Mon Apr 27 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-22
- Fix pstack/gstack cutting very long lines (BZ 497849).

* Sun Apr 19 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-21
- New test for parsing elf64-i386 files for kdump PAE vmcore dumps (BZ 457187).

* Thu Apr 16 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-20
- Fix crash in the charset support.

* Wed Apr 15 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-19
- Fix crash on pretty-printer reading uninitialized std::string (BZ 495781).

* Mon Apr 13 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-18
- Archer update to the snapshot: d1fee5066408a09423621d1ebc64e6d3e248ed08
- Archer backport: 4854339f75bdaf4b228fc35579bddbb2a1fecdc1
  - Fix Python FrameIterator.

* Mon Apr 13 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-17
- Archer update to the snapshot: 7c250ce99c90cf6097e2ec55ea0f205830979cee
- Archer backport: c14d9ab7eef43281b2052c885f89d2db96fb5f8e
  - Revert a change regressing: gdb.objc/basicclass.exp
- Archer backport: ebd649b96e61a1fb481801b65d827bca998c6633
		 + 1f080e897996d60ab7fde20423e2947512115667
		 + 1948198702b51b31d79793fc49434b529b4e245f
		 + e107fb9687bb1e7f74170aa3d19c4a8f6edbb10f
		 + 1e012c996e121cb35053d239a46bd5dc65b0ce60
  - Update the Python API from upstream.
- Archer backport: d3c83ad5ec9f7672b87af9ad29279f459e53da11
  - Fix a Python branch crash.

* Mon Apr 13 2009 Dennis Gilmore <dennis@ausil.us> - 6.8.50.20090302-16
- enable gdbserver package on sparc64

* Sun Apr  5 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-15
- Archer update to the snapshot: 7c7c77576669d17ad5072daa47ea3a4fd954483d
- Archer backport: 7c7c77576669d17ad5072daa47ea3a4fd954483d (Peter Bergner)
  - Disassemble Power7 instructions right in the default/only -Many GDB mode.

* Sun Apr  5 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-14
- Archer update to the snapshot: f6273d446ff87e50976600ba3f71b88d61043e20
- Archer backport: f6273d446ff87e50976600ba3f71b88d61043e20
  - Use pretty-printers to print base classes inside a derived class.

* Mon Mar 30 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-13
- Archer update to the snapshot: d144a3633454046aaeae3e2c369c271834431d36
- Archer backport: a2c49b7640ebe7ce1376902d48d5bbbee600996b
  - Fixup compilation older GCCs.
- Archer backport: fe48224ce1bd22f37a7fa6d111d54c1a340392bf
  - KFAIL 4 cases of: gdb.arch/powerpc-power7.exp
- Archer backport: d144a3633454046aaeae3e2c369c271834431d36
  - Fix C local extern variables (requires gcc-4.4.0-0.30).

* Fri Mar 27 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-12
- Archer update to the snapshot: 837d9879980148af05eae540d92caeb7200f1813
- Archer backport: 8340d06295c8db80c544503458305197891e0348
  - Fixes [master] regression for Eclipse CDT testsuite.
- Archer backport: 16328456d5740917ade0a49bcecc14c4564b9a99
  - Fixes #2 [expr] compatibility with gcc-4.4 on gdb.cp/namespace-using.exp.
- Rebase [expr] on the Keith Seitz's sync with FSF GDB fixing the former merge.

* Sun Mar 22 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-11
- Archer update to the snapshot: e734ed95d296a3342d4147873c4641cea6c4d7fe
- Archer backport: 1e1d73cda98b1adda884b80e07c7b4929c175628
  - Fixes [expr] compatibility with gcc-4.4 on gdb.cp/namespace-using.exp.

* Sun Mar 15 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-10
- Archer update to the snapshot: 935f217d3367a642374bc56c6b146d376fc3edab
- Archer backport: 281278326412f9d6a3fabb8adc1d419fd7ddc7d7
  - Fix [expr] crash reading invalid DWARF C++ symbol "" (BZ 490319).

* Thu Mar 12 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-9
- Archer backport: aafe933b497eee8cfab736a10bae1a90d4bceb18
  - [python] Remove duplicate target-wide-charset parameter

* Mon Mar  9 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-8
- Archer update to the snapshot: a99e30d08ade4a2df0f943b036cd653bcd12b04d
- Fixes internal error on breaking at a multi-locations C++ caller (BZ 488572).

* Mon Mar  9 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-7
- Archer update to the snapshot: ec29855686f2a78d90ebcc63765681249bbbe808
- Temporarily place libstdc++ pretty printers in this gdb.rpm.

* Sat Mar  7 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-6
- Archer update to the snapshot: 543fb2154d3bd551344b990b911be5c6cc703504
 - Fixes [delayed-symfile] excessive `(no debugging symbols found)' messages.

* Sat Mar  7 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-5
- Improve `gdb-6.6-buildid-locate-rpm.patch' by dlopen() (+pkg-config compat.).

* Sat Mar  7 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-4
- Split `gdb-6.6-buildid-locate.patch' to `gdb-6.6-buildid-locate-rpm.patch'.

* Sat Mar  7 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-3
- Archer update to the snapshot: 6cf16c0711e844094ab694b3d929f7bd30b49f61
- Fix crash on the inlined functions support.
- Fix crash from the PIE support, its varobj_refresh() was called only before
  varobj_invalidate() which is sufficient.
- Fix BuildRequires for the `--with testsuite' runs.
- Use the newly introduced `--with-pythondir' option.
- Remove libstdcxx [python] pretty printers (as included in libstdc++ rpm now).

* Fri Mar 06 2009 Jesse Keating <jkeating@redhat.com> - 6.8.50.20090302-2
- Rebuild for new rpm libs

* Mon Mar  2 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090302-1
- Include the Archer Project: http://sourceware.org/gdb/wiki/ProjectArcher
  snapshot: 8cc3753a9aad85bf53bef54c04334c60d16cb251
  * [python] Python scripting support: http://sourceware.org/gdb/wiki/PythonGdb
  * [catch-syscall] Trap and display syscalls.
  * [delayed-symfile] Improve startup performance by lazily read psymtabs.
  * [exception-rewind] Fix fatal C++ exceptions in an inferior function call.
  * [expr] Expressions, single-quote elimination, C++ input canonicalization.
  * [using-directive] C++ namespaces.
  * [vla] C variable length arrays / DW_FORM_block / Fortran dynamic arrays.
  * [misc] Fix debuginfoless `return' (BZ 365111), fix command-line macros for
    expected GCC (BZ 479914), new testcase for valgrind (for BZ 483262),
    implement `info common' for Fortran, fix Fortran logical-kind=8 (BZ 465310),
    fix static variable in C++ constructors (BZ 445912), fix power7 (BZ 485319).
- Upgrade to the FSF GDB gdb-6.8.50 snapshot.
- Fix parsing elf64-i386 files for kdump PAE vmcore dumps (BZ 457187).
  - Turn on 64-bit BFD support, globally enable AC_SYS_LARGEFILE.

* Tue Feb 24 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 6.8.50.20090210-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Wed Feb 11 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090210-1
- Upgrade to the FSF GDB gdb-6.8.50 snapshot.

* Wed Feb 11 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20090209-1
- Upgrade to the FSF GDB gdb-6.8.50 snapshot.

* Mon Feb  9 2009 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20081214-2
- Fix crash / implement `finish' into inlined functions (BZ 479781).
- Drop the gdb.threads/attach-into-signal.exp change as obsolete.

* Sun Dec 14 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8.50.20081214-1
- Upgrade to the upstream gdb-6.8.50 snapshot.

* Mon Dec  1 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-33
- Make `--with testsuite' BuildRequires properly conditional.

* Mon Dec  1 2008 Stepan Kasal <skasal@redhat.com> - 6.8-32
- Remove trivial BuildRequires, use rpm macros in a few remaining places.

* Tue Nov 18 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-31
- Enable ia64 hardware watchpoints if created before starting inferior.

* Sun Nov  9 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-30
- Fix a race in the testcase `gdb.threads/step-thread-exit.exp'.

* Sun Nov  9 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-29
- Fix more the variable-length-arrays support (BZ 468266, feature BZ 377541).
- Integrate the `bt full' protection (for BZ 466901) into the VLA patch.

* Thu Nov  6 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-28
- Fix the "never terminate `bt full'" patch false GCC warning / build error.

* Thu Nov  6 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-27
- Fix resolving of variables at locations lists in prelinked libs (BZ 466901),
  bugreported by Michal Babej.
- Never terminate `bt full' on a problem of variable resolving (for BZ 466901).

* Thu Nov  6 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-26
- Fix more the variable-length-arrays support (BZ 468266, feature BZ 377541).
- Fix the watchpoints conditionals.
- Fix on PPC spurious SIGTRAPs on active watchpoints.
- Fix occasional stepping lockup on many threads, seen on ia64.

* Mon Nov  3 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-25
- Fix the variable-length-arrays support (BZ 468266, feature BZ 377541).
- Fix the debuginfo-install suggestions for missing base packages (BZ 467901),
  also update the rpm/yum code to no longer require _RPM_4_4_COMPAT.

* Tue Sep  2 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-24
- Fix PIE patch regression for loading binaries from valgrind (BZ 460319).

* Thu Aug 28 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-23
- Fix attaching to stopped processes, based on the upstream version now.
  - Just kernel-2.6.25 neither upstream nor utrace work with it; 2.6.9 works.
- Fix occasional crash on a removed watchpoint.
- Fix false testcase FAILs for `gdb.pie/break.exp'.
- Fix a false warning (+a testcase FAIL) on s390x watchpoints.
- Fix a false FAIL on s390x `gdb.base/dump.exp'.

* Wed Aug 27 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-22
- Remove `gdb-6.3-nonthreaded-wp-20050117.patch' as obsoleted + regressing now.
- Make the GDB quit processing non-abortable to cleanup everything properly.
- Support DW_TAG_constant for Fortran in recent Fedora/RH GCCs.
- Fix crash on DW_TAG_module for Fortran in recent Fedora/RH GCCs.
- Readd resolving of bare names of constructors and destructors.
- Include various vendor testcases:
  - Leftover zombie process (BZ 243845).
  - Multithreaded watchpoints (`gdb.threads/watchthreads2.exp').
  - PIE testcases (`gdb.pie/*').
  - C++ contructors/destructors (`gdb.cp/constructortest.exp').

* Sat Aug 23 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-21
- Fix MI debuginfo print on reloaded exec, found by Denys Vlasenko (BZ 459414).
- Extend the Fortran dynamic variables patch also for dynamic Fortran strings.

* Wed Aug 13 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-20
- Temporarily disable attaching to a stopped process (BZ 453688)
  - To be reintroduced after a fix of the kernel BZ 454404.

* Mon Aug  4 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-19
- Fix `errno' printing on nonthreaded non-g3 inferiors (TLS minsym is absolute).

* Fri Aug  1 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-18
- Fix powerpc recent secure PLTs handling (shared library calls) (BZ 452960).
- Fix the testsuite .spec runner to run biarch also on ppc.
- Reenable testcases threadcrash.exp, chng-syms.exp, checkpoint.exp (BZ 207002).
- Fix PRPSINFO in the core files dumped by gcore (BZ 254229), reformatted patch
  from Denys Vlasenko.
- Fix register assignments with no GDB stack frames, Denys Vlasenko (BZ 436037).

* Mon Jul 14 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-17
- Refresh the patchset with fuzz 0 (for new rpmbuild).

* Mon Jul 14 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-16
- Rebuild with the new rpm-4.5.90 in the buildroot.

* Sat Jul 12 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-15
- Temporary rpm-4.5.90 compatibility workaround by Panu Matilainen.
- Fix a regression in the constant watchpoints fix, found by Daniel Jacobowitz.
- Fix the prelink testcase for false FAILs on i386.

* Tue Jul  8 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-14
- Fix crash due to calling an inferior function right after a watchpoint stop.

* Thu Jul  3 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-13
- Support transparent debugging of inlined functions for an optimized code.

* Fri Jun 20 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-12
- Remove the gdb/gdbtui binaries duplicity.

* Tue Jun 17 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-11
- Fix the testsuite run for ia64 (where no -m64 is present).
- Test a crash on libraries missing the .text section.
- Protect development in the build tree by automatic Makefile dependencies.
- Refuse creating watchpoints of an address value, suggested by Martin Stransky.
- Disable randomization (such as by setarch -R), suggested by Jakub Jelinek.
- Fix compatibility with recent glibc headers.

* Sun Jun  1 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-10
- Fix crash on a watchpoint update on an inferior stop.
- Fix the s390x part of the hardware watchpoints after a fork.

* Thu May 22 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-9
- Fix memory trashing on binaries from GNAT/Ada (workaround GCC PR 35998).

* Thu May 15 2008 Tom "spot" Callaway <tcallawa@redhat.com> - 6.8-8
- Silence memcpy check which returns false positive (sparc64)

* Thu May 15 2008 Tom "spot" Callaway <tcallawa@redhat.com> - 6.8-7
- patch from DaveM for sparc/sparc64
- touch up spec to enable sparcv9/sparc64

* Sat May  3 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-6
- Fix gdb.base/gcore-shmid0.exp to be unresolved on recent kernels.
- Make the testsuite results of dfp-test.exp more stable.

* Sun Apr 27 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-5
- Remove the kernel VDSO workaround (`no loadable ...') (kernel BZ 312011).

* Wed Apr 23 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-4
- Backport fix on various forms of threads tracking across exec() (BZ 442765).
- Testsuite: Include more biarch libraries on %%{multilib_64_archs}.
- Disable the build-id warnings for the testsuite run as they cause some FAILs.
- Fix PIE support for 32bit inferiors on 64bit debugger.
- Fix trashing memory on one ada/gnat testcase.
- Make the testsuite results on ada more stable.

* Wed Apr 16 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-3
- Fix ia64 compilation errors (Yi Zhan, BZ 442684).
- Fix build on non-standard rpm-devel includes (Robert Scheck, BZ 442449).
- Do not run the PIE mode for the testsuite during `--with upstream'.
- Fix test of the crash on a sw watchpoint condition getting out of the scope.

* Fri Apr 11 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-2
- Fix a regression due to PIE of reloading a changed exec file (BZ 433410).
- Include also biarch libgcc on %%{multilib_64_archs} for the testsuite.
- Cosmetic fix of a testcase sanity breakpoint setting (part of BZ 233852).
- New test of hiding unexpected breakpoints on intentional step commands.
- New test of GCORE for shmid 0 shared memory mappings.
- New test of a crash on `focus cmd', `focus prev' commands.
- Fix a minor test race of the hardware watchpoints after the fork call.
- Test crash on a sw watchpoint condition getting out of the scope.

* Fri Mar 28 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.8-1
- Upgrade to the latest upstream final release gdb-6.8.

* Mon Mar 10 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.50.20080227-3
- build-id warnings integrated more with rpm and the lists of the warnings got
  replaced usually by a single-line `debuginfo-install' advice.
  - FIXME: Testsuite needs an update for the new pre-prompt messages.
- Fix the `--with upstream' compilation - gstack/pstack are now omitted.

* Tue Mar  4 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.50.20080227-2
- Drop the unused `ChangeLog.RedHat' file stubs.
- New rpm option `--with upstream' to drop the Fedora patches for testing.
- Drop some no longer valid .spec file comments.
- Include the Fortran dynamic arrays entry for changelog of 6.7.50.20080227-1.

* Mon Mar  3 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.50.20080227-1
- Upgrade to the upstream gdb-6.8 prerelease.
- Cleanup the leftover `.orig' files during %%prep.
- Add expat-devel check by the configure script (for the other-arch builds).
- `--with testsuite' now also BuildRequires: fpc
- Backport fix of a segfault + PIE regression since 6.7.1 on PIE executables.
- Update the printed GDB version string to be Fedora specific.
- Fix/implement the Fortran dynamic arrays support (BZ 377541).

* Sat Mar  1 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-16
- Run the full testsuite also in the `-fPIE -pie' mode.

* Mon Feb 25 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-15
- New --with parameters `testsuite' and `debug'.
  - Testsuite is now run during the build only on explicit `--with testsuite'.
- Testsuite now possibly produces two outputs for the two GDB target arches.

* Thu Feb 21 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-14
- Rename `set debug build-id' as `set build-id-verbose', former level 1 moved
  to level 2, default value is now 1, use `set build-id-verbose 0' now to
  disable the missing separate debug filenames messages (BZ 432164).

* Wed Feb 20 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-13
- ia64 build fixes from Doug Chapman (BZ 428882).
- gdbserver separated into an extra package (BZ 405791).
- pstack obsoleted by included gstack (BZ 197020).
- Fix #include <asm/ptrace.h> on kernel-headers-2.6.25-0.40.rc1.git2.fc9.x86_64.
- Drop the PowerPC simulator as no longer being compatible with Fedora binaries.

* Thu Feb  7 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-12
- build-id debug messages print now the library names unconditionally.

* Thu Jan 24 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-11
- Improve the text UI messages for the build-id debug files locating.
  - Require now the rpm libraries.
- Fix false `(no debugging symbols found)' on `-readnever' runs.
- Extend the testcase `gdb.arch/powerpc-prologue.exp' for ppc64.

* Sat Jan 12 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-10
- Compilation fixup (-9 was never released).

* Sat Jan 12 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-9
- Fix also threaded inferiors for hardware watchpoints after the fork call.
- Test debugging statically linked threaded inferiors (BZ 239652).
  - It requires recent glibc to work in this case properly.
- Testcase cleanup fixup of the gcore memory and time requirements of 6.7.1-8.

* Thu Jan 10 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-8
- Fix detaching from a threaded formerly stopped process with non-primary
  thread currently active (general BZ 233852).
  - Enable back again the testcases named `attachstop.exp' (no such exist now).
  - Rename the testcase `gdb.threads/attachstop' to `gdb.threads/attachstop-mt'.
- Test ia64 memory leaks of the code using libunwind.
- Testcase delay increase (for BZ 247354).
- Test gcore memory and time requirements for large inferiors.

* Mon Jan  7 2008 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-7
- Backport the gcc-4.3 compatibility -Werror fixes.
- Fix documentation on hardware watchpoints wrt multiple threads.
- Rename the patch file for BZ 235197 from its former name 234468.
- Fix the vendora testcase `attach-32.exp' affecting the other tests results.
- Support DW_TAG_interface_type the same way as DW_TAG_class_type (BZ 426600).

* Mon Dec 10 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-6
- Testsuite fixes for more stable/comparable results.

* Sat Nov 24 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-5
- Reduce the excessive gcc-* packages dependencies outside of mock/koji.

* Fri Nov 16 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-4
- Fix `errno' resolving across separate debuginfo files.
- Fix segfault on no file loaded, `set debug solib 1', `info sharedlibrary'.
- Extend the testsuite run for all the languages if %%{dist} is defined.
- Support gdb.fortran/ tests by substituting the g77 compiler by gfortran.

* Sun Nov  4 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-3
- Fix `errno' resolving on recent glibc with broken DW_AT_MIPS_linkage_name.
- Imported new test for 6.7 PPC hiding of call-volatile parameter register.

* Sat Nov  3 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-2
- Backport `Breakpoints at multiple locations' patch primarily for C++.

* Thu Nov  1 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7.1-1
- Upgrade to GDB 6.7.1.  Drop redundant patches, forward-port remaining ones.

* Thu Nov  1 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.7-1
- Upgrade to GDB 6.7.  Drop redundant patches, forward-port remaining ones.
- Fix rereading of the main executable on its change.

* Fri Oct 19 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-37
- Fix hiding unexpected breakpoints on intentional step/next commands.
- Fix s390 compilation warning/failure due to a wrongly sized type-cast.

* Sun Oct 14 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-36
- Fix hardware watchpoints after inferior forks-off some process.

* Sat Oct 13 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-35
- Fix non-threaded watchpoints CTRL-C regression on `set follow child'.

* Fri Oct 12 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-34
- Fix gdbserver for threaded applications and recent glibc (BZ 328021).

* Tue Oct  9 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-33
- Fix debug load for sparse assembler files (such as vDSO32 for i386-on-x86_64).

* Mon Oct  8 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-32
- Set the breakpoints always to all the ctors/dtors variants (BZ 301701).
- Fix a TUI visual corruption due to the build-id warnings (BZ 320061).
- Fixed the kernel i386-on-x86_64 VDSO loading (producing `Lowest section in').

* Fri Oct  5 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-31
- Fix address changes of the ctors/dtors breakpoints w/multiple PCs (BZ 301701).
- Delete an info doc file on `rpmbuild -bp' later rebuilt during `rpmbuild -bc'.

* Tue Sep 25 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-30
- Fix re-setting of the ctors/dtors breakpoints with multiple PCs (BZ 301701).
- Avoid one useless user question in the core files locator (build-id).

* Sun Sep 23 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-29
- Fixed the kernel VDSO loading (`warning: no loadable sections found in ...').
- Fix the testcase for pending signals (from BZ 233852).

* Sat Sep 22 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-28
- Support also the `$allocate' and `$delete' ctor/dtor variants (BZ 301701).
- Fix the build compatibility with texinfo >= 4.10.
- Fix the testcase for pending signals (from BZ 233852).

* Sun Sep 16 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-27
- Fix attaching to stopped processes and/or pending signals.

* Tue Aug 28 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-26
- New fast verification whether the .debug file matches its peer (build-id).
- New locating of the matching binaries from the pure core file (build-id).

* Fri Aug 17 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-25
- Fixed excessive RPATH (related to BZ 228891).

* Wed Aug  8 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-24
- Fixed compatibility with the Rawhide glibc open(2) syscall sanity checking.
- Update the core_dump_elf_headers=1 compatibility code to the upstream variant.

* Mon Aug  6 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-23
- Update PPC unwinding patches to their upstream variants (BZ 140532).

* Wed Jul 25 2007 Jesse Keating <jkeating@redhat.com> - 6.6-22
- Rebuild for RH #249435

* Mon Jul 23 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-21
- Fixed compatibility with Rawhide kernel fs.binfmt_elf.core_dump_elf_headers=1.
- .spec file updates to mostly pass RPMLINT - Fedora merge review (BZ 225783).
- Fixed testcase of the exit of a thread group leader (of BZ 247354).
- Cleanup any leftover testsuite processes as it may stuck mock(1) builds.

* Sun Jul  8 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-20
- Do not hang on exit of a thread group leader (BZ 247354).
- New test for upstream fix of VDSO decoding while attaching to an i386 process.
- Fixed BZ # 232014 -> 232015.

* Thu Jul  5 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-19
- Link with libreadline provided by the operating system.

* Tue Jun 26 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-18
- Fix PPC software watchpoints active while stepping atomic instr. (BZ 237572).

* Thu Jun 21 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-17
- Support for stepping over PPC atomic instruction sequences (BZ 237572).
- `set scheduler-locking step' is no longer enforced but it is now default.

* Wed Jun 20 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-16
- Fix attaching a stopped process on expected + upstream kernels (BZ 233852).
 - Fix attaching during a pending signal being delivered.

* Thu Jun  7 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-15
- Testcase update to cover PPC Power6/DFP instructions disassembly (BZ 230000).
- Disable some known timeouting/failing testcases to reduce the build time.
- Fix crash on missing filenames debug info (BZ 242155).

* Sat Apr 28 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-14
- Fixup for the PPC Power6/DFP instructions disassembly (BZ 230000).
- New testcase for the GCORE buffer overflow (for BZ 238285, formerly 235753).

* Wed Apr 25 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-13
- Fix `gcore' command for 32bit PPC inferiors on 64bit PPC hosts (BZ 232015).

* Wed Apr 25 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-12
- Enable PowerPC to print 128-bit long double variables (BZ 237872).
- New testcase for gcore of 32bit inferiors on 64bit hosts.

* Tue Apr 24 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-11
- Package review, analysed by Ralf Corsepius (BZ 225783).
 - Fix prelink(8) testcase for non-root $PATH missing `/usr/sbin' (BZ 225783).
 - Fix debugging GDB itself - the compiled in source files paths (BZ 225783).
 - Fix harmless GCORE stack buffer overflow, by _FORTIFY_SOURCE=2 (BZ 238285).
 - Fix XML support - the build was missing `expat-devel'.
 - Updated the `info' files handling by the spec file.
 - Building now with the standard Fedora code protections - _FORTIFY_SOURCE=2.
 - Use multiple CPUs for the build (%%{?_smp_mflags}).
 - Separate testsuite run to its %%check section.
 - Fix (remove) non-ASCII spec file characters.
 - Remove system tools versions dumping - already present in mock build logs.

* Sun Apr 22 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-10
- Notify user of a child forked process being detached (BZ 235197).

* Sun Apr 22 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-9
- Allow running `/usr/bin/gcore' with provided but inaccessible tty (BZ 229517).
- Fix testcase for watchpoints in threads (for BZ 237096).
- BuildRequires now `libunwind-devel' instead of the former `libunwind'.
- Use the runtime libunwind .so.7, it requires now >= 0.99-0.1.frysk20070405cvs.

* Sat Mar 24 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-8
- Use definition of an empty structure as it is not an opaque type (BZ 233716).
- Fixed the gdb.base/attachstop.exp testcase false 2 FAILs.

* Thu Mar 15 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-7
- Suggest SELinux permissions problem; no assertion failure anymore (BZ 232371).

* Wed Mar 14 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-6
- Fix occasional dwarf2_read_address: Corrupted DWARF expression (BZ 232353).

* Mon Mar 12 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-5
- Temporary support for shared libraries >2GB on 64bit hosts. (BZ 231832)

* Sun Feb 25 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-4
- Backport + testcase for PPC Power6/DFP instructions disassembly (BZ 230000).

* Mon Feb  5 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-3
- Fix a race during attaching to dying threads; backport (BZ 209445).
- Testcase of unwinding has now marked its unsolvable cases (for BZ 140532).

* Fri Jan 26 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-2
- Backported post gdb-6.6 release PPC `show endian' fixup.
- Fix displaying of numeric char arrays as strings (BZ 224128).
- Simplified patches by merging upstream accepted ones into a single file.

* Sat Jan 20 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.6-1
- Upgrade to GDB 6.6.  Drop redundant patches, forward-port remaining ones.
- Backported post gdb-6.6 release ia64 unwinding fixups.
- Testcase for exec() from threaded program (BZ 202689).

* Mon Jan 15 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-27
- Fix the testsuite results broken in 6.5-26, stop invalid testsuite runs.

* Sat Jan 13 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-26
- Fix unwinding of non-debug (.eh_frame) PPC code, Andreas Schwab (BZ 140532).
- Fix unwinding of debug (.debug_frame) PPC code, workaround GCC (BZ 140532).
- Fix missing testsuite .log output of testcases using get_compiler_info().

* Fri Jan 12 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-25
- Fix unwinding of non-CFI (w/o debuginfo) PPC code by recent GCC (BZ 140532).

* Thu Jan 11 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-24
- Backport readline history for input mode commands like `command' (BZ 215816).

* Tue Jan  9 2007 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-23
- Find symbols properly at their original (included) file (BZ 109921).
- Remove the stuck mock(1) builds disfunctional workaround (-> mock BZ 221351).

* Sat Dec 30 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-22
- Fix unwinding crash on older gcj(1) code (extended CFI support) (BZ 165025).
- Include testcase for the readline history of input mode commands (BZ 215816).

* Sat Dec 23 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-21
- Try to reduce sideeffects of skipping ppc .so libs trampolines (BZ 218379).
- Fix lockup on trampoline vs. its function lookup; unreproducible (BZ 218379).

* Tue Dec 19 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-20
- Fix bogus 0x0 unwind of the thread's topmost function clone(3) (BZ 216711).
- Testcase for readline segfault on excessively long hand-typed lines.

* Tue Dec 12 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-19
- Fix attachment also to a threaded stopped process (BZ 219118).
- Cleanup any leftover testsuite processes as it may stuck mock(1) builds.

* Sat Nov 25 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-18
- Fix readline history for input mode commands like `command' (BZ 215816).

* Thu Nov 16 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-17
- Bugfix testcase typo of gdb-6.5-16.

* Thu Nov 16 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-16
- Provide testcase for accessing the last address space byte.

* Thu Nov  9 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-15
- Fix readline segfault on excessively long hand-typed lines.

* Thu Nov  2 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-14
- Fix "??" resolving of symbols from (non-prelinked) debuginfo packages.
- Fix "??" resolving of symbols from overlapping functions (nanosleep(3)).
- Also disable testcase "checkpoint.exp" for a possible kernel Bug 207002.
- Provided (disabled during build) threading testsuite from BEA.

* Sat Oct 14 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-13
- Fix deadlock accessing last address space byte; for corrupted backtraces.

* Sun Oct  8 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-12
- Disabled IPv6 until its user visible syntax gets stable upstream.

* Sun Oct  1 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-11
- No longer disassemble invalid i386 opcodes of movQ/movA (BZ 172034).
- Simplify the IPv6 patch for gdbserver (BZ 198365).
- Suggest fixing your target architecture for gdbserver(1) (BZ 190810).
- Fix dereferencing registers for 32bit inferiors on 64bit hosts (BZ 181390).
- Fix `gcore' command for 32bit inferiors on 64bit hosts.

* Wed Sep 27 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-10
- Support IPv6 for gdbserver (BZ 198365).
- Temporarily disable testcase "chng-syms.exp" for a possible kernel Bug 207002.

* Thu Sep 21 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-9
- Fix crash on C++ symbol failing to be demangled (BZ 206813).
- Fix attach to stopped process, supersede `gdb-6.3-attach-stop-20051011.patch'.
- Fix TLS symbols resolving for objects with separate .debug file (-debuginfo).
- Fix TLS symbols resolving for shared libraries with a relative pathname.
- Support TLS symbols (+`errno' suggestion if no pthread is found) (BZ 185337).

* Mon Sep 11 2006 Jan Kratochvil <jan.kratochvil@redhat.com> - 6.5-8
- Fix gdb printf command argument using "%%p" (BZ 205551).

* Mon Sep  4 2006 Alexandre Oliva <aoliva@redhat.com> - 6.5-7
- Fix bug in patch for CVE-2006-4146. (BZ 203873, BZ 203880)

* Thu Aug 24 2006 Alexandre Oliva <aoliva@redhat.com> - 6.5-6
- Avoid overflows and underflows in dwarf expression computation stack.
(BZ 203873)

* Thu Aug 24 2006 Alexandre Oliva <aoliva@redhat.com> - 6.5-5
- Backport support for i386 nop memory instructions.
- Fix debuginfo addresses resolving for --emit-relocs Linux kernels
(BZ 203661, from Jan Kratochvil, like the remaining changes).
- Bugfix segv on the source display by ^X 1 (fixes Patch130, BZ
200048).
- Do not step into the PPC solib trampolines (BZ 200533).
- Fix exec() from threaded program, partial CVS backport (BZ 182116).
- Fix occasional failure to load shared libraries (BZ 146810).
- Bugfix object names completion (fixes Patch116, BZ 193763).
- Avoid crash of 'info threads' if stale threads exist (BZ 195429).
- Handle corrupted or missing location list information (BZ 196439).

* Thu Jul 13 2006 Alexandre Oliva <aoliva@redhat.com> - 6.5-3
- Add missing definition of multilib_64_archs for glibc-devel buildreqs.
- Backport support for .gnu.hash sections.

* Wed Jul 12 2006 Alexandre Oliva <aoliva@redhat.com> - 6.5-2
- BuildReq sharutils, prelink and, on multilib systems, 32-bit glibc-devel.
- Drop obsolete attach-stop patch.
- Fix testcases in threaded-watchpoints2 and step-thread-exit patches.
- Re-enable attach-pie.exp, asm-source.exp and sigstep.exp tests.

* Tue Jul 11 2006 Alexandre Oliva <aoliva@redhat.com> - 6.5-1
- Upgrade to GDB 6.5.  Drop redundant patches, forward-port remaining
ones.  Re-enable ada and objc testsuites.

* Thu Jun 15 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.132
- Require flex and bison at build time.
- Additional patch for BZ 175083, to cope with waitpid setting status
even when returning zero.

* Wed May 31 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.131
- Require gettext at build time.  (BZ193366)

* Sat May 27 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.130
- Rewrite patch for BZ 175270, BZ 175083 so as to catch the exception
earlier.
- Remove too-fragile testcases from patches for CFA value and "S"
augmentation.

* Wed May 17 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.129
- Add not-automatically-generated file to fopen64 patch (BZ 191948).

* Fri Apr 14 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.128
- Avoid race conditions caused by exceptions messing with signal masks.
(BZ 175270, BZ 175083, maybe BZ 172938).
- Hardcode /bin and /usr/bin paths into gstack (BZ 179829, BZ 190548).
- Build in a subdir of the source tree instead of in a sibling directory.
- Switch to versioning scheme that uses the same base revision number
for all OSes, and uses a suffix to tell the builds apart and ensure
upgradability.

* Thu Apr 13 2006 Stepan Kasal <skasal@redhat.com> - 6.3.0.0-1.127
- Bump up release number.

* Thu Apr 13 2006 Stepan Kasal <skasal@redhat.com> - 6.3.0.0-1.123
- Use fopen64 where available.  Fixes BZ 178796, BZ 190547.
- Use bigger numbers than int.  Fixes BZ 171783, BZ 179096.

* Wed Mar  8 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.122
- Bump up release number.

* Wed Mar  8 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.119
- Fix regression in PIE debugging (BZ 133944) (re?)introduced by the
prelink fix (BZ 175075, BZ 190545).  Improve testcase for the prelink
fix.
- Revert dwarf2 frame identifier change.

* Tue Mar  7 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.118
- Bump up release number.

* Tue Mar  7 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.115
- Change dwarf2 frame identifiers to use the actual PC instead of the
function's entry point.
- Fix FSF and GDB contact addresses in new testcases.
- Do not try to compile x86_64-only CFA testcase on 32-bit x86.
- Change prelink test to issue untested instead of warning message if
system libraries are not prelinked.

* Fri Mar  3 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.114
- Bump up release number.

* Fri Mar  3 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.111
- Add support for "S" augmentation for signal stack frames.
- Add support for CFA value expressions and encodings.
- Various improvements to the prelink test.

* Thu Feb 23 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.110
- Bump up release number.

* Thu Feb 23 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.107
- Enable gdb to debug core files and executables with mismatched
prelink base addresses.  Fixes BZ 175075, BZ 190545.

* Tue Feb 14 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.106
- Bump up release number.

* Tue Feb 14 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.103
- Adjust type-punning patch to include fix not needed upstream.

* Tue Feb 14 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.102
- Bump up release number.

* Tue Feb 14 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.99
- Use type-punning warning fixes as accepted upstream.

* Fri Feb 10 2006 Jesse Keating <jkeating@redhat.com> - 6.3.0.0-1.98.2
- bump again for double-long bug on ppc(64)

* Tue Feb 07 2006 Jesse Keating <jkeating@redhat.com> - 6.3.0.0-1.98.1
- rebuilt for new gcc4.1 snapshot and glibc changes

* Mon Jan 16 2006 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.98
- Bump up release number.

* Mon Dec 19 2005 Alexandre Oliva <aoliva@redhat.com> - 6.3.0.0-1.94
- Fix type-punning warnings issued by GCC 4.1.

* Fri Dec 16 2005 Jesse Keating <jkeating@redhat.com>
- rebuilt for new gcj

* Thu Dec 01 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.93
- Bump up release number.

* Thu Dec 01 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.90
- Add option to allow backtracing past zero pc value.
- Bugzilla 170275

* Tue Nov 15 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.89
- Bump up release number.

* Tue Nov 15 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.86
- Fix ia64 user-specified SIGILL handling error.
- Bugzilla 165038.

* Tue Oct 18 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.85
- Bump up release number.

* Tue Oct 18 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.82
- Modify attach patch to add missing fclose.
- Bugzilla 166712

* Tue Oct 11 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.81
- Bump up release number.

* Tue Oct 11 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.78
- Support gdb attaching to a stopped process.

* Thu Sep 29 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.77
- Bump up release number.

* Thu Sep 29 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.74
- Fix up DSO read logic when process is attached.

* Mon Sep 26 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.73
- Bump up release number.

* Mon Sep 26 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.70
- Fix frame pointer calculation for ia64 sigtramp frame.

* Thu Sep 22 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.69
- Bump up release number.

* Thu Sep 22 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.66
- Remove extraneous xfree.

* Wed Sep 07 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.65
- Bump up release number.

* Wed Sep 07 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.62
- Readd readnever option

* Wed Jul 27 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.61
- Bump up release number.

* Tue Jul 26 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.57
- Bump up release number.

* Tue Jul 26 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.54
- Add testcase to verify printing of inherited members
- Bugzilla 146835

* Mon Jul 25 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.53
- Bump up release number.

* Mon Jul 25 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.50
- Fix bug with info frame and cursor address on ia64.
- Add testcase to verify pseudo-registers calculated for ia64 sigtramp.
- Bugzilla 160339

* Fri Jul 22 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.49
- Bump up release number.

* Fri Jul 22 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.46
- Fix attaching to 32-bit processes on 64-bit systems.
- Bugzilla 160254

* Thu Jul 14 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.45
- Bump up release number.

* Thu Jul 14 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.42
- Add work-around to make ia64 gcore work faster.
- Bugzilla 147436

* Thu Jul 14 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.41
- Bump up release number.

* Mon Jul 11 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.38
- Fix backtracing across sigaltstack for ia64
- Bugzilla 151741

* Fri Jul 08 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.37
- Bump up release number.

* Fri Jul 08 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.35
- Build pseudo-registers properly for sigtramp frame.
- Bugzilla 160339

* Fri Jul 08 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.34
- Bump up release number.

* Thu Jul 07 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.31
- Modify security errata to include additional bfd robustness updates
- Bugzilla 158680

* Fri Jun 10 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.30
- Bump up release number.

* Fri Jun 10 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.28
- Security errata for bfd and .gdbinit file usage
- Bugzilla 158680

* Wed May 18 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.24
- Bump up release number.

* Wed May 18 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.23
- Bump up release number.

* Wed May 18 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.22
- Specify SA_RESTART for linux-nat.c handlers and use my_waitpid
  which handles EINTR.

* Tue May 03 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.21
- Bump up release number.

* Tue May 03 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.20
- Bump up release number.

* Tue May 03 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.19
- Fix for partial die in cache error
- Bugzilla 137904

* Wed Apr 27 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.18
- Bump up release number.

* Wed Apr 27 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.17
- Bump up release number.

* Wed Apr 27 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.16
- Update ia64 sigtramp support to use libunwind and fix top-level
  rse register reads to also use libunwind.
- Bugzilla 151741

* Thu Apr 21 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.15
- Bump up release number.

* Thu Apr 21 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.14
- Bump up release number.

* Thu Apr 21 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.13
- Do not issue warning message for gcore under ia64
- Bugzilla 146416

* Mon Apr 11 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-1.12
- Update gstack patch, handle systems that lack /proc/<pid>/tasks.

* Fri Apr 8 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-1.11
- Replace patch warning about DW_OP_piece with a patch that implements
  the DW_OP_piece read path.

* Sat Apr 2 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-1.10
- Print a warning when the separate debug info's CRC doen't match;
  test.

* Wed Mar 30 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.9
- Bump up release number.

* Wed Mar 30 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.7
- Add proper vsyscall page support for ia64.

* Thu Mar 24 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.6
- Bump up release number.

* Thu Mar 24 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.4
- Fix printing of inherited members of C++ classes.
- Fix for Bugzilla 146835.

* Tue Mar 22 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.3
- Bump up release number.

* Thu Mar 17 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-1.1
- Remove warnings that cause errors when compiled with gcc4 and -Werror.

* Wed Mar 16 2005 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Fri Mar 04 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.37
- Bump up release number.

* Thu Mar 03 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.35
- Add follow-fork fix from mainline sources.

* Thu Mar 03 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.34
- Bump up release number.

* Mon Feb 28 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.32
- Modify debug register handling for x86, x86-64 to be thread-specific.
- Modify threaded watchpoint code for x86, x86-64 to properly insert
  and remove watchpoints on all threads.

* Tue Feb 22 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-0.31
- Bump version number.

* Tue Feb 22 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-0.30
- Bump version number.

* Tue Feb 22 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-0.29
- Modify gdb-6.3-dwattype0-20050201.patch to check for a zero address
  and not zero unsnd.  Fix BE 32- vs 64-bit problem.

* Mon Feb 21 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-0.28
- Back port patch adding symfile-mem.o to all GNU/Linux builds.
  Fix BZ 146087.

* Wed Feb 16 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.27
- Bump up release number.

* Wed Feb 16 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.26
- Fix unload.exp testcase.

* Mon Feb 14 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.25
- Bump up release number.

* Mon Feb 14 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.24
- Fix gdb to always grab the terminal before a readline call.
- Bugzilla 147880

* Fri Feb 11 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.23
- Bump up release number.

* Fri Feb 11 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.21
- Fix gdb to handle stepping over the end of an exiting thread.
- Bugzilla 146848

* Thu Feb 10 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.20
- Bump up release number.

* Tue Feb 08 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.18
- Modify previous gcore patch to not use linux_proc_xfer_memory even
  for main thread.

* Mon Feb 07 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.17
- Modify previous gcore patch to only apply to ia64.

* Fri Feb 04 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.16
- Fix gcore to work properly for threaded applications
- Bugzilla 145309, 145092

* Fri Feb 04 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.15
- Tolerate DW_AT_type referencing <0> and instead of generating an
  error, treat as unknown type.
- Bugzilla 144852.

* Thu Feb  3 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-0.14
- Separate out test patches.

* Thu Jan 27 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.13
- Fix to allow ia64 gdb to backtrace from syscalls in a corefile.
- Bugzilla 145092.

* Wed Jan 26 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.12
- Fix to support examining files even when the executable moves
- Bugzilla 142122

* Wed Jan 26 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-0.11
- gdb-6.3-ppcdotsyms-20050126.patch: Backport BFD changes for reading
  synthetic symbols.  Rewrite code reading elf minimal symbols so that
  it includes synthetics.

* Fri Jan 21 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.10
- Fix to prevent resetting unwind kernel table size to invalid value
  when debugging a core file
- Bugzilla 145309

* Fri Jan 21 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-0.9
- When single stepping handle both back-to-back and nested signals.
- Disable .symbol patch, results in BFD errors.

* Fri Jan 21 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.8
- Support listing both in-charge and not-in-charge dtors when
  just the dtor name is given.
- Add new test case for newly added ctor/dtor functionality.

* Thu Jan 20 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.7
- Fix to allow breaking by line in both the in-charge and
  not-in-charge ctor/dtor.
- Bugzilla 117826

* Thu Jan 20 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-0.6
- Rebuild.

* Thu Jan 20 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-0.5
- Use bfd_get_synthetic_symtab to read in any synthetic symbols
  such as 64-bit PPC's ".symbol"s.

* Tue Jan 18 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.4
- Modify non-threaded watchpoint patch to use new observer.

* Mon Jan 17 2005 Jeff Johnston <jjohnstn@redhat.com> - 6.3.0.0-0.3
- Fix for non-threaded watchpoints.

* Mon Jan 17 2005 Andrew Cagney <cagney@redhat.com> - 6.3.0.0-0.2
- Enable PPC CFI, remove merged ppc patches.

* Wed Jan 12 2005 Elena Zannoni <ezannoni@redhat.com> - 6.3.0.0-0.1
- commit co-authors Andrew Cagney <cagney@redhat.com> and
  Jeff Johnston <jjohnstn@redhat.com>.
- Various fixes to complete the import and merge.

* Wed Dec 01 2004 Andrew Cagney <cagney@redhat.com> - 6.3.0.0
- Import GDB 6.3, get building, add all patches.

* Tue Nov 30 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.63
- When removing breakpoints, continue removing breakpoints even if an
  error occurs on the list.

* Sun Nov 28 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.62
- Bump version for RHEL4 build.

* Wed Nov 24 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.61
- For PPC-64, fix search for a symbol (wasn't specifying the section).

* Wed Nov 24 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.60
- For PPC-64, do not set malloc name to ".malloc"; no longer needed.
- For all, only define kfail when not already defined.

* Wed Nov 24 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.59
- Bump version.

* Wed Nov 24 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.58
- Add rs6000 reggroups; fixes problem of PS register being trashed
  causing mysterious branch breakpoints.

* Tue Nov 23 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.57
- Backport i386 prolog parser - better backtraces out of semop().
- Add option --readnever to suppress the reading of symbolic debug
  information.
- Add script gstack.sh, installed as gstack.
  Bugzilla 136584, 137121
- Add missing files gdb.pie/attach2.c, gdb.pie/break1.c and
  gdb.pie/Makefile.in along with testsuite/configure stuff for pie.

* Tue Nov 23 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.57
- Backport i386 prolog parser - better backtraces out of semop().
- Add option --readnever to suppress the reading of symbolic debug
  information.
- Add script gstack.sh, installed as gstack.
  Bugzilla 136584, 137121

* Mon Nov 22 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.56
- Bump up release number.

* Mon Nov 22 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.55
- Multiple ia64 backtrace fixes.  Bugzilla 125157

* Thu Nov 11 2004 Elena Zannoni <ezannoni@redhat.com> - 1.200400607.54
- Bump up release number

* Thu Nov 11 2004 Elena Zannoni <ezannoni@redhat.com> - 1.200400607.51
- Modify configure line to not use absolute paths. This was
  creating problems with makeinfo/texinfo.
- Get rid of makeinfo hack.
Bugzilla 135633

* Tue Nov 09 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.50
- Bump up release number

* Tue Nov 09 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.49
- Bump up release number

* Tue Nov 09 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.48
- Expose $base, $allocate constructors and $delete, $base destructors
  for breakpoints.

* Tue Nov 09 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.47
- Enable PPC CFI.

* Mon Nov 08 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.46
- Bump up release number

* Mon Nov 08 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.45
- Bump up release number

* Fri Nov 05 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.44
- Allow macros to continue past a backtrace error

* Tue Oct 26 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.43
- Hack around broken PT_FPSCR defined in headers.
- Import latest s390 fixes.
- Disable sigstep.exp - s390 has problems.
- Use PC's symtab when looking for a symbol.
- Work around DW_OP_piece.

* Fri Oct 22 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.42
- For 64-bit PPC, convert _dl_debug_state descriptor into a code address.
- Fix --ignore option.

* Sun Oct 10 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.40
- Disable attach-pie.exp test, hangs on amd64 without auxv.
- Move pie tests to pie.

* Sun Oct 10 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.39
- Fix comment bug in sigstep.exp.

* Thu Oct 07 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.38
- Do not invalidate cached thread info when resuming threads.
- Bump up release number.

* Fri Oct 01 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.35
- Fix S/390 watchpoint support to work better under threading.

* Fri Oct 01 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.34
- Fix thread_db_get_lwp to handle 2nd format ptids.

* Mon Sep 27 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.33
- Don't unwind past a zero PC (when normal frames).

* Mon Sep 27 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.32
- Add threaded watchpoint support for x86, x86-64, and ia64.

* Mon Sep 27 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.31
- Instead of deleting bigcore.exp, use runtest --ignore.

* Thu Sep 23 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.30
- Merge in mainline testsuite up to 2004-09-23, pick up sig*.exp tests.
  Merge in mainline infrun.c, pick up all infrun.c fixes.
  Generate bigcore's corefile from the running inferior.
  Limit bigcore's corefile to max file-size.

* Thu Sep 02 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.29
- Fix low-level lin-lwp code to wait specifically for any stepping
  LWP (bugzilla 130896)

* Tue Aug 31 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.28
- Add test case for bugzilla 128618 fix.

* Mon Aug 30 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.27
- Add support for breakpoints in manually loaded/unloaded shared libs.
  (bugzilla 128618)

* Mon Aug 30 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.26
- Add java inferior call support.

* Mon Aug 30 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.25
- Convert "main" the function descriptor, into an address.

* Mon Aug 30 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.24
- Fix single-stepping when a signal is pending, was exiting program.
  -- needs kernel fix so that ptrace(PT_STEP,SIG) doesn't do a PT_CONT.
  -- sigstep.exp tests pass with this fix applied.

* Mon Aug 30 2004 Elena Zannoni <ezannoni@redhat.com> - 1.200400607.23
- Delete some part of gdb-6.1post-test-rh.patch, to avoid confusing
  gdb when testing itself, and loading separate debug info.

* Fri Aug 13 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.22
- Check in gdb mainline fix for applications calling clone directly.

* Tue Aug 10 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.21
- Alter libunwind frame code to allow using libunwind 0.97 and up.

* Tue Aug 03 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.20
- Fix the ia64 libunwind test to match current output.

* Fri Jul 30 2004 Elena Zannoni <ezannoni@redhat.com> - 1.200400607.19
- Fix the tests where gdb debugs itself, as to not copy
  the executable to xgdb.

* Mon Jul 26 2004 Elena Zannoni <ezannoni@redhat.com> - 1.200400607.18
- Add Pie patches back in.

* Fri Jul 16 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.17
- Fix stepping over a no-debug shared-library function.
- Fix patch vsyscall patch name.

* Thu Jul 8 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.16
- Update thread code with fix from gdb HEAD

* Wed Jul 7 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.15
- disable vsyscall
- import Bob's crasher fix
- disable bigcore.exp

* Mon Jul 5 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.14
- Make large corefiles work on systems that require O_LARGEFILE.

* Tue Jun 29 2004 Elena Zannoni <ezannoni@redhat.com> - 1.200400607.13
- Fix BuildRequires for libunwind on ia64.

* Mon Jun 28 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.12
- Import wild frame ID patch.  Stops GDB incorrectly matching invalid
  frame IDs.
- Disable bigcore on ia64 and amd64.

* Fri Jun 25 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.11
- Fix testsuite to kill attach process (from corrinna/mainline).
- Fix build problems with vsyscall patch.

* Fri Jun 25 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.10
- Fix annotate test messages.
- Recognize VSYSCALL pages.

* Thu Jun 24 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.200400607.9
- Fix ia64 watchpoint support.

* Wed Jun 23 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.8
- Do not xfail signals on i387, convert KFAIL to FAIL and not XFAIL.

* Wed Jun 23 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.7
- Fix to ppc64 unwinder - handle glibcs function within syscall
  function hack.
- Update sigbpt.exp, ena-dis-br.exp observer.exp signull.exp,
  step-test.exp and sizeof.exp, so that test names are architecture
  clean.
- Disable bigcore.exp on PowerPC 64.

* Tue Jun 22 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.6
- Merge in mainline testsuite changes up to 2004-06-21.
- Re-implement 32 and 64-bit PPC signal trampolines.
- Check i386 and amd64 signal trampolines before dwarf2.
- Allow tramp-frame when there is a symbol.
- Test interaction between single-step, breakpoint and signal.
- ABI: Fix PPC64 function parameters, sizeof long-double, and enum
  return values.

* Mon Jun 21 2004 Elena Zannoni <ezannoni@redhat.com> - 1.200400607.5
- Fix sed line for gz info files.

* Mon Jun 21 2004 Andrew Cagney <cagney@redhat.com> - 1.200400607.4
- Tar/uuencode both the .sum and .log test results.

* Tue Jun 15 2004 Elena Zannoni <ezannoni@redhat.com> - 0.200400607.3
- Remove installation of mmalloc, and its info files.
- Add hack to deal with differring info files generated by makeinfo.
- Restore release number convention.

* Tue Jun 15 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Thu Jun 10 2004 Elena Zannoni <ezannoni@redhat.com> - 0.200400607.2
- Fix Requires and BuildRequires for libunwind dependencies.
- Add patch to silence gcc3.4 warnings.

* Wed Jun 09 2004 Elena Zannoni <ezannoni@redhat.com> - 0.200400607.1
- New import: revamp everything. Remove all patches for now.
- Update the Requires and BuildRequires sections.
- Removed stupid Ada testcases (there is no ada support in gdb yet).

* Mon May 10 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.20
- Disable PIE again.
- obsolete gdb64 only if on ppc64.

* Mon May 03 2004 Jeff Johnston <jjohnstn@redhat.com> - 0.20040223.19
- Add -u parameter to build ChangeLog patch.

* Mon May 03 2004 Jeff Johnston <jjohnstn@redhat.com> - 0.20040223.18
- Update thread fix made for .6 release to FSF version.

* Thu Apr 22 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.17
- Disable PIE again.

* Thu Apr 22 2004 Jeff Johnston <jjohnstn@redhat.com> - 0.20040223.16
- Bump version number.

* Wed Apr 21 2004 Jeff Johnston <jjohnstn@redhat.com> - 0.20040223.15
- fix ia64 info frame command
- also fix ia64 tdep file for which elf header file to include

* Tue Mar 30 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.14
- re-enable pie.

* Tue Mar 30 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.13
- Fix testsuite glitches.

* Wed Mar 24 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.12
- Fix typo.

* Wed Mar 24 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.11
- Make gdb compile w/o warnings with gcc-3.4.
- Reenable PIE support code.

* Tue Mar 23 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.10
- Bump version number

* Tue Mar 23 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.9
- temporarily disable PIE support.
- Add section to obsolete gdb64 package.

* Sun Mar 21 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.8
- Add support for debugging of PIE executables.

* Tue Mar 09 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.7
- Bump version number.

* Mon Mar 08 2004 Jeff Johnston <jjohnstn@redhat.com> - 0.20040223.6
- Fix thread support to recognize new threads even when they reuse
  tids of expired threads.  Also ensure that terminal is held by gdb
  while determining if a thread-create event has occurred.

* Mon Mar 08 2004 Andrew Cagney <cagney@redhat.com> - 0.20040223.5
- Sync with 6.1 branch; eliminate all amd64 patches;
  add more robust 32x64 PPC64 patches.

* Tue Mar 02 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Tue Mar 2 2004 Andrew Cagney <cagney@redhat.com> - 0.20040223.4
- 32x64 fixes that work with threads, replaced old
  non-thread 32x64 patch, add nat patch.

* Wed Feb 25 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.3
- Add patch for x86_64 in 32 bit mode.

* Wed Feb 25 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.2
- Remove ppc64 hacks.
- Refresh some patches.

* Wed Feb 25 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20040223.1
- Import new gdb snapshot from mainline FSF.
- Update patch list.

* Tue Feb 17 2004 Jeff Johnston <jjohnstn@redhat.com> - 1.20031117.8
- Switch ia64-tdep.c to use new abi used by libunwind-0.95 and up.
- Fix gate area specification for ia64-linux-tdep.c.
- Fix long double support for ia64.

* Fri Feb 13 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Thu Jan 08 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20031117.7
- Add fixes for ppc32 support on ppc64 platform, from Andrew Cagney.

* Tue Jan 06 2004 Elena Zannoni <ezannoni@redhat.com> - 0.20031117.6
- Add patch to have unique binary names in the testsuite.
- Disable s390/s390x pthread.exp test (FIXME)
- Don't install any info files for the ppc platform. Let's take them
  from the ppc64 one (or we get install conflicts).
- Remove generated info files from the source tree. They are generated
  as part of the FSF snapshot process.

* Mon Nov 24 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20031117.5
- Add patches from old rpm for i386 support on x86_64.
- Add build dependency on libunwind for ia64.

* Fri Nov 21 2003 Jeremy Katz <katzj@redhat.com> - 0.20031117.4
- more rpm tricks to get the gdb64 package happier

* Thu Nov 20 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20031117.3
- Add sick and twisted workaround for ppc64 architecture.

* Wed Nov 19 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20031117.2
- Fix typo in libunwind test.

* Tue Nov 18 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20031117.1
- Import new gdb snapshot from mainline FSF.
- Fix some testfiles.
- Add fixes for gcore, and patch for libunwind support on ia64.
- Add tests to see what versions of gcc, binutils, glibc and kernel we
  are running with.

* Wed Oct 15 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.41
- Bump up version number.

* Wed Sep 24 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.40
- Fix problem with gcore and single threaded programs. (bugzilla 103531)

* Mon Sep 22 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20030710.39
- Fix call to quit_target from quit_force.

* Sun Sep 21 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.38
- Fix PPC64 push dummy call.
- Re-fix PPC64 return value (had wrong / old patch).

* Sat Sep 20 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.37
- Fix PPC32 return values.

* Sat Sep 20 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.36
- Rewrite ppc64 retun value methods so that they (hopefully)
match the SysV spec.
- Enable ppc64 testsuite.

* Thu Sep 18 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.35
- Hack around problem "break main" vs "break .main" when there is
only a minimal ppc64 symbol table.  The former is a function descriptor
and not where you want the breakpoint to go.  Only convert descriptors
to pointers when the address is in the ".opd" section.

* Wed Sep 17 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.34
- Fix ppc32 push_dummy_call.

* Tue Sep 16 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.33
- Pack gdb.sum and gdb.log using uuencode and bzip.

* Tue Sep 16 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20030710.32
- Catch errors when quitting so exit of gdb still occurs.

* Mon Sep 15 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.31
- Fix ppc32 use_struct_convention.

* Thu Sep 11 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.30
- Mods to dwarf2-frame.c to work around a lack of GCC/CFI info.

* Thu Sep 11 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.29
- Bump up version number.

* Wed Sep 10 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.28
- Fix a core dump with MI.
- Add new ChangeLog patch for mi changes.

* Thu Sep 04 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.27
- Change the name of the package to gdb64 in ppc64 case.

* Tue Aug 26 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.26
- Add testcase for separate debug info.

* Tue Aug 26 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.25
- fix i386 on x86-64 TLS
- add "base-aug2003" suffix to older x86i386 patch

* Tue Aug 26 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.24
- skip the ppc64 and x86-64 frame redzone.

* Fri Aug 22 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.23
- Relax one testcase in selftest.exp a bit.
- Accept different output as well in thread bt (platform dependent).
- Enable testsuite run for ia64, ppc, s390 and s390x. They are in
  reasonably good shape.

* Thu Aug 21 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20030710.22
- Multiple ia64 fixes.
- Fix ia64 printing of function pointers.
- Fix ia64 prologue examination to ignore predicated insns if we
  haven't found the return address yet.
- Skip dump.exp testcase for ia64

* Thu Aug 21 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.21
- Bump release number.

* Wed Aug 20 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.20
- Relax pattern in annota2.exp test.

* Wed Aug 20 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.19
- rename gdb binary to gdb64 for ppc64 platform.

* Tue Aug 19 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20030710.18
- Fix ia64 pc unwinding to include psr slot.

* Mon Aug 18 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.17
- Fix info installation for annotate.texi. (Bugzilla 102521)

* Fri Aug 15 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.16
- revamp tls tests a bit.
- Handle new output from gdb in relocate.exp

* Wed Aug 13 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.15
- Fix problem for processing of separate debug info files.

* Wed Aug 13 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20030710.14
- add ia64.inc file for testing ia64 in gdb.asm testsuite

* Fri Aug 8 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.13
- print the libthread_db library path, print when threads are enabled

* Thu Aug 7 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.12
- "cat" the test log into the build log

* Wed Aug 06 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20030710.11
- modernize ia64 gdb to use new frame model
- remove/replace deprecated interfaces used by ia64 gdb

* Wed Aug 06 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.10
- Sync to gdb-5.3.90-sync-20030806.patch.

* Tue Jul 29 2003 Andrew Cagney <cagney@redhat.com> - 0.20030710.9
- add x86-64 i386 fixes

* Tue Jul 29 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.8
- Fix some tests by xfailing the correct target triplet for RedHat.
- Remove include of config.h from pthreads.c testcases.

* Mon Jul 28 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.7
- Fix some test failures, by escaping correctly.

* Thu Jul 24 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.6
- Remove one testcase that is redundant.

* Wed Jul 23 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.5
- Bump up release number.

* Wed Jul 23 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.4
- Bring in sync with current head of gdb-6 branch.
- Remove linespec patch, because included in the new sync patch.

* Fri Jul 18 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.3
- Add patch to avoid gdb segfault with bad debug info.
- Change location of build tree to avoid conflicts with older versions
  possibly installed.

* Thu Jul 17 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.2
- Add patch to synchronize the current snapshot with the gdb-6 branch head.
- Remove some patches that are includd in such diff.
- Enable tests on AMD64 as well.

* Fri Jul 11 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20030710.1
- Import new gdb snapshot.
- Revamp gdb.spec. Get rid of patches that apply to older versions.
- Add patches for ppc64 support, kfail and make gdb more robust in copingi
  with bad debug info.

* Wed Jul 02 2003 Jeff Johnston <jjohnstn@redhat.com> - 1.20021129.39
- Fix bug with ia64 checking of hardware breakpoints.

* Mon Jun 30 2003 Elena Zannoni <ezannoni@redhat.com> - 1.20021129.38
- Add necessary function for NPTL support on x86-64.

* Wed Jun 04 2003 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Wed Jun 04 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.37
* Tue Jun 03 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.36
- Enable warnings for x86_64, not x86-64.
- Fix warnings from infptrace.c and dwarfread.c.
- Print error message only when reading separate debug info really
  doesn't work (jimb@redhat.com).

* Fri May 23 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.35
- Fixes for fetching and storing access registers on s390x (jimb@redhat.com).
  Bugzilla 91455.

* Wed May 21 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20021129.34
- Do not generate error on detach failure.  Bugzilla 90900.

* Thu May 8 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.33
- New tests for asm on s390x (jimb@redhat.com). Bugzilla 90503.
- Fixes for prologue analysis on s390x (jimb@redhat.com). Bugzilla 90506.
- bfd fix for 64-bit platforms (jimb@redhat.com).
- Disable ppc64 builds until we have a port.

* Thu May 1 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20021129.32
- Add ia64 support to the float.exp testcase.

* Thu May 1 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.31
- Clean up the tls tests some more.
- Fix problem with non US-eng locale. Bugzilla bug 88823.

* Wed Apr 30 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20021129.30
- Fix ia64 prologue skipping.
- Fix ia64 line table.
- Fix setting of prev_pc in infrun.c.

* Mon Mar 31 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.29
- Include the gcore script, as gdb_gcore.sh and install it in
  /usr/bin as gcore.
- One more disassembly fix for core files. Added to
  gdb-5.3post-disasm-mar2003.patch. Bugzilla 87677.
- Enable build warnings for x86-64.

* Mon Mar 31 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.28
- Fix Java strings printing.
- Fix memory corruption in disassembly code. Bugzilla 85644.
- Testsuite fixes (jimb@redhat.com). Bugzilla 85457.
- Fixes for s390 stack handling (jimb@redhat.com). Bugzilla 85039.
- Fixes for s390 struct return (jimb@redhat.com).

* Wed Mar 26 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.27
- Fixes for disassembly of code in threaded applications. Bugzilla 87495.
- Fixes for s390 prologue analysis. (jimb@redhat.com).
  Bugzilla bugs 85251, 85214.

* Thu Mar 20 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.26
- Fix inferior function calls with void return on x86-64. Bugzilla bug 83197.
- Fix for upstream PR/699.
- Fix some problems with gdb-5.3post-thrtst-feb2003.patch.

* Wed Mar 19 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20021129.25
- Fix for thread-db.c: check_event() - Bugzilla bug 86231.

* Fri Mar 14 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.24
- Fix some problems with inferior function calls on x86-64.

* Fri Mar 07 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.23
- testsuite patches. Bugzilla 85215 85028 85335.

* Thu Mar 06 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.22
- Fix testsuite problems related to having '+' in the directory name.
  Bugzilla 85031.

* Mon Mar 03 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.21
- Fix a few inferior function call problems.

* Mon Mar 03 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.20
- Split the changelog patches in two. Cleanup messy patch section.

* Thu Feb 27 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20021129.19
- Perform run-time check for tkill syscall in lin-lwp.c.

* Mon Feb 24 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.18
- Update copyright year printed in version.

* Mon Feb 24 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.17
- Refresh build.

* Mon Feb 24 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.16
- Add some testsuite cleanups, to avoid spurious test failures.

* Fri Feb 21 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20021129.15
- Add patch to handle thread exiting when LD_ASSUME_KERNEL=2.4.1 which
  fixes Bugzilla bug 84217.

* Fri Feb 21 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.14
- New patch to fix disassembly on s390. Bugzilla bug 84286.
- New patch for attach/ptrace fix. Bugzilla bug 84220.
- Reenable tests for x86.

* Thu Feb 20 2003 Jeff Johnston <jjohnstn@redhat.com> - 0.20021129.13
- Add patch for mixed stabs with dwarf2 - bugzilla bug 84253.

* Wed Feb 12 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.12
- Disable tests also for x86.

* Tue Feb 11 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.11
- Add patch for mi threads tests.
- Add patch for dwarf2 debug_ranges section.
- Add patch for detach bug.

* Mon Feb 10 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.10
- Add patch for testsuite auto answering internal error queries.
- Add new TLS tests.
- Add cleanup patches for thread tests.

* Mon Feb 03 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.9
- Add new patch for thread support. Apply on all arches.
- Do not apply old patches, but leave them around for now.
- Add new patch for dwarf2 debug info reading.
- Add new patch for dwarf2 cfi engine cleanup.
- Add new patch for uiout problems.
- Add new patch for s390 build.
- Disable tests on all platforms but x86.

* Mon Jan 27 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.8
- Move all the changelog entries to a single patch.
- Add tests to the args patch.
- Add new patch for until command fix (bugzilla Bug 19890).
- s390 and s390x can be built with -Werror.
- Run make check for s390 and s390x too.
- Include an updated version of the thread nptl patch (still WIP).

* Wed Jan 15 2003 Phil Knirsch <pknirsch@redhat.com> - 0.20021129.7
- Apply the 2nd misc patch for s390 and s390x, too.

* Tue Jan 14 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.6
- Add patches for NPTL support, to be applied on i386 only.
  (this is still WIP)
- Split old misc patch in two parts.
- Temporarily disable testsuite run on alpha.

* Sun Jan 12 2003 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.5
- Add patch for --args with zero-length arguments. Fix for bug 79833.

* Tue Dec 17 2002 Elliot Lee <sopwith@redhat.com> - 0.20021129.4
- The define directive to rpm is significant even if the line it is
  in happens to start with a '#' character. Fixed.

* Fri Dec 13 2002 Elena Zannoni <ezannoni@redhat.com> - 0.20021129.3
- Merge previous patches for warnings into a single one.
- Add changelogs to patches.
- Add, but don't use, a macro to avoid stripping.

* Fri Dec  6 2002 Elena Zannoni <ezannoni@redhat.com>
- Add patch to allow debugging of executables with debug info stored
  in separate files.
- Add patch for Makefile dependencies and disable warnings for
  building thread-db.c.
- Re-enable building with -Werror for alpha, ia64, ppc.

* Mon Dec  2 2002 Elena Zannoni <ezannoni@redhat.com>
- Don't pass to gdb an empty build warnings flag, or that will disable warnings
  completely. We want to build using gdb's standard warnings instead.

* Mon Dec  2 2002 Elena Zannoni <ezannoni@redhat.com>
- Don't do testing for x86_64.

* Sun Dec  1 2002 Elena Zannoni <ezannoni@redhat.com>
- x86_64 doesn't build with Werror yet.
- Add patch for alpha.
- Alpha doesn't build with -Werror either.
- Add patch for ia64.
- Add patch for ppc.
- Drop ia64 from -Werror list.
- Drop ppc from -Werror list.

* Sun Dec  1 2002 Elena Zannoni <ezannoni@redhat.com>
- Add dejagnu to the build requirements.
- Enable make check.
- Add enable-gdb-build-warnings to the configure flags.

* Fri Nov 29 2002 Elena Zannoni <ezannoni@redhat.com>
- Import new upstream sources.
- Change version and release strings.
- Upgrade patches.
- Build gdb/gdbserver as well.
- Define and use 'cvsdate'.
- Do %%setup specifying the source directory name.
- Don't cd up one dir before removing tcl and friends.
- Change the configure command to allow for the new source tree name.
- Ditto for the copy of NEWS.
- Add some comments.

* Mon Nov 25 2002 Elena Zannoni <ezannoni@redhat.com> 5.2.1-5
General revamp.
- Add patch for gdb/doc/Makefile.in. Part of fix for bug 77615.
- Add patch for mmalloc/Makefile.in. Part of fix for bug 77615.
- Change string printed in version.in to <version>-<release>rh.
- Move the deletion of dejagnu, expect, tcl to the prep section,
  from the build section.
- Add build directory housekeeping to build section.
- Use macros for configure parameters.
- Do the build in a separate directory.
- Prepare for testing, but not enable it yet.
- Correctly copy the NEWS file to the top level directory, for the doc
  section to find it.
- Cd to build directory before doing install.
- Use makeinstall macro, w/o options.
- Remove workaround for broken gdb info files. Part of fix for bug 77615.
- Remove share/locale directory, it is in binutils.
- Remove info/dir file.
- Clarify meaning of post-install section.
- Add gdbint info files to post-install, pre-uninstall and files sections.
  Part of fix for bugs 77615, 76423.
- Add libmmalloc.a to package.

* Fri Aug 23 2002 Florian La Roche <Florian.LaRoche@redhat.de>
- added mainframe patch from developerworks

* Wed Aug 21 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.2.1-3
- Add changelogs to the previous patch

* Wed Aug 14 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.2.1-2
- Add some patches from Elena Zannoni <ezannoni@redhat.com>

* Tue Jul 23 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.2.1-1
- 5.2.1

* Mon Jul 22 2002 Florian La Roche <Florian.LaRoche@redhat.de>
- compile on mainframe

* Mon Jul  8 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.2-3
- Rebuild

* Tue May  7 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.2-2
- Rebuild

* Mon Apr 29 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.2-1
- 5.2

* Mon Apr 29 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.1.92-1
- 5.1.92. Hopefully identical to 5.2 final

* Mon Apr 22 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.1.91-1
- 5.1.91. 5.2 expected in a week

* Thu Mar 28 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.1.90CVS-5
- Update to current

* Thu Mar 28 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.1.90CVS-4
- Update to current

* Thu Mar 28 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.1.90CVS-3
- Update to current

* Wed Mar 20 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.1.90CVS-2
- Update to current

* Wed Mar 13 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.1.90CVS-1
- Update to current 5.2 branch

* Thu Jan 24 2002 Trond Eivind Glomsrod <teg@redhat.com> 5.1.1-1
- 5.1.1
- add URL

* Wed Jan 09 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Mon Dec 10 2001 Trond Eivind Glomsrod <teg@redhat.com> 5.1-2
- Fix some thread+fpu problems

* Mon Nov 26 2001 Trond Eivind Glomsrod <teg@redhat.com> 5.1-1
- 5.1

* Mon Nov 19 2001 Trond Eivind Glomsrod <teg@redhat.com> 5.0.94-0.71
- 5.0.94. Almost there....

* Mon Nov 12 2001 Trond Eivind Glomsrod <teg@redhat.com> 5.0.93-2
- Add patch from jakub@redhat.com to improve handling of DWARF

* Mon Nov 12 2001 Trond Eivind Glomsrod <teg@redhat.com> 5.0.93-1
- 5.0.93
- handle missing info pages in post/pre scripts

* Wed Oct 31 2001 Trond Eivind Glomsrod <teg@redhat.com> 5.0.92-1
- 5.0.92

* Fri Oct 26 2001 Trond Eivind Glomsrod <teg@redhat.com> 5.0.91rh-1
- New snapshot
- Use the 5.0.91 versioning from the snapshot

* Wed Oct 17 2001 Trond Eivind Glomsrod <teg@redhat.com> 5.0rh-17
- New snapshot

* Thu Sep 27 2001 Trond Eivind Glomsrod <teg@redhat.com>
- New snapshot

* Wed Sep 12 2001 Trond Eivind Glomsrod <teg@redhat.com> 5.0rh-16
- New snapshot

* Mon Aug 13 2001 Trond Eivind Glomsrod <teg@redhat.com> 5.0rh-15
- Don't buildrequire compat-glibc (#51690)

* Thu Aug  9 2001 Trond Eivind Glomsrod <teg@redhat.com>
- New snapshot, from the stable branch eventually leading to gdb 5.1

* Mon Jul 30 2001 Trond Eivind Glomsrod <teg@redhat.com>
- s/Copyright/License/
- Add texinfo to BuildRequires

* Mon Jun 25 2001 Trond Eivind Glomsrod <teg@redhat.com>
- New snapshot

* Fri Jun 15 2001 Trond Eivind Glomsrod <teg@redhat.com>
- New snapshot
- Add ncurses-devel to buildprereq
- Remove perl from buildprereq, as gdb changed the way
  version strings are generated

* Thu Jun 14 2001 Trond Eivind Glomsrod <teg@redhat.com>
- New snapshot

* Wed May 16 2001 Trond Eivind Glomsrod <teg@redhat.com>
- New snapshot - this had thread fixes for curing #39070
- New way of specifying version

* Tue May  1 2001 Trond Eivind Glomsrod <teg@redhat.com>
- New tarball
- Kevin's patch is now part of gdb

* Mon Apr  9 2001 Trond Eivind Glomsrod <teg@redhat.com>
- Add patch from kevinb@redhat.com to fix floating point + thread
  problem (#24310)
- remove old workarounds
- new snapshot

* Thu Apr  5 2001 Trond Eivind Glomsrod <teg@redhat.com>
- New snapshot

* Sat Mar 17 2001 Bill Nottingham <notting@redhat.com>
- on ia64, there are no old headers :)

* Fri Mar 16 2001 Trond Eivind Glomsrod <teg@redhat.com>
- build with old headers, new compiler

* Fri Mar 16 2001 Trond Eivind Glomsrod <teg@redhat.com>
- new snapshot

* Mon Feb 26 2001 Trond Eivind Glomsrod <teg@redhat.com>
- new snapshot which should fix some more IA64 problems (#29151)
- remove IA64 patch, it's now integrated

* Wed Feb 21 2001 Trond Eivind Glomsrod <teg@redhat.com>
- add IA64 and Alpha patches from Kevin Buettner <kevinb@redhat.com>
- use perl instead of patch for fixing the version string

* Tue Feb 20 2001 Trond Eivind Glomsrod <teg@redhat.com>
- don't use kgcc anymore
- mark it as our own version
- new snapshot

* Mon Jan 22 2001 Bernhard Rosenkraenzer <bero@redhat.com>
- Link with ncurses 5.x even though we're using kgcc.
  No need to drag in requirements on ncurses4 (Bug #24445)

* Fri Jan 19 2001 Trond Eivind Glomsrod <teg@redhat.com>
- new snapshot

* Wed Dec 20 2000 Trond Eivind Glomsrod <teg@redhat.com>
- new snapshot

* Mon Dec 04 2000 Trond Eivind Glomsrod <teg@redhat.com>
- new snapshot
- new alpha patch - it now compiles everywhere. Finally.

* Fri Dec 01 2000 Trond Eivind Glomsrod <teg@redhat.com>
- new snapshot

* Mon Nov 20 2000 Trond Eivind Glomsrod <teg@redhat.com>
- new CVS snapshot
- disable the patches
- don't use %%configure, as it confuses the autoconf script
- enable SPARC, disable Alpha


* Wed Aug 09 2000 Trond Eivind Glomsrod <teg@redhat.com>
- added patch from GDB team for C++ symbol handling

* Tue Jul 25 2000 Trond Eivind Glomsrod <teg@redhat.com>
- upgrade to CVS snapshot
- excludearch SPARC, build on IA61

* Wed Jul 19 2000 Trond Eivind Glomsrod <teg@redhat.com>
- rebuild

* Thu Jul 13 2000 Prospector <bugzilla@redhat.com>
- automatic rebuild

* Sun Jul 02 2000 Trond Eivind Glomsrod <teg@redhat.com>
- rebuild

* Thu Jun 08 2000 Trond Eivind Glomsrod <teg@redhat.com>
- use %%configure, %%makeinstall, %%{_infodir}, %%{_mandir},
  and %%{_tmppath}
- the install scripts  for info are broken(they don't care about
  you specify in the installstep), work around that.
- don't build for IA64

* Mon May 22 2000 Trond Eivind Glomsrod <teg@redhat.com>
- upgraded to 5.0 - dump all patches. Reapply later if needed.
- added the NEWS file to the %%doc files
- don't delete files which doesn't get installed (readline, texinfo)
- let build system handle stripping and gzipping
- don't delete libmmalloc
- apply patch from jakub@redhat.com to make it build on SPARC

* Fri Apr 28 2000 Matt Wilson <msw@redhat.com>
- rebuilt against new ncurses

* Tue Mar  7 2000 Jeff Johnson <jbj@redhat.com>
- rebuild for sparc baud rates > 38400.

* Tue Feb  8 2000 Jakub Jelinek <jakub@redhat.com>
- fix core file handling on i386 with glibc 2.1.3 headers

* Fri Jan 14 2000 Jakub Jelinek <jakub@redhat.com>
- fix reading registers from core on sparc.
- hack around build problems on i386 with glibc 2.1.3 headers

* Thu Oct 7 1999 Jim Kingdon
- List files to install in /usr/info specifically (so we don't pick up
things like info.info from GDB snapshots).

* Thu Oct 7 1999 Jim Kingdon
- Update GDB to 19991004 snapshot.  This eliminates the need for the
sigtramp, sparc, xref, and threads patches.  Update sparcmin patch.

* Mon Aug 23 1999 Jim Kingdon
- Omit readline manpage.

* Sat Aug  7 1999 Jim Kingdon
- Remove H.J. Lu's patches (they had been commented out).
- Add sigtramp patch (from gdb.cygnus.com) and threads patch (adapted
from code fusion CD-ROM).

* Wed Apr 14 1999 Jeff Johnson <jbj@redhat.com>
- merge H.J. Lu's patches into 4.18.

* Mon Apr 05 1999 Cristian Gafton <gafton@redhat.com>
- updated the kern22 patch with stuff from davem

* Thu Apr  1 1999 Jeff Johnson <jbj@redhat.com>
- sparc with 2.2 kernels no longer uses sunos ptrace (davem)

* Sun Mar 21 1999 Cristian Gafton <gafton@redhat.com>
- auto rebuild in the new build environment (release 3)

* Mon Mar  8 1999 Jeff Johnson <jbj@redhat.com>
- Sparc fiddles for Red Hat 6.0.
