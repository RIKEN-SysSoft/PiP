# How to build this RPM:
#
#	$ rpm -Uvh glibc-2.17-106.el7_2.8.src.rpm 
#	$ cd .../$SOMEWHERE/.../PiP-glibc
#	$ git diff origin/centos/2.17/master pip-centos7 >~/rpmbuild/SOURCES/glibc-pip.patch
#	$ rpmbuild -bb ~/rpmbuild/SPECS/pip-glibc.spec
#

%define	pip_glibc_dir		/opt/pip
%define	pip_glibc_release	pip0

# disable strip, otherwise the pip-gdb shows the following error:
#	warning: Unable to find libthread_db matching inferior's thread library, thread debugging will not be available.
%define __spec_install_post /usr/lib/rpm/brp-compress
%define __os_install_post /usr/lib/rpm/brp-compress
%define debug_package %{nil}

%define glibcsrcdir glibc-2.17-c758a686
%define glibcversion 2.17
%define glibcrelease 106%{?dist}.8.%{pip_glibc_release}
##############################################################################
# If run_glibc_tests is zero then tests are not run for the build.
# You must always set run_glibc_tests to one for production builds.
%define run_glibc_tests 0
##############################################################################
# Auxiliary arches are those arches that can be built in addition
# to the core supported arches. You either install an auxarch or
# you install the base arch, not both. You would do this in order
# to provide a more optimized version of the package for your arch.
%define auxarches athlon alphaev6
%define xenarches i686 athlon
##############################################################################
# We build a special package for Xen that includes TLS support with
# no negative segment offsets for use with Xen guests. This is
# purely an optimization for increased performance on those arches.
%ifarch %{xenarches}
%define buildxen 1
%define xenpackage 0
%else
%define buildxen 0
%define xenpackage 0
%endif
##############################################################################
# In RHEL7 for 32-bit and 64-bit POWER the following runtimes are provided:
# - POWER7 (-mcpu=power7 -mtune=power7)
# - POWER8 (-mcpu=power7 -mtune=power8)
#
# We will eventually make the POWER8-tuned runtime into a POWER8 runtime when
# there is enough POWER8 hardware in the build farm to ensure that all
# builds are done on POWER8.
#
# The POWER5 and POWER6 runtimes are now deprecated and no longer provided
# or supported. This means that RHEL7 will only run on POWER7 or newer
# hardware.
#
%ifarch ppc %{power64}
%define buildpower6 0
%define buildpower8 1
%else
%define buildpower6 0
%define buildpower8 0
%endif
##############################################################################
# We build librtkaio for all rtkaioarches. The library is installed into
# a distinct subdirectory in the lib dir. This define enables the rtkaio
# add-on during the build. Upstream does not have rtkaio and it is provided
# strictly as part of our builds.
%define rtkaioarches %{ix86} x86_64 ppc %{power64} s390 s390x
##############################################################################
# Any architecture/kernel combination that supports running 32-bit and 64-bit
# code in userspace is considered a biarch arch.
%define biarcharches %{ix86} x86_64 ppc %{power64} s390 s390x
##############################################################################
# If the debug information is split into two packages, the core debuginfo
# pacakge and the common debuginfo package then the arch should be listed
# here. If the arch is not listed here then a single core debuginfo package
# will be created for the architecture.
%define debuginfocommonarches %{biarcharches} alpha alphaev6
##############################################################################
# If the architecture has multiarch support in glibc then it should be listed
# here to enable support in the build. Multiarch support is a single library
# with implementations of certain functions for multiple architectures. The
# most optimal function is selected at runtime based on the hardware that is
# detected by glibc. The underlying support for function selection and
# execution is provided by STT_GNU_IFUNC.
%define multiarcharches ppc %{power64} %{ix86} x86_64 %{sparc} s390 s390x
##############################################################################
# Add -s for a less verbose build output.
%define silentrules PARALLELMFLAGS=
##############################################################################
# %%package glibc - The GNU C Library (glibc) core package.
##############################################################################
Summary: The GNU libc libraries
Name: pip-glibc
Version: %{glibcversion}
Release: %{glibcrelease}
# GPLv2+ is used in a bunch of programs, LGPLv2+ is used for libraries.
# Things that are linked directly into dynamically linked programs
# and shared libraries (e.g. crt files, lib*_nonshared.a) have an additional
# exception which allows linking it into any kind of programs or shared
# libraries without restrictions.
License: LGPLv2+ and LGPLv2+ with exceptions and GPLv2+
Group: System Environment/Libraries
URL: http://www.gnu.org/software/glibc/
# We do not use usptream source tarballs as the start place for our package.
# We should use upstream source tarballs for official releases though and
# it will look like this:
# Source0: http://ftp.gnu.org/gnu/glibc/%%{glibcsrcdir}.tar.gz
# Source1: %%{glibcsrcdir}-releng.tar.gz
# TODO:
# The Source1 URL will never reference an upstream URL. In fact the plan
# should be to merge the entire release engineering tarball into upstream
# instead of keeping it around as a large dump of files. Distro specific
# changes should then be a very very small patch set.
Source0: %{?glibc_release_url}%{glibcsrcdir}.tar.gz
Source1: %{glibcsrcdir}-releng.tar.gz

##############################################################################
# Start of glibc patches
##############################################################################
# 0000-0999 for patches which are unlikely to ever go upstream or which
# have not been analyzed to see if they ought to go upstream yet.
#
# 1000-2000 for patches that are already upstream.
#
# 2000-3000 for patches that are awaiting upstream approval
#
# Yes, I realize this means some gratutious changes as patches to from
# one bucket to another, but I find this scheme makes it easier to track
# the upstream divergence and patches needing approval.
#
# Note that we can still apply the patches in any order we see fit, so
# the changes from one bucket to another won't necessarily result in needing
# to twiddle the patch because of dependencies on prior patches and the like.

##############################################################################
#
# Patches that are unlikely to go upstream or not yet analyzed.
#
##############################################################################

# Configuration twiddle, not sure there's a good case to get upstream to
# change this.
Patch0001: glibc-fedora-nscd.patch

Patch0002: glibc-fedora-regcomp-sw11561.patch
Patch0003: glibc-fedora-ldd.patch

Patch0004: glibc-fedora-ppc-unwind.patch

# Build info files in the source tree, then move to the build
# tree so that they're identical for multilib builds
Patch0005: glibc-rh825061.patch

# Horrible hack, never to be upstreamed.  Can go away once the world
# has been rebuilt to use the new ld.so path.
Patch0006: glibc-arm-hardfloat-3.patch

# Needs to be sent upstream
Patch0008: glibc-fedora-getrlimit-PLT.patch
Patch0009: glibc-fedora-include-bits-ldbl.patch

# stap, needs to be sent upstream
Patch0010: glibc-stap-libm.patch

# Needs to be sent upstream
Patch0029: glibc-rh841318.patch

# All these were from the glibc-fedora.patch mega-patch and need another
# round of reviewing.  Ideally they'll either be submitted upstream or
# dropped.
Patch0012: glibc-fedora-linux-tcsetattr.patch
Patch0014: glibc-fedora-nptl-linklibc.patch
Patch0015: glibc-fedora-localedef.patch
Patch0016: glibc-fedora-i386-tls-direct-seg-refs.patch
Patch0017: glibc-fedora-gai-canonical.patch
Patch0019: glibc-fedora-nis-rh188246.patch
Patch0020: glibc-fedora-manual-dircategory.patch
Patch0024: glibc-fedora-locarchive.patch
Patch0025: glibc-fedora-streams-rh436349.patch
Patch0028: glibc-fedora-localedata-rh61908.patch
Patch0030: glibc-fedora-uname-getrlimit.patch
Patch0031: glibc-fedora-__libc_multiple_libcs.patch
Patch0032: glibc-fedora-elf-rh737223.patch
Patch0033: glibc-fedora-elf-ORIGIN.patch
Patch0034: glibc-fedora-elf-init-hidden_undef.patch

# Needs to be sent upstream
Patch0035: glibc-rh911307.patch
Patch0036: glibc-rh892777.patch
Patch0037: glibc-rh952799.patch
Patch0038: glibc-rh959034.patch
Patch0039: glibc-rh970791.patch

# GLIBC_PTHREAD_STACKSIZE - Needs to be upstreamed on top of the future
# tunables framework.
Patch0040: glibc-rh990388.patch
Patch0041: glibc-rh990388-2.patch
Patch0042: glibc-rh990388-3.patch
Patch0043: glibc-rh990388-4.patch

# Remove non-ELF support in rtkaio
Patch0044: glibc-rh731833-rtkaio.patch
Patch0045: glibc-rh731833-rtkaio-2.patch

# Add -fstack-protector-strong support.
Patch0048: glibc-rh1070806.patch

Patch0060: glibc-aa64-commonpagesize-64k.patch

Patch0061: glibc-rh1133812-1.patch

Patch0062: glibc-cs-path.patch

# Use __int128_t in link.h to support older compilers.
Patch0063: glibc-rh1120490-int128.patch

# Workaround to extend DTV_SURPLUS. Not to go upstream.
Patch0066: glibc-rh1227699.patch

# CVE-2015-7547
Patch0067: glibc-rh1296031.patch

##############################################################################
#
# Patches from upstream
#
##############################################################################

Patch1000: glibc-rh905877.patch
Patch1001: glibc-rh958652.patch
Patch1002: glibc-rh977870.patch
Patch1003: glibc-rh977872.patch
Patch1004: glibc-rh977874.patch
Patch1005: glibc-rh977875.patch
Patch1006: glibc-rh977887.patch
Patch1007: glibc-rh977887-2.patch
Patch1008: glibc-rh980323.patch
Patch1009: glibc-rh984828.patch
Patch1010: glibc-rh966633.patch

# Additional backports from upstream to fix problems caused by
# -ftree-loop-distribute-patterns
Patch1011: glibc-rh911307-2.patch
Patch1012: glibc-rh911307-3.patch

# PowerPC backports
Patch1013: glibc-rh977110.patch
Patch1014: glibc-rh977110-2.patch

# HWCAPS2 support and POWER8 additions to it
Patch1015: glibc-rh731833-hwcap.patch
Patch1016: glibc-rh731833-hwcap-2.patch
Patch1017: glibc-rh731833-hwcap-3.patch
Patch1018: glibc-rh731833-hwcap-4.patch
Patch1019: glibc-rh731833-hwcap-5.patch

# Miscellaneous fixes for PowerPC
Patch1020: glibc-rh731833-misc.patch
Patch1021: glibc-rh731833-misc-2.patch
Patch1022: glibc-rh731833-misc-3.patch
Patch1023: glibc-rh731833-misc-4.patch
Patch1024: glibc-rh731833-misc-5.patch
Patch1025: glibc-rh731833-misc-6.patch

# Math fixes for PowerPC
Patch1026: glibc-rh731833-libm.patch
Patch1027: glibc-rh731833-libm-2.patch
Patch1028: glibc-rh731833-libm-3.patch
Patch1029: glibc-rh731833-libm-4.patch
Patch1030: glibc-rh731833-libm-5.patch
Patch1031: glibc-rh731833-libm-6.patch
Patch1032: glibc-rh731833-libm-7.patch

Patch1034: glibc-rh996227.patch
Patch1035: glibc-rh1000923.patch
Patch1036: glibc-rh884008.patch
Patch1037: glibc-rh1008298.patch
# Add support for rtlddir distinct from slibdir.
Patch1038: glibc-rh950093.patch
Patch1039: glibc-rh1025612.patch
Patch1040: glibc-rh1032435.patch
Patch1041: glibc-rh1020637.patch
# Power value increase for MINSIGSTKSZ and SIGSTKSZ.
Patch1042: glibc-rh1028652.patch

# Upstream BZ 15601
Patch1043: glibc-rh1039496.patch

Patch1044: glibc-rh1047983.patch

Patch1045: glibc-rh1064945.patch

# Patch to update the manual to say SunRPC AUTH_DES will prevent FIPS 140-2
# compliance.
Patch1046: glibc-rh971589.patch
# Patch to update translation for stale file handle error.
# Only libc.pot part is sent upstream, the only valid part
# since upstream translations are done by the TP.
Patch1047: glibc-rh981332.patch

# Upstream BZ 9954
Patch1048: glibc-rh739743.patch

# Upstream BZ 13028
Patch1049: glibc-rh841787.patch

# Systemtap malloc probes
Patch1050: glibc-rh742038.patch

# Upstream BZ 15006
Patch1051: glibc-rh905184.patch

# Upstream BZ 14256
Patch1052: glibc-rh966259.patch

# Upstream BZ 15362
Patch1053: glibc-rh979363.patch

# Upstream BZ 14547
Patch1054: glibc-rh989862.patch
Patch1055: glibc-rh989862-2.patch
Patch1056: glibc-rh989862-3.patch
Patch1057: glibc-rh989861.patch

# Upstream s390/s390x bug fixes
Patch1058: glibc-rh804768-bugfix.patch

# Upstream BZ 15754
Patch1059: glibc-rh990481-CVE-2013-4788.patch

# Upstream BZ 16366
Patch1060: glibc-rh1039970.patch

# Upstream BZ 16365
Patch1061: glibc-rh1046199.patch

# Upstream BZ 16532
Patch1062: glibc-rh1063681.patch

# Upstream BZ 16680
Patch1063: glibc-rh1074410.patch

# PPC64LE Patch Set:
# abilist-pattern configurability
Patch1064: glibc-ppc64le-01.patch

# PowerPC: powerpc64le abilist for 2.17
Patch1065: glibc-ppc64le-02.patch

# Update miscellaneous scripts from upstream.
Patch1066: glibc-ppc64le-03.patch

# IBM long double mechanical changes to support little-endian
Patch1067: glibc-ppc64le-04.patch

# Fix for [BZ #15680] IBM long double inaccuracy
Patch1068: glibc-ppc64le-05.patch

# PowerPC floating point little-endian [1-15 of 15]
Patch1069: glibc-ppc64le-06.patch
Patch1070: glibc-ppc64le-07.patch
Patch1071: glibc-ppc64le-08.patch
Patch1072: glibc-ppc64le-09.patch
Patch1073: glibc-ppc64le-10.patch
Patch1074: glibc-ppc64le-11.patch
Patch1075: glibc-ppc64le-12.patch
Patch1076: glibc-ppc64le-13.patch
Patch1077: glibc-ppc64le-14.patch
Patch1078: glibc-ppc64le-15.patch
Patch1079: glibc-ppc64le-16.patch
Patch1080: glibc-ppc64le-17.patch
Patch1081: glibc-ppc64le-18.patch
Patch1082: glibc-ppc64le-19.patch
Patch1083: glibc-ppc64le-20.patch

# PowerPC LE setjmp/longjmp
Patch1084: glibc-ppc64le-21.patch

# PowerPC ugly symbol versioning
Patch1085: glibc-ppc64le-22.patch

# PowerPC LE _dl_hwcap access
Patch1086: glibc-ppc64le-23.patch

# PowerPC makecontext
Patch1087: glibc-ppc64le-24.patch

# PowerPC LE strlen
Patch1088: glibc-ppc64le-25.patch

# PowerPC LE strnlen
Patch1089: glibc-ppc64le-26.patch

# PowerPC LE strcmp and strncmp
Patch1090: glibc-ppc64le-27.patch

# PowerPC LE strcpy
Patch1091: glibc-ppc64le-28.patch

# PowerPC LE strchr
Patch1092: glibc-ppc64le-29.patch

# PowerPC LE memcmp
Patch1093: glibc-ppc64le-30.patch

# PowerPC LE memcpy
Patch1094: glibc-ppc64le-31.patch

# PowerPC LE memset
Patch1095: glibc-ppc64le-32.patch

# PowerPC LE memchr and memrchr
Patch1096: glibc-ppc64le-33.patch

# PowerPC LE configury
Patch1097: glibc-ppc64le-34.patch

# PowerPC64: Fix incorrect CFI in *context routines
Patch1098: glibc-ppc64le-35.patch

# PowerPC64: Report overflow on @h and @ha relocations
Patch1099: glibc-ppc64le-36.patch

# PowerPC64: Add __private_ss field to TCB header
Patch1100: glibc-ppc64le-37.patch

# PowerPC64 ELFv2 ABI 1/6: Code refactoring
Patch1101: glibc-ppc64le-38.patch

# PowerPC64 ELFv2 ABI 2/6: Remove function descriptors
Patch1102: glibc-ppc64le-39.patch

# PowerPC64 ELFv2 ABI 3/6: PLT local entry point optimization
Patch1103: glibc-ppc64le-40.patch

# PowerPC64 ELFv2 ABI 4/6: Stack frame layout changes
Patch1104: glibc-ppc64le-41.patch

# PowerPC64 ELFv2 ABI 5/6: LD_AUDIT interface changes
Patch1105: glibc-ppc64le-42.patch

# PowerPC64 ELFv2 ABI 6/6: Bump ld.so soname version number
Patch1106: glibc-ppc64le-43.patch

# Fix s_copysign stack temp for PowerPC64 ELFv2 [BZ #16786]
Patch1107: glibc-ppc64le-44.patch

# PPC64LE only Versions.def change.
Patch1108: glibc-ppc64le-45.patch

# powerpc/fpu/libm-test-ulps tan() round-toward-zero fixup.
Patch1109: glibc-ppc64le-46.patch
# End of PPC64LE patch set.
# Leave room up to 1120 for ppc64le patches.

# Split out ldbl_high into a distinct patch that is applied *before*
# the ppc64le patches and the ppc64 IFUNC patches. We do this because
# we want the IFUNC patches to be LE-safe and some code has to be
# factored out and applied for both to use.
Patch1110: glibc-powerpc-ldbl_high.patch

# RH BZ #1186491: disable inlining in math tests to work around
# a GCC bug.
Patch1112: glibc-rh1186491.patch

# Backport the write buffer size adjustment to match newer kernels.
Patch1122: glibc-fix-test-write-buf-size.patch

Patch1123: glibc-rh1248208.patch
Patch1124: glibc-rh1248208-2.patch

# Upstream BZ #14142
# Required RHEL 7.1 BZ #1067754
Patch1499: glibc-rh1067755.patch

# Acadia BZ #1070458
Patch1500: glibc-rh1070458.patch

# Upstream BZ #15036
# Acadia BZ #1070471
Patch1501: glibc-rh1070471.patch

# Upstream BZ #16169
# Acadia BZ #1078225
Patch1502: glibc-rh1078225.patch

# Upstream BZ #16629 -- plus a bit extra
Patch1503: glibc-aa64-setcontext.patch

Patch1504: glibc-aarch64-add-ptr_mangle-support.patch
Patch1505: glibc-aarch64-fpu-optional-trapping-exceptions.patch
Patch1506: glibc-aarch64-syscall-rewrite.patch
Patch1508: glibc-aarch64-ifunc.patch

Patch1509: glibc-rh1133812-2.patch
Patch1510: glibc-rh1133812-3.patch

Patch1511: glibc-rh1083647.patch
Patch1512: glibc-rh1085290.patch
Patch1513: glibc-rh1083644.patch
Patch1514: glibc-rh1083646.patch

Patch1515: glibc-rh1103856.patch
Patch1516: glibc-rh1080766.patch
Patch1517: glibc-rh1103874.patch
Patch1518: glibc-rh1125306.patch
Patch1519: glibc-rh1098047.patch
Patch1520: glibc-rh1138520.patch
Patch1521: glibc-rh1085313.patch
Patch1522: glibc-rh1140474.patch

# Backport multi-lib support in GLIBC for PowerPC using IFUNC
Patch1530: glibc-rh731837-00.patch
Patch1531: glibc-rh731837-01.patch
Patch1532: glibc-rh731837-02.patch
Patch1533: glibc-rh731837-03.patch
Patch1534: glibc-rh731837-04.patch
Patch1535: glibc-rh731837-05.patch
Patch1536: glibc-rh731837-06.patch
Patch1537: glibc-rh731837-07.patch
Patch1538: glibc-rh731837-08.patch
Patch1539: glibc-rh731837-09.patch
Patch1540: glibc-rh731837-10.patch
Patch1541: glibc-rh731837-11.patch
Patch1542: glibc-rh731837-12.patch
Patch1543: glibc-rh731837-13.patch
Patch1544: glibc-rh731837-14.patch
Patch1545: glibc-rh731837-15.patch
Patch1546: glibc-rh731837-16.patch
Patch1547: glibc-rh731837-17.patch
Patch1548: glibc-rh731837-18.patch
Patch1549: glibc-rh731837-19.patch
Patch1550: glibc-rh731837-20.patch
Patch1551: glibc-rh731837-21.patch
Patch1552: glibc-rh731837-22.patch
Patch1553: glibc-rh731837-23.patch
Patch1554: glibc-rh731837-24.patch
Patch1555: glibc-rh731837-25.patch
Patch1556: glibc-rh731837-26.patch
Patch1557: glibc-rh731837-27.patch
Patch1558: glibc-rh731837-28.patch
Patch1559: glibc-rh731837-29.patch
Patch1560: glibc-rh731837-30.patch
Patch1561: glibc-rh731837-31.patch
Patch1562: glibc-rh731837-32.patch
Patch1563: glibc-rh731837-33.patch
Patch1564: glibc-rh731837-34.patch
Patch1565: glibc-rh731837-35.patch
Patch1566: glibc-rh731837-33A.patch
Patch1567: glibc-rh731837-36.patch

# Intel AVX-512 support.
Patch1570: glibc-rh1140272-avx512.patch
# Intel MPX support.
Patch1571: glibc-rh1132518-mpx.patch
# glibc manual update.
Patch1572: glibc-manual-update.patch

Patch1573: glibc-rh1120490.patch

# Fix ppc64le relocation handling:
Patch1574: glibc-rh1162847-p1.patch
Patch1575: glibc-rh1162847-p2.patch

# CVE-2014-7817
Patch1576: glibc-rh1170118-CVE-2014-7817.patch

# Provide artificial OPDs for ppc64 VDSO functions.
Patch1577: glibc-rh1077389-p1.patch
# BZ#16431
Patch1578: glibc-rh1077389-p2.patch
# BZ#16037 - Allow GNU Make version 4.0 and up to be used.
Patch1579: glibc-gmake.patch

Patch1580: glibc-rh1183545.patch

Patch1581: glibc-rh1064066.patch

# RHBZ #1165212 - [SAP] Recursive dlopen causes SAP HANA installer
#                 to crash
#   including RHBZ #1225959 - Test suite failure: tst-rec-dlopen fails
Patch1582: glibc-rh1165212.patch
# BZ#17411
Patch1583: glibc-rh1144133.patch

# BZ#12100 - QoI regression: strstr() slowed from O(n) to O(n^2)
#            on SSE4 machines
# but mainly RHBZ #1150282 - glibc-2.17-55 crashes sqlplus
Patch1584: glibc-rh1150282.patch
# BZ#13862
Patch1585: glibc-rh1189278.patch
Patch1586: glibc-rh1189278-1.patch
# BZ#17892
Patch1587: glibc-rh1183456.patch
# BZ#14841
Patch1588: glibc-rh1186620.patch

# BZ#16878 - nscd enters busy loop on long netgroup entry via nss_ldap
#            of nslcd
Patch1589: glibc-rh1173537.patch

# BZ#14906: inotify failed when /etc/hosts file change
Patch1590: glibc-rh1193797.patch
# RHBZ #1173238 - `check-abi-librtkaio' not performed during
# make check after building glibc
Patch1591: glibc-rh1173238.patch

Patch1592: glibc-rh1207032.patch

Patch1593: glibc-rh1176906.patch
Patch1594: glibc-rh1159169.patch
Patch1595: glibc-rh1098042.patch
Patch1596: glibc-rh1194143.patch
Patch1597: glibc-rh1219891.patch

# RHBZ #1209107 - CVE-2015-1781 CVE-2015-1473 CVE-2015-1472 glibc:
#                 various flaws [rhel-7.2]
# * RHBZ #1188235 - (CVE-2015-1472) CVE-2015-1472 glibc: heap buffer
#                   overflow in glibc swscanf
#   upstream #16618 - wscanf allocates too little memory
#                     (CVE-2015-1472, CVE-2015-1473)
Patch1598: glibc-rh1188235.patch
#
# * RHBZ #1195762 - glibc: _IO_wstr_overflow integer overflow
#   upstream #16009 - Possible buffer overflow in strxfrm
Patch1599: glibc-rh1195762.patch
#
# * RHBZ #1197730 - glibc: potential denial of service in internal_fnmatch()
#   upstream #17062 - fnmatch: buffer overflow read from pattern
#                     "[[:alpha:]'[:alpha:]"
Patch1600: glibc-rh1197730-1.patch
#   upstream #18032 - buffer overflow (read past end of buffer) in
#                     internal_fnmatch
Patch1601: glibc-rh1197730-2.patch
#   upstream #18036 - buffer overflow (read past end of buffer)
#                     in internal_fnmatch=>end_pattern with "**(!()" pattern
Patch1602: glibc-rh1197730-3.patch
#
# * RHBZ #1199525 - (CVE-2015-1781) CVE-2015-1781 glibc: buffer overflow
#                   in gethostbyname_r() and related functions with
#                   misaligned buffer
#  upstream #18287 - (CVE-2015-1781) - Buffer overflow in getanswer_r,
#                    resolv/nss_dns/dns-host.c (CVE-2015-1781)
Patch1603: glibc-rh1199525.patch
#
# RHBZ #1162895 - Backport upstream ppc64 and ppc64le enhancements
# * upstream #17153 - Shared libraries built with multiple tocs resolve
#                     plt to local function entry
Patch1604: glibc-rh1162895-1.patch
#
# * upstream #16740 - IBM long double frexpl wrong when value slightly
#                     smaller than a power of two
# * upstream #16619 - [ldbl-128ibm] frexpl bad results on some denormal
#                     arguments
Patch1605: glibc-rh1162895-2.patch
#
# * upstream #16739 - IBM long double nextafterl wrong on power of two value
Patch1606: glibc-rh1162895-3.patch
# RHBZ #1214326 - Upstream benchtests/ rebase
# RHBZ #1084395 - Run pythong scripts with $(PYTHON).
Patch1607: glibc-rh1084395.patch

# CVE-2014-8121:
Patch1608: glibc-rh1165192.patch

# BZ #17090, BZ #17620, BZ #17621, BZ #17628
Patch1609: glibc-rh1202952.patch

# BZ #15234:
Patch1610: glibc-rh1234622.patch

# Fix 32-bit POWER assembly to use only 32-bit instructions.
Patch1611: glibc-rh1240796.patch

# CVE-2015-5229 and regression test.
Patch1612: glibc-rh1293976.patch
Patch1613: glibc-rh1293976-2.patch

# BZ #16574
Patch1614: glibc-rh1296031-0.patch
# BZ #13928
Patch1616: glibc-rh1296031-2.patch

# Malloc trim fixes: #17195, #18502.
Patch1617: glibc-rh1284959-1.patch
Patch1618: glibc-rh1284959-2.patch
Patch1619: glibc-rh1284959-3.patch

# ppc64le monstartup fix:
Patch1620: glibc-rh1249102.patch

# Fix race in free() of fastbin chunk: BZ #15073.
Patch1621: glibc-rh1027101.patch

# BZ #17370: Memory leak in wide-oriented ftell.
Patch1622: glibc-rh1310530.patch 

# BZ #19791: NULL pointer dereference in stub resolver with unconnectable
# name server addresses
Patch1623: glibc-rh1320596.patch

# RHBZ #1331283 - Backport "Coordinate IPv6 definitions for Linux and glibc"
Patch1624: glibc-rh1331283.patch
Patch1625: glibc-rh1331283-1.patch
Patch1626: glibc-rh1331283-2.patch
Patch1627: glibc-rh1331283-3.patch
Patch1628: glibc-rh1331283-4.patch

##############################################################################
#
# Patches submitted, but not yet approved upstream.
#
##############################################################################
#
# Each should be associated with a BZ.
# Obviously we're not there right now, but that's the goal
#

# http://sourceware.org/ml/libc-alpha/2012-12/msg00103.html
# Not upstream as of 2014-02-27
Patch2007: glibc-rh697421.patch

# Not upstream as of 2014-02-27
Patch2011: glibc-rh757881.patch

# Not upstream as of 2014-02-27
Patch2013: glibc-rh741105.patch

# Upstream BZ 14247
# Not upstream as of 2014-02-27.
Patch2023: glibc-rh827510.patch

# Upstream BZ 14185
# Not upstream as of 2014-02-27.
Patch2027: glibc-rh819430.patch

# Fix nscd to use permission names not constants.
# Not upstream as of 2014-02-27.
Patch2048: glibc-rh1025934.patch

# Upstream BZ 16398.
Patch2051: glibc-rh1048036.patch
Patch2052: glibc-rh1048123.patch

# Upstream BZ 16680
Patch2053: glibc-rh1074410-2.patch

# Upstream BZ 15493.
# Upstream as of 2013-03-20
Patch2055: glibc-rh1073667.patch

Patch2060: glibc-aarch64-rh1076760.patch

# Include pthread.h in rtkaio/tst-aiod2.c and rtkaio/tst-aiod3.c.
Patch2062: glibc-rtkaio-inc-pthread.patch

Patch2063: glibc-rh1084089.patch

Patch2064: glibc-rh1161666.patch

Patch2065: glibc-rh1156331.patch

# Upstream BZ 18557: Fix ruserok scalability issues.
Patch2066: glibc-rh1216246.patch

# PiP
Patch9999: glibc-pip.patch

##############################################################################
# End of glibc patches.
##############################################################################

##############################################################################
# Continued list of core "glibc" package information:
##############################################################################

Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

##############################################################################
# Prepare for the build.
##############################################################################
%prep
%setup -q -n %{glibcsrcdir} -b1

# Patch order is important as some patches depend on other patches and
# therefore the order must not be changed.
%patch0001 -p1
%patch0002 -p1
%patch0003 -p1
%patch0004 -p1
%patch0005 -p1
%patch0006 -p1
%patch2007 -p1
%patch0008 -p1
%patch0009 -p1
%patch0010 -p1
%patch2011 -p1
%patch0012 -p1
%patch2013 -p1
%patch0014 -p1
%patch0015 -p1
%patch0016 -p1
%patch0017 -p1
%patch0019 -p1
%patch0020 -p1
%patch1048 -p1
%patch2023 -p1
%patch0024 -p1
%patch0025 -p1
%patch1049 -p1
%patch2027 -p1
%patch0028 -p1
%patch0029 -p1
%patch0030 -p1
%patch0031 -p1
%patch0032 -p1
%patch0033 -p1
%patch0034 -p1
%patch1051 -p1
%patch0035 -p1
%patch0036 -p1
%patch0037 -p1
%patch1000 -p1
%patch0038 -p1
%patch1052 -p1
%patch1001 -p1
%patch1002 -p1
%patch1003 -p1
%patch1004 -p1
%patch1005 -p1
%patch1006 -p1
%patch1007 -p1
%patch1008 -p1
%patch1009 -p1
%patch1053 -p1
%patch1010 -p1
%patch0039 -p1
%patch1054 -p1
%patch1055 -p1
%patch1056 -p1
%patch1057 -p1
%patch0040 -p1
%patch0041 -p1
%patch0042 -p1
%patch0043 -p1
%patch1050 -p1
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
%patch1029 -p1
%patch1030 -p1
%patch1031 -p1
%patch1032 -p1
%patch0044 -p1
%patch0045 -p1
%patch1047 -p1
%patch1034 -p1
%patch1058 -p1
%patch1035 -p1
%patch1059 -p1
%patch1036 -p1
%patch1046 -p1
%patch1037 -p1
%patch1038 -p1
%patch2048 -p1
%patch1039 -p1
%patch1040 -p1
%patch1041 -p1
%patch1042 -p1
%patch1043 -p1
%patch1060 -p1
%patch1061 -p1
%patch2051 -p1
%patch1044 -p1
%patch1045 -p1
%patch2052 -p1
%patch0048 -p1
%patch0060 -p1
%patch1062 -p1
%patch1063 -p1
%patch2053 -p1
# Apply ldbl_high() patch for both ppc64le and ppc64.
%patch1110 -p1

# PPC64LE Patch set:
# 1064 to 1109.

%patch1064 -p1
%patch1065 -p1
%patch1066 -p1
%patch1067 -p1
%patch1068 -p1
%patch1069 -p1
%patch1070 -p1
%patch1071 -p1
%patch1072 -p1
%patch1073 -p1
%patch1074 -p1
%patch1075 -p1
%patch1076 -p1
%patch1077 -p1
%patch1078 -p1
%patch1079 -p1
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
%patch1102 -p1
%patch1103 -p1
%patch1104 -p1
%patch1105 -p1
%patch1106 -p1
%patch1107 -p1
%patch1108 -p1
%patch1109 -p1

%patch1112 -p1
# End of PPC64LE Patch Set.

%patch1122 -p1
%patch2055 -p1
%patch1499 -p1
%patch1500 -p1
%patch1501 -p1
%patch1502 -p1
%patch1503 -p1
%patch1504 -p1
%patch1505 -p1
%patch2060 -p1
%patch1506 -p1
%patch1508 -p1
%patch2062 -p1
%patch0061 -p1
%patch1509 -p1
%patch1510 -p1
%patch1511 -p1
%patch1512 -p1
%patch1513 -p1
%patch1514 -p1
%patch1515 -p1
%patch1516 -p1
%patch1517 -p1
%patch1518 -p1
%patch1519 -p1
%patch0062 -p1
%patch2063 -p1
%patch1520 -p1
%patch1521 -p1
%patch1522 -p1
# Start of IBM IFUNC patch set.
%patch1530 -p1
%patch1531 -p1
%patch1532 -p1
%patch1533 -p1
%patch1534 -p1
%patch1535 -p1
%patch1536 -p1
%patch1537 -p1
%patch1538 -p1
%patch1539 -p1
%patch1540 -p1
%patch1541 -p1
%patch1542 -p1
%patch1543 -p1
%patch1544 -p1
%patch1545 -p1
%patch1546 -p1
%patch1547 -p1
%patch1548 -p1
%patch1549 -p1
%patch1550 -p1
%patch1551 -p1
%patch1552 -p1
%patch1553 -p1
%patch1554 -p1
%patch1555 -p1
%patch1556 -p1
%patch1557 -p1
%patch1558 -p1
%patch1559 -p1
%patch1560 -p1
%patch1561 -p1
%patch1562 -p1
%patch1563 -p1
%patch1564 -p1
%patch1565 -p1
# Apply fixup patch for -33 in the series to make it LE safe.
%patch1566 -p1
%patch1567 -p1
# End of IBM IFUNC patch set.
%patch1570 -p1
%patch1571 -p1
%patch1572 -p1
%patch1573 -p1
%patch0063 -p1
%patch2064 -p1
%patch1574 -p1
%patch1575 -p1
%patch2065 -p1
%patch1576 -p1
%patch1577 -p1
%patch1578 -p1
%patch1608 -p1
%patch1579 -p1
%patch1580 -p1
%patch1581 -p1
%patch1582 -p1
%patch1583 -p1
%patch1584 -p1
%patch1585 -p1
%patch1586 -p1
%patch1587 -p1
%patch1588 -p1
%patch1589 -p1
%patch1590 -p1
%patch1591 -p1
%patch1592 -p1
%patch1593 -p1
%patch1594 -p1
%patch1595 -p1
%patch1596 -p1
%patch1597 -p1
%patch0066 -p1
%patch1598 -p1
%patch1599 -p1
%patch1600 -p1
%patch1601 -p1
%patch1602 -p1
%patch1603 -p1
%patch1604 -p1
%patch1605 -p1
%patch1606 -p1
%patch2066 -p1
# Rebase of microbenchmarks.
%patch1607 -p1
%patch1609 -p1
%patch1610 -p1
%patch1611 -p1
%patch1123 -p1
%patch1124 -p1
%patch1612 -p1
%patch1613 -p1
%patch1614 -p1
%patch1616 -p1
%patch0067 -p1
%patch1617 -p1
%patch1618 -p1
%patch1619 -p1
%patch1620 -p1
%patch1621 -p1
%patch1622 -p1
%patch1623 -p1

# RHBZ #1331283 - Backport "Coordinate IPv6 definitions for Linux and glibc"
%patch1624 -p1
%patch1625 -p1
%patch1626 -p1
%patch1627 -p1
%patch1628 -p1

# PiP
%patch9999 -p1

%description
glibc with patches for PiP

##############################################################################
# Build glibc...
##############################################################################
%build
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

d=`pwd`
mkdir build &&
(
	cd build &&
	DESTDIR="$RPM_BUILD_ROOT" sh ../build.sh -b $d %{pip_glibc_dir}
)

%install

d=`pwd`
(
	cd build &&
	DESTDIR="$RPM_BUILD_ROOT" sh ../build.sh -i $d %{pip_glibc_dir}
)

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root)
%{pip_glibc_dir}
