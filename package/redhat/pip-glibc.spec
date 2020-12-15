# How to build this RPM:
#
#	$ rpm -Uvh glibc-2.17-260.el7.src.rpm
#	$ cd .../$SOMEWHERE/.../PiP-glibc
#	$ git diff origin/centos/glibc-2.17-260.el7.branch centos/glibc-2.17-260.el7.pip.branch >~/rpmbuild/SOURCES/glibc-pip.patch
#	$ rpmbuild -bb ~/rpmbuild/SPECS/pip-glibc.spec
#

%define	pip_glibc_dir		/opt/pip
%define	pip_glibc_release	pip2

# disable strip, otherwise the pip-gdb shows the following error:
#	warning: Unable to find libthread_db matching inferior's thread library, thread debugging will not be available.
%define __spec_install_post /usr/lib/rpm/brp-compress
%define __os_install_post /usr/lib/rpm/brp-compress
%define debug_package %{nil}

%define glibcsrcdir glibc-2.17-c758a686
%define glibcversion 2.17
%define glibcrelease 260%{?dist}.%{pip_glibc_release}
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
Source2: verify.md5

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

# releng patch from Fedora
Patch0068: glibc-rh1349982.patch

# These changes were brought forward from RHEL 6 for compatibility
Patch0069: glibc-rh1448107.patch
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

# Fix for RHBZ #1213267 as a prerequisite for the patches below.
Patch1612: glibc-rh1240351-1.patch

# Backport of POWER8 glibc optimizations for RHEL7.3: math functions
Patch1613: glibc-rh1240351-2.patch
Patch1614: glibc-rh1240351-3.patch

# Backport of POWER8 glibc optimizations for RHEL7.3: string functions
Patch1615: glibc-rh1240351-4.patch
Patch1616: glibc-rh1240351-5.patch
Patch1617: glibc-rh1240351-6.patch
Patch1618: glibc-rh1240351-7.patch
Patch1619: glibc-rh1240351-8.patch
Patch1620: glibc-rh1240351-9.patch
Patch1621: glibc-rh1240351-10.patch
Patch1622: glibc-rh1240351-11.patch
Patch1623: glibc-rh1240351-12.patch

# Backport of upstream IBM z13 patches for RHEL 7.3
Patch1624: glibc-rh1268008-1.patch
Patch1625: glibc-rh1268008-2.patch
Patch1626: glibc-rh1268008-3.patch
Patch1627: glibc-rh1268008-4.patch
Patch1628: glibc-rh1268008-5.patch
Patch1629: glibc-rh1268008-6.patch
Patch1630: glibc-rh1268008-7.patch
Patch1631: glibc-rh1268008-8.patch
Patch1632: glibc-rh1268008-9.patch
Patch1633: glibc-rh1268008-10.patch
Patch1634: glibc-rh1268008-11.patch
Patch1635: glibc-rh1268008-12.patch
Patch1636: glibc-rh1268008-13.patch
Patch1637: glibc-rh1268008-14.patch
Patch1638: glibc-rh1268008-15.patch
Patch1639: glibc-rh1268008-16.patch
Patch1640: glibc-rh1268008-17.patch
Patch1641: glibc-rh1268008-18.patch
Patch1642: glibc-rh1268008-19.patch
Patch1643: glibc-rh1268008-20.patch
Patch1644: glibc-rh1268008-21.patch
Patch1645: glibc-rh1268008-22.patch
Patch1646: glibc-rh1268008-23.patch
Patch1647: glibc-rh1268008-24.patch
Patch1648: glibc-rh1268008-25.patch
Patch1649: glibc-rh1268008-26.patch
Patch1650: glibc-rh1268008-27.patch
Patch1651: glibc-rh1268008-28.patch
Patch1652: glibc-rh1268008-29.patch
Patch1653: glibc-rh1268008-30.patch

Patch1654: glibc-rh1249102.patch

# CVE-2015-5229 and regression test.
Patch1656: glibc-rh1293976.patch
Patch1657: glibc-rh1293976-2.patch

# BZ #16574
Patch1658: glibc-rh1296031-0.patch
# BZ #13928
Patch1660: glibc-rh1296031-2.patch

# Malloc trim fixes: #17195, #18502.
Patch1661: glibc-rh1284959-1.patch
Patch1662: glibc-rh1284959-2.patch
Patch1663: glibc-rh1284959-3.patch

# RHBZ #1293916 - iconv appears to be adding a duplicate "SI"
#                 to the output for certain inputs 
Patch1664: glibc-rh1293916.patch

# Race condition in _int_free involving fastbins: #15073
Patch1665: glibc-rh1027101.patch

# BZ #17370: Memory leak in wide-oriented ftell.
Patch1666: glibc-rh1310530.patch

# BZ #19791: NULL pointer dereference in stub resolver with unconnectable
# name server addresses
Patch1667: glibc-rh1320596.patch

# RHBZ #1298349 - Backport tst-getpw enhancements
Patch1668: glibc-rh1298349.patch

# RHBZ #1293433 - Test suite failure: Fix bug17079
Patch1669: glibc-rh1293433.patch

# RHBZ #1298354 - Backport test-skeleton.c conversions
Patch1670: glibc-rh1298354.patch

# RHBZ #1288613 - gethostbyname_r hangs forever
Patch1671: glibc-rh1288613.patch

# RHBZ #1064063 - Test suite failure: tst-mqueue5
Patch1672: glibc-rh1064063.patch

# RHBZ #140250 - Unexpected results from using posix_fallocate
#                with nfs target 
Patch1675: glibc-rh1140250.patch

# RHBZ #1324427 - RHEL7.3 - S390: fprs/vrs are not saved/restored while
#                 resolving symbols
Patch1676: glibc-rh1324427-1.patch
Patch1677: glibc-rh1324427-2.patch
Patch1678: glibc-rh1324427-3.patch

# RHBZ #1234449 - glibc: backport upstream hardening patches
Patch1679: glibc-rh1234449-1.patch
Patch1680: glibc-rh1234449-2.patch
Patch1681: glibc-rh1234449-3.patch
Patch1682: glibc-rh1234449-4.patch

# RHBZ #1221046 - make bits/stat.h FTM guards consistent on all arches
Patch1683: glibc-rh1221046.patch

# RHBZ #971416 - Locale alias no_NO.ISO-8859-1 not working
Patch1684: glibc-rh971416-1.patch
Patch1685: glibc-rh971416-2.patch
Patch1686: glibc-rh971416-3.patch

# RHBZ 1302086 -  Improve libm performance AArch64
Patch1687: glibc-rh1302086-1.patch
Patch1688: glibc-rh1302086-2.patch
Patch1689: glibc-rh1302086-3.patch
Patch1690: glibc-rh1302086-4.patch
Patch1691: glibc-rh1302086-5.patch
Patch1692: glibc-rh1302086-6.patch
Patch1693: glibc-rh1302086-7.patch
Patch1694: glibc-rh1302086-8.patch
Patch1695: glibc-rh1302086-9.patch
Patch1696: glibc-rh1302086-10.patch
Patch1697: glibc-rh1302086-11.patch

# RHBZ 1346397 debug/tst-longjump_chk2 calls printf from a signal handler
Patch1698: glibc-rh1346397.patch

# RHBZ #1211823 Update BIG5-HKSCS charmap to HKSCS-2008
Patch1699: glibc-rh1211823.patch

# RHBZ #1268050 Backport "Coordinate IPv6 definitions for Linux and glibc"
Patch1700: glibc-rh1331283.patch
Patch1701: glibc-rh1331283-1.patch
Patch1702: glibc-rh1331283-2.patch
Patch1703: glibc-rh1331283-3.patch
Patch1704: glibc-rh1331283-4.patch

# RHBZ #1296297 enable (backport) instLangs in RHEL glibc
Patch1705: glibc-rh1296297.patch
Patch1706: glibc-rh1296297-1.patch

# RHBZ #1027348 sem_post/sem_wait race causing sem_post to return EINVAL
Patch1707: glibc-rh1027348.patch
Patch1708: glibc-rh1027348-1.patch
Patch1709: glibc-rh1027348-2.patch
Patch1710: glibc-rh1027348-3.patch
Patch1711: glibc-rh1027348-4.patch

# RHBZ #1308728 Fix __times() handling of EFAULT when buf is NULL
Patch1712: glibc-rh1308728.patch

# RHBZ #1249114 [s390] setcontext/swapcontext does not restore signal mask
Patch1713: glibc-rh1249114.patch
# RHBZ #1249115 S390: backtrace() returns infinitely deep stack ...
Patch1714: glibc-rh1249115.patch

# RHBZ #1321993: CVE-2016-3075: Stack overflow in nss_dns_getnetbyname_r
Patch1715: glibc-rh1321993.patch

# RHBZ #1256317 - IS_IN backports.
Patch1716: glibc-rh1256317-21.patch
Patch1717: glibc-rh1256317-20.patch
Patch1718: glibc-rh1256317-19.patch
Patch1719: glibc-rh1256317-18.patch
Patch1720: glibc-rh1256317-17.patch
Patch1721: glibc-rh1256317-16.patch
Patch1722: glibc-rh1256317-15.patch
Patch1723: glibc-rh1256317-14.patch
Patch1724: glibc-rh1256317-13.patch
Patch1725: glibc-rh1256317-12.patch
Patch1726: glibc-rh1256317-11.patch
Patch1727: glibc-rh1256317-10.patch
Patch1728: glibc-rh1256317-9.patch
Patch1729: glibc-rh1256317-8.patch
Patch1730: glibc-rh1256317-7.patch
Patch1731: glibc-rh1256317-6.patch
Patch1732: glibc-rh1256317-5.patch
Patch1733: glibc-rh1256317-4.patch
Patch1734: glibc-rh1256317-3.patch
Patch1735: glibc-rh1256317-2.patch
Patch1736: glibc-rh1256317-1.patch
Patch1737: glibc-rh1256317-0.patch

# RHBZ #1335286 [Intel 7.3 Bug] (Purley) Backport 64-bit memset from glibc 2.18
Patch1738: glibc-rh1335286-0.patch
Patch1739: glibc-rh1335286.patch

# RHBZ #1292018 [Intel 7.3 Bug] Improve branch prediction on Knights Landing/Silvermont
Patch1740: glibc-rh1292018-0.patch
Patch1741: glibc-rh1292018-0a.patch
Patch1742: glibc-rh1292018-0b.patch
Patch1743: glibc-rh1292018-1.patch
Patch1744: glibc-rh1292018-2.patch
Patch1745: glibc-rh1292018-3.patch
Patch1746: glibc-rh1292018-4.patch
Patch1747: glibc-rh1292018-5.patch
Patch1748: glibc-rh1292018-6.patch
Patch1749: glibc-rh1292018-7.patch

# RHBZ #1255822 glibc: malloc may fall back to calling mmap prematurely if arenas are contended
Patch1750: glibc-rh1255822.patch

# RHBZ #1298526 [Intel 7.3 FEAT] glibc: AVX-512 optimized memcpy
Patch1751: glibc-rh1298526-0.patch
Patch1752: glibc-rh1298526-1.patch
Patch1753: glibc-rh1298526-2.patch
Patch1754: glibc-rh1298526-3.patch
Patch1755: glibc-rh1298526-4.patch

# RHBZ #1350733 locale-archive.tmpl cannot be processed by build-locale-archive
Patch1756: glibc-rh1350733-1.patch

# Fix tst-cancel17/tst-cancelx17, which sometimes segfaults while exiting.
Patch1757: glibc-rh1337242.patch

# RHBZ #1418978: backport upstream support/ directory
Patch17580: glibc-rh1418978-max_align_t.patch
Patch1758: glibc-rh1418978-0.patch
Patch1759: glibc-rh1418978-1.patch
Patch1760: glibc-rh1418978-2-1.patch
Patch1761: glibc-rh1418978-2-2.patch
Patch1762: glibc-rh1418978-2-3.patch
Patch1763: glibc-rh1418978-2-4.patch
Patch1764: glibc-rh1418978-2-5.patch
Patch1765: glibc-rh1418978-2-6.patch
Patch1766: glibc-rh1418978-3-1.patch
Patch1767: glibc-rh1418978-3-2.patch

# RHBZ #906468: Fix deadlock between fork, malloc, flush (NULL)
Patch1768: glibc-rh906468-1.patch
Patch1769: glibc-rh906468-2.patch

# RHBZ #988869: stdio buffer auto-tuning should reject large buffer sizes
Patch1770: glibc-rh988869.patch

# RHBZ #1398244 - RHEL7.3 - glibc: Fix TOC stub on powerpc64 clone()
Patch1771: glibc-rh1398244.patch

# RHBZ #1228114: Fix sunrpc UDP client timeout handling
Patch1772: glibc-rh1228114-1.patch
Patch1773: glibc-rh1228114-2.patch

# RHBZ #1298975 - [RFE] Backport the groups merging feature
Patch1774: glibc-rh1298975.patch

# RHBZ #1318877 - Per C11 and C++11, <stdint.h> should not look at
# __STDC_LIMIT_MACROS or __STDC_CONSTANT_MACROS
Patch1775: glibc-rh1318877.patch

# RHBZ #1417205: Add AF_VSOCK/PF_VSOCK, TCP_TIMESTAMP
Patch1776: glibc-rh1417205.patch

# RHBZ #1338672: GCC 6 enablement for struct sockaddr_storage
Patch1777: glibc-rh1338672.patch

# RHBZ #1325138 - glibc: Corrupted aux-cache causes ldconfig to segfault
Patch1778: glibc-rh1325138.patch

# RHBZ #1374652: Unbounded stack allocation in nan* functions
Patch1779: glibc-rh1374652.patch

# RHBZ #1374654: Unbounded stack allocation in nan* functions
Patch1780: glibc-rh1374654.patch

# RHBZ #1322544: Segmentation violation can occur within glibc if fork()
# is used in a multi-threaded application
Patch1781: glibc-rh1322544.patch

# RHBZ #1418997: does not build with binutils 2.27 due to misuse of the cmpli instruction on ppc64
Patch1782: glibc-rh1418997.patch

# RHBZ #1383951: LD_POINTER_GUARD in the environment is not sanitized
Patch1783: glibc-rh1383951.patch

# RHBZ #1385004: [7.4 FEAT] POWER8 IFUNC update from upstream
Patch1784: glibc-rh1385004-1.patch
Patch1785: glibc-rh1385004-2.patch
Patch1786: glibc-rh1385004-3.patch
Patch1787: glibc-rh1385004-4.patch
Patch1788: glibc-rh1385004-5.patch
Patch1789: glibc-rh1385004-6.patch
Patch1790: glibc-rh1385004-7.patch
Patch1791: glibc-rh1385004-8.patch
Patch1792: glibc-rh1385004-9.patch
Patch1793: glibc-rh1385004-10.patch
Patch1794: glibc-rh1385004-11.patch
Patch1795: glibc-rh1385004-12.patch
Patch1796: glibc-rh1385004-13.patch
Patch1797: glibc-rh1385004-14.patch
Patch1798: glibc-rh1385004-15.patch
Patch1799: glibc-rh1385004-16.patch
Patch1800: glibc-rh1385004-17.patch
Patch1801: glibc-rh1385004-18.patch
Patch1802: glibc-rh1385004-19.patch
Patch1803: glibc-rh1385004-20.patch
Patch1804: glibc-rh1385004-21.patch
Patch1805: glibc-rh1385004-22.patch
Patch1806: glibc-rh1385004-23.patch
Patch1807: glibc-rh1385004-24.patch

# RHBZ 1380680 - [7.4 FEAT] z13 exploitation in glibc - stage 2
Patch1808: glibc-rh1380680-1.patch
Patch1809: glibc-rh1380680-2.patch
Patch1810: glibc-rh1380680-3.patch
Patch1811: glibc-rh1380680-4.patch
Patch1812: glibc-rh1380680-5.patch
Patch1813: glibc-rh1380680-6.patch
Patch1814: glibc-rh1380680-7.patch
Patch1815: glibc-rh1380680-8.patch
Patch1816: glibc-rh1380680-9.patch
Patch1817: glibc-rh1380680-10.patch
Patch1818: glibc-rh1380680-11.patch
Patch1819: glibc-rh1380680-12.patch
Patch1820: glibc-rh1380680-13.patch
Patch1821: glibc-rh1380680-14.patch
Patch1822: glibc-rh1380680-15.patch
Patch1823: glibc-rh1380680-16.patch
Patch1824: glibc-rh1380680-17.patch

# RHBZ #1326739: malloc: additional unlink hardening for non-small bins
Patch1825: glibc-rh1326739.patch

# RHBZ #1374657: CVE-2015-8778: Integer overflow in hcreate and hcreate_r
Patch1826: glibc-rh1374657.patch

# RHBZ #1374658 - CVE-2015-8776: Segmentation fault caused by passing
# out-of-range data to strftime()
Patch1827: glibc-rh1374658.patch

# RHBZ #1385003 - SIZE_MAX evaluates to an expression of the wrong type
# on s390
Patch1828: glibc-rh1385003.patch

# RHBZ #1387874 - MSG_FASTOPEN definition missing
Patch1829: glibc-rh1387874.patch

# RHBZ #1409611 - poor performance with exp()
Patch1830: glibc-rh1409611.patch

# RHBZ #1421155 - Update dynamic loader trampoline for Intel SSE, AVX, and AVX512 usage.
Patch1831: glibc-rh1421155.patch

# RHBZ #841653 - [Intel 7.0 FEAT] [RFE] TSX-baed lock elision enabled in glibc.
Patch1832: glibc-rh841653-0.patch
Patch1833: glibc-rh841653-1.patch
Patch1834: glibc-rh841653-2.patch
Patch1835: glibc-rh841653-3.patch
Patch1836: glibc-rh841653-4.patch
Patch1837: glibc-rh841653-5.patch
Patch1838: glibc-rh841653-6.patch
Patch1839: glibc-rh841653-7.patch
Patch1840: glibc-rh841653-8.patch
Patch1841: glibc-rh841653-9.patch
Patch1842: glibc-rh841653-10.patch
Patch1843: glibc-rh841653-11.patch
Patch1844: glibc-rh841653-12.patch
Patch1845: glibc-rh841653-13.patch
Patch1846: glibc-rh841653-14.patch
Patch1847: glibc-rh841653-15.patch
Patch1848: glibc-rh841653-16.patch
Patch1849: glibc-rh841653-17.patch

# RHBZ #731835 - [RFE] [7.4 FEAT] Hardware Transactional Memory in GLIBC
Patch1850: glibc-rh731835-0.patch
Patch1851: glibc-rh731835-1.patch
Patch1852: glibc-rh731835-2.patch

# RHBZ #1413638: Inhibit FMA while compiling sqrt, pow
Patch1853: glibc-rh1413638-1.patch
Patch1854: glibc-rh1413638-2.patch

# RHBZ #1439165: Use a built-in list of known syscalls for <bits/syscall.h>
Patch1855: glibc-rh1439165.patch
Patch1856: glibc-rh1439165-syscall-names.patch

# RHBZ #1457177: Rounding issues on POWER
Patch1857: glibc-rh1457177-1.patch
Patch1858: glibc-rh1457177-2.patch
Patch1859: glibc-rh1457177-3.patch
Patch1860: glibc-rh1457177-4.patch

Patch1861: glibc-rh1348000.patch
Patch1862: glibc-rh1443236.patch
Patch1863: glibc-rh1447556.patch
Patch1864: glibc-rh1463692-1.patch
Patch1865: glibc-rh1463692-2.patch
Patch1866: glibc-rh1347277.patch

# RHBZ #1375235: Add new s390x instruction support
Patch1867: glibc-rh1375235-1.patch
Patch1868: glibc-rh1375235-2.patch
Patch1869: glibc-rh1375235-3.patch
Patch1870: glibc-rh1375235-4.patch
Patch1871: glibc-rh1375235-5.patch
Patch1872: glibc-rh1375235-6.patch
Patch1873: glibc-rh1375235-7.patch
Patch1874: glibc-rh1375235-8.patch
Patch1875: glibc-rh1375235-9.patch
Patch1876: glibc-rh1375235-10.patch

# RHBZ #1435615: nscd cache thread hangs
Patch1877: glibc-rh1435615.patch

# RHBZ #1398413: libio: Implement vtable verification
Patch1878: glibc-rh1398413.patch

# RHBZ #1445781:  elf/tst-audit set of tests fails with "no PLTREL"
Patch1879: glibc-rh1445781-1.patch
Patch1880: glibc-rh1445781-2.patch

Patch1881: glibc-rh1500908.patch
Patch1882: glibc-rh1448822.patch
Patch1883: glibc-rh1468807.patch
Patch1884: glibc-rh1372305.patch
Patch1885: glibc-rh1349962.patch
Patch1886: glibc-rh1349964.patch
Patch1887: glibc-rh1440250.patch
Patch1888: glibc-rh1504809-1.patch
Patch1889: glibc-rh1504809-2.patch
Patch1890: glibc-rh1504969.patch
Patch1891: glibc-rh1498925-1.patch
Patch1892: glibc-rh1498925-2.patch

# RHBZ #1503854: Pegas1.0 - Update HWCAP bits for POWER9 DD2.1
Patch1893: glibc-rh1503854-1.patch
Patch1894: glibc-rh1503854-2.patch
Patch1895: glibc-rh1503854-3.patch

# RHBZ #1527904: PTHREAD_STACK_MIN is too small on x86_64
Patch1896: glibc-rh1527904-1.patch
Patch1897: glibc-rh1527904-2.patch
Patch1898: glibc-rh1527904-3.patch
Patch1899: glibc-rh1527904-4.patch

# RHBZ #1534635: CVE-2018-1000001 glibc: realpath() buffer underflow.
Patch1900: glibc-rh1534635.patch

# RHBZ #1529982: recompile glibc to fix incorrect CFI information on i386.
Patch1901: glibc-rh1529982.patch

Patch1902: glibc-rh1523119-compat-symbols.patch
Patch2500: glibc-rh1505492-nscd_stat.patch
Patch2501: glibc-rh1564638.patch
Patch2502: glibc-rh1566623.patch
Patch2503: glibc-rh1349967.patch
Patch2504: glibc-rh1505492-undef-malloc.patch
Patch2505: glibc-rh1505492-undef-elf-dtv-resize.patch
Patch2506: glibc-rh1505492-undef-elision.patch
Patch2507: glibc-rh1505492-undef-max_align_t.patch
Patch2508: glibc-rh1505492-unused-tst-default-attr.patch
Patch2509: glibc-rh1505492-prototypes-rtkaio.patch
Patch2510: glibc-rh1505492-zerodiv-log.patch
Patch2511: glibc-rh1505492-selinux.patch
Patch2512: glibc-rh1505492-undef-abi.patch
Patch2513: glibc-rh1505492-unused-math.patch
Patch2514: glibc-rh1505492-prototypes-1.patch
Patch2515: glibc-rh1505492-uninit-intl-plural.patch
Patch2516: glibc-rh1505492-undef-1.patch
Patch2517: glibc-rh1505492-undef-2.patch
Patch2518: glibc-rh1505492-bounded-1.patch
Patch2519: glibc-rh1505492-bounded-2.patch
Patch2520: glibc-rh1505492-bounded-3.patch
Patch2521: glibc-rh1505492-bounded-4.patch
Patch2522: glibc-rh1505492-undef-3.patch
Patch2523: glibc-rh1505492-bounded-5.patch
Patch2524: glibc-rh1505492-bounded-6.patch
Patch2525: glibc-rh1505492-bounded-7.patch
Patch2526: glibc-rh1505492-bounded-8.patch
Patch2527: glibc-rh1505492-unused-1.patch
Patch2528: glibc-rh1505492-bounded-9.patch
Patch2529: glibc-rh1505492-bounded-10.patch
Patch2530: glibc-rh1505492-bounded-11.patch
Patch2531: glibc-rh1505492-bounded-12.patch
Patch2532: glibc-rh1505492-bounded-13.patch
Patch2533: glibc-rh1505492-unused-2.patch
Patch2534: glibc-rh1505492-bounded-14.patch
Patch2535: glibc-rh1505492-bounded-15.patch
Patch2536: glibc-rh1505492-bounded-16.patch
Patch2537: glibc-rh1505492-bounded-17.patch
Patch2538: glibc-rh1505492-malloc_size_t.patch
Patch2539: glibc-rh1505492-malloc_ptrdiff_t.patch
Patch2540: glibc-rh1505492-prototypes-2.patch
Patch2541: glibc-rh1505492-prototypes-libc_fatal.patch
Patch2542: glibc-rh1505492-getlogin.patch
Patch2543: glibc-rh1505492-undef-4.patch
Patch2544: glibc-rh1505492-register.patch
Patch2545: glibc-rh1505492-prototypes-3.patch
Patch2546: glibc-rh1505492-unused-3.patch
Patch2547: glibc-rh1505492-ports-move-powerpc.patch
Patch2548: glibc-rh1505492-unused-4.patch
Patch2549: glibc-rh1505492-systemtap.patch
Patch2550: glibc-rh1505492-prototypes-wcschr-1.patch
Patch2551: glibc-rh1505492-prototypes-wcsrchr.patch
Patch2552: glibc-rh1505492-prototypes-powerpc-wcscpy.patch
Patch2553: glibc-rh1505492-prototypes-powerpc-wordcopy.patch
Patch2554: glibc-rh1505492-bsd-flatten.patch
Patch2555: glibc-rh1505492-unused-5.patch
Patch2556: glibc-rh1505492-types-1.patch
Patch2557: glibc-rh1505492-powerpc-sotruss.patch
Patch2558: glibc-rh1505492-s390x-sotruss.patch
Patch2559: glibc-rh1505492-ports-am33.patch
Patch2560: glibc-rh1505492-ports-move-arm.patch
Patch2561: glibc-rh1505492-undef-5.patch
Patch2562: glibc-rh1505492-prototypes-4.patch
Patch2563: glibc-rh1505492-ports-move-tile.patch
Patch2564: glibc-rh1505492-ports-move-m68k.patch
Patch2565: glibc-rh1505492-ports-move-mips.patch
Patch2566: glibc-rh1505492-ports-move-aarch64.patch
Patch2567: glibc-rh1505492-ports-move-alpha.patch
Patch2568: glibc-rh1505492-ports-move-ia64.patch
Patch2569: glibc-rh1505492-undef-6.patch
Patch2570: glibc-rh1505492-undef-7.patch
Patch2571: glibc-rh1505492-undef-intl.patch
Patch2572: glibc-rh1505492-undef-obstack.patch
Patch2573: glibc-rh1505492-undef-error.patch
Patch2574: glibc-rh1505492-undef-string.patch
Patch2575: glibc-rh1505492-undef-tempname.patch
Patch2576: glibc-rh1505492-undef-8.patch
Patch2577: glibc-rh1505492-undef-mktime.patch
Patch2578: glibc-rh1505492-undef-sysconf.patch
Patch2579: glibc-rh1505492-prototypes-Xat.patch
Patch2580: glibc-rh1505492-undef-ipc64.patch
Patch2581: glibc-rh1505492-undef-xfs-chown.patch
Patch2582: glibc-rh1505492-undef-9.patch
Patch2583: glibc-rh1505492-undef-10.patch
Patch2584: glibc-rh1505492-undef-11.patch
Patch2585: glibc-rh1505492-undef-12.patch
Patch2586: glibc-rh1505492-prototypes-5.patch
Patch2587: glibc-rh1505492-undef-13.patch
Patch2588: glibc-rh1505492-undef-14.patch
Patch2589: glibc-rh1505492-undef-15.patch
Patch2590: glibc-rh1505492-ports-move-hppa.patch
Patch2591: glibc-rh1505492-undef-16.patch
Patch2592: glibc-rh1505492-undef-17.patch
Patch2593: glibc-rh1505492-undef-18.patch
Patch2594: glibc-rh1505492-undef-19.patch
Patch2595: glibc-rh1505492-undef-20.patch
Patch2596: glibc-rh1505492-undef-21.patch
Patch2597: glibc-rh1505492-undef-22.patch
Patch2598: glibc-rh1505492-undef-23.patch
Patch2599: glibc-rh1505492-undef-24.patch
Patch2600: glibc-rh1505492-prototypes-rwlock.patch
Patch2601: glibc-rh1505492-undef-25.patch
Patch2602: glibc-rh1505492-undef-26.patch
Patch2603: glibc-rh1505492-undef-27.patch
Patch2604: glibc-rh1505492-undef-28.patch
Patch2605: glibc-rh1505492-undef-29.patch
Patch2606: glibc-rh1505492-undef-30.patch
Patch2607: glibc-rh1505492-undef-31.patch
Patch2608: glibc-rh1505492-undef-32.patch
Patch2609: glibc-rh1505492-undef-33.patch
Patch2610: glibc-rh1505492-prototypes-memchr.patch
Patch2611: glibc-rh1505492-undef-34.patch
Patch2612: glibc-rh1505492-prototypes-powerpc-memmove.patch
Patch2613: glibc-rh1505492-undef-35.patch
Patch2614: glibc-rh1505492-undef-36.patch
Patch2615: glibc-rh1505492-undef-37.patch
Patch2616: glibc-rh1505492-uninit-1.patch
Patch2617: glibc-rh1505492-undef-38.patch
Patch2618: glibc-rh1505492-uninit-2.patch
Patch2619: glibc-rh1505492-undef-39.patch
Patch2620: glibc-rh1505492-undef-40.patch
Patch2621: glibc-rh1505492-undef-41.patch
Patch2622: glibc-rh1505492-undef-42.patch
Patch2623: glibc-rh1505492-undef-43.patch
Patch2624: glibc-rh1505492-undef-44.patch
Patch2625: glibc-rh1505492-undef-45.patch
Patch2626: glibc-rh1505492-undef-46.patch
Patch2627: glibc-rh1505492-undef-47.patch
Patch2628: glibc-rh1505492-prototypes-6.patch
Patch2629: glibc-rh1505492-undef-48.patch
Patch2630: glibc-rh1505492-prototypes-execve.patch
Patch2631: glibc-rh1505492-prototypes-readv-writev.patch
Patch2632: glibc-rh1505492-prototypes-7.patch
Patch2633: glibc-rh1505492-prototypes-powerpc-pread-pwrite.patch
Patch2634: glibc-rh1505492-s390-backtrace.patch
Patch2635: glibc-rh1505492-unused-6.patch
Patch2636: glibc-rh1505492-prototypes-8.patch
Patch2637: glibc-rh1505492-prototypes-ctermid.patch
Patch2638: glibc-rh1505492-unused-7.patch
Patch2639: glibc-rh1505492-uninit-3.patch
Patch2640: glibc-rh1505492-types-2.patch
Patch2641: glibc-rh1505492-unused-8.patch
Patch2642: glibc-rh1505492-unused-9.patch
Patch2643: glibc-rh1505492-types-3.patch
Patch2644: glibc-rh1505492-unused-10.patch
Patch2645: glibc-rh1505492-types-5.patch
Patch2646: glibc-rh1505492-unused-11.patch
Patch2647: glibc-rh1505492-unused-12.patch
Patch2648: glibc-rh1505492-unused-13.patch
Patch2649: glibc-rh1505492-deprecated-1.patch
Patch2650: glibc-rh1505492-unused-14.patch
Patch2651: glibc-rh1505492-types-6.patch
Patch2652: glibc-rh1505492-address.patch
Patch2653: glibc-rh1505492-types-7.patch
Patch2654: glibc-rh1505492-prototypes-9.patch
Patch2655: glibc-rh1505492-diag.patch
Patch2656: glibc-rh1505492-zerodiv-1.patch
Patch2657: glibc-rh1505492-deprecated-2.patch
Patch2658: glibc-rh1505492-unused-15.patch
Patch2659: glibc-rh1505492-prototypes-sigvec.patch
Patch2660: glibc-rh1505492-werror-activate.patch
Patch2661: glibc-rh1505492-types-8.patch
Patch2662: glibc-rh1505492-prototypes-intl.patch
Patch2663: glibc-rh1505492-types-9.patch
Patch2664: glibc-rh1505492-types-10.patch
Patch2665: glibc-rh1505492-prototypes-sem_unlink.patch
Patch2666: glibc-rh1505492-prototypes-s390-pthread_once.patch
Patch2667: glibc-rh1505492-types-11.patch
Patch2668: glibc-rh1505492-types-12.patch
Patch2669: glibc-rh1505492-types-13.patch
Patch2670: glibc-rh1505492-undef-49.patch
Patch2671: glibc-rh1505492-undef-50.patch
Patch2672: glibc-rh1505492-undef-51.patch
Patch2673: glibc-rh1505492-undef-52.patch
Patch2674: glibc-rh1505492-types-14.patch
Patch2675: glibc-rh1505492-prototypes-10.patch
Patch2676: glibc-rh1505492-prototypes-wcschr-2.patch
Patch2677: glibc-rh1505492-prototypes-bzero.patch
Patch2678: glibc-rh1505492-winline.patch
Patch2679: glibc-rh1505492-prototypes-scandir.patch
Patch2680: glibc-rh1505492-prototypes-timespec_get.patch
Patch2681: glibc-rh1505492-prototypes-gettimeofday.patch
Patch2682: glibc-rh1505492-prototypes-no_cancellation.patch
Patch2683: glibc-rh1505492-prototypes-getttynam.patch
Patch2684: glibc-rh1505492-undef-53.patch
Patch2685: glibc-rh1505492-prototypes-stpcpy.patch
Patch2686: glibc-rh1505492-undef-54.patch
Patch2687: glibc-rh1505492-undef-55.patch
Patch2688: glibc-rh1505492-undef-activate.patch
Patch2689: glibc-rh1505492-prototypes-debug.patch
Patch2690: glibc-rh1505492-prototypes-putXent.patch
Patch2691: glibc-rh1505492-prototypes-11.patch
Patch2692: glibc-rh1505492-prototypes-12.patch
Patch2693: glibc-rh1505492-prototypes-13.patch
Patch2694: glibc-rh1505492-prototypes-14.patch
Patch2695: glibc-rh1505492-prototypes-15.patch
Patch2696: glibc-rh1505492-prototypes-16.patch
Patch2697: glibc-rh1505492-prototypes-17.patch
Patch2698: glibc-rh1505492-prototypes-18.patch
Patch2699: glibc-rh1505492-prototypes-activate.patch
Patch2700: glibc-rh1505492-unused-16.patch
Patch2701: glibc-rh1505492-unused-17.patch
Patch2702: glibc-rh1505492-undef-56.patch
Patch2703: glibc-rh1548002.patch
Patch2704: glibc-rh1307241-1.patch
Patch2705: glibc-rh1307241-2.patch

Patch2706: glibc-rh1563747.patch
Patch2707: glibc-rh1476120.patch
Patch2708: glibc-rh1505647.patch

Patch2709: glibc-rh1457479-1.patch
Patch2710: glibc-rh1457479-2.patch
Patch2711: glibc-rh1457479-3.patch
Patch2712: glibc-rh1457479-4.patch
Patch2713: glibc-rh1461231.patch

Patch2714: glibc-rh1577333.patch
Patch2715: glibc-rh1531168-1.patch
Patch2716: glibc-rh1531168-2.patch
Patch2717: glibc-rh1531168-3.patch
Patch2718: glibc-rh1579742.patch
Patch2719: glibc-rh1579727-1.patch
Patch2720: glibc-rh1579727-2.patch
Patch2721: glibc-rh1579809-1.patch
Patch2722: glibc-rh1579809-2.patch
Patch2723: glibc-rh1505451.patch
Patch2724: glibc-rh1505477.patch
Patch2725: glibc-rh1505500.patch
Patch2726: glibc-rh1563046.patch

# RHBZ 1560641 - backport of upstream sem_open patch
Patch2727: glibc-rh1560641.patch

Patch2728: glibc-rh1550080.patch
Patch2729: glibc-rh1526193.patch
Patch2730: glibc-rh1372304-1.patch
Patch2731: glibc-rh1372304-2.patch
Patch2732: glibc-rh1540480-0.patch
Patch2733: glibc-rh1540480-1.patch
Patch2734: glibc-rh1540480-2.patch
Patch2735: glibc-rh1540480-3.patch
Patch2736: glibc-rh1540480-4.patch
Patch2737: glibc-rh1540480-5.patch
Patch2738: glibc-rh1540480-6.patch
Patch2739: glibc-rh1540480-7.patch
Patch2740: glibc-rh1540480-8.patch
Patch2741: glibc-rh1447808-0.patch
Patch2742: glibc-rh1447808-1.patch
Patch2743: glibc-rh1447808-2.patch
Patch2744: glibc-rh1401665-0.patch
Patch2745: glibc-rh1401665-1a.patch
Patch2746: glibc-rh1401665-1b.patch
Patch2747: glibc-rh1401665-1c.patch
Patch2748: glibc-rh1401665-2.patch
Patch2749: glibc-rh1401665-3.patch
Patch2750: glibc-rh1401665-4.patch
Patch2751: glibc-rh1401665-5.patch

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

# Backport of fix for malloc arena free list management (upstream bug 19048)
# The preparatory patch removes !PER_THREAD conditional code.
Patch20670: glibc-rh1276753-0.patch
Patch2067: glibc-rh1276753.patch

# Backport to fix ld.so crash when audit modules provide path (upstream bug 18251)
Patch2068: glibc-rh1211100.patch

# aarch64 MINSIGSTKSZ/SIGSTKSZ fix
Patch2069: glibc-rh1335629.patch
Patch2070: glibc-rh1335925-1.patch
Patch2071: glibc-rh1335925-2.patch
Patch2072: glibc-rh1335925-3.patch
Patch2073: glibc-rh1335925-4.patch

# Do not set initgroups in default nsswitch.conf
Patch2074: glibc-rh1366569.patch

# Various nss_db fixes
Patch2075: glibc-rh1318890.patch
Patch2076: glibc-rh1213603.patch
Patch2077: glibc-rh1370630.patch

# Add internal-only support for O_TMPFILE.
Patch2078: glibc-rh1330705-1.patch
Patch2079: glibc-rh1330705-2.patch
Patch2080: glibc-rh1330705-3.patch
Patch2081: glibc-rh1330705-4.patch
Patch2082: glibc-rh1330705-5.patch
# The following patch *removes* the public definition of O_TMPFILE.
Patch2083: glibc-rh1330705-6.patch

# getaddrinfo with nscd fixes
Patch2084: glibc-rh1324568.patch

# RHBZ #1404435 - Remove power8 platform directory
Patch2085: glibc-rh1404435.patch

# RHBZ #1144516 - aarch64 profil fix
Patch2086: glibc-rh1144516.patch

# RHBZ #1392540 - Add "sss" service to the automount database in nsswitch.conf
Patch2087: glibc-rh1392540.patch

# RHBZ #1452721: Avoid large allocas in the dynamic linker
Patch2088: glibc-rh1452721-1.patch
Patch2089: glibc-rh1452721-2.patch
Patch2090: glibc-rh1452721-3.patch
Patch2091: glibc-rh1452721-4.patch

Patch2092: glibc-rh677316-libc-pointer-arith.patch
Patch2093: glibc-rh677316-libc-lock.patch
Patch2094: glibc-rh677316-libc-diag.patch
Patch2095: glibc-rh677316-check_mul_overflow_size_t.patch
Patch2096: glibc-rh677316-res_state.patch
Patch2097: glibc-rh677316-qsort_r.patch
Patch2098: glibc-rh677316-fgets_unlocked.patch
Patch2099: glibc-rh677316-in6addr_any.patch
Patch2100: glibc-rh677316-netdb-reentrant.patch
Patch2101: glibc-rh677316-h_errno.patch
Patch2102: glibc-rh677316-scratch_buffer.patch
Patch2103: glibc-rh677316-mtrace.patch
Patch2104: glibc-rh677316-dynarray.patch
Patch2105: glibc-rh677316-alloc_buffer.patch
Patch2106: glibc-rh677316-RES_USE_INET6.patch
Patch2107: glibc-rh677316-inet_pton.patch
Patch2108: glibc-rh677316-inet_pton-zeros.patch
Patch2109: glibc-rh677316-hesiod.patch
Patch2110: glibc-rh677316-resolv.patch
Patch2111: glibc-rh677316-legacy.patch

Patch2112: glibc-rh1498566.patch
Patch2113: glibc-rh1445644.patch

Patch2114: glibc-rh1471405.patch

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
%patch20670 -p1
%patch2067 -p1
%patch2068 -p1
%patch2069 -p1
%patch2070 -p1
%patch2071 -p1
%patch2072 -p1
%patch2073 -p1
%patch2074 -p1
%patch2075 -p1
%patch2076 -p1
%patch2077 -p1
%patch2078 -p1
%patch2079 -p1
%patch2080 -p1
%patch2081 -p1
%patch2082 -p1
%patch2083 -p1
%patch2084 -p1
%patch2085 -p1
%patch2086 -p1
%patch2087 -p1
%patch2088 -p1
%patch2089 -p1
%patch2090 -p1
%patch2091 -p1

# Rebase of microbenchmarks.
%patch1607 -p1
%patch1609 -p1
%patch1610 -p1
%patch1611 -p1

# Backport of POWER8 glibc optimizations for RHEL7.3
%patch1612 -p1
%patch1613 -p1
%patch1614 -p1
%patch1615 -p1
%patch1616 -p1
%patch1617 -p1
%patch1618 -p1
%patch1619 -p1
%patch1620 -p1
%patch1621 -p1
%patch1622 -p1
%patch1623 -p1

# Backport of upstream IBM z13 patches for RHEL 7.3
%patch1624 -p1
%patch1625 -p1
%patch1626 -p1
%patch1627 -p1
%patch1628 -p1
%patch1629 -p1
%patch1630 -p1
%patch1631 -p1
%patch1632 -p1
%patch1633 -p1
%patch1634 -p1
%patch1635 -p1
%patch1636 -p1
%patch1637 -p1
%patch1638 -p1
%patch1639 -p1
%patch1640 -p1
%patch1641 -p1
%patch1642 -p1
%patch1643 -p1
%patch1644 -p1
%patch1645 -p1
%patch1646 -p1
%patch1647 -p1
%patch1648 -p1
%patch1649 -p1
%patch1650 -p1
%patch1651 -p1
%patch1652 -p1
%patch1653 -p1
%patch1654 -p1

%patch1123 -p1
%patch1124 -p1

%patch1656 -p1
%patch1657 -p1
%patch1658 -p1
%patch1660 -p1
%patch0067 -p1
%patch1661 -p1
%patch1662 -p1
%patch1663 -p1

%patch1664 -p1
%patch1665 -p1
%patch1666 -p1
%patch1667 -p1
%patch1668 -p1
%patch1669 -p1
%patch1670 -p1
%patch1671 -p1
%patch1672 -p1

%patch1675 -p1

# RHBZ #1324427, parts 1 through 3
%patch1676 -p1
%patch1677 -p1
%patch1678 -p1

# RHBZ #1234449, parts 1 through 4
%patch1679 -p1
%patch1680 -p1
%patch1681 -p1
%patch1682 -p1

# RHBZ #1221046
%patch1683 -p1

# RHBZ #971416
%patch1684 -p1
%patch1685 -p1
%patch1686 -p1

# RHBZ #1302086
%patch1687 -p1
%patch1688 -p1
%patch1689 -p1
%patch1690 -p1
%patch1691 -p1
%patch1692 -p1
%patch1693 -p1
%patch1694 -p1
%patch1695 -p1
%patch1696 -p1
%patch1697 -p1

# RHBZ #1346397
%patch1698 -p1

# RHBZ #1211823
%patch1699 -p1

# RHBZ #1268050, parts 1 through 5
%patch1700 -p1
%patch1701 -p1
%patch1702 -p1
%patch1703 -p1
%patch1704 -p1

# RHBz #1296297, part 1 and 2.
%patch1705 -p1
%patch1706 -p1

# RHBZ #1027348, part 1 through 5.
%patch1707 -p1
%patch1708 -p1
%patch1709 -p1
%patch1710 -p1
%patch1711 -p1

%patch1712 -p1
%patch1713 -p1
%patch1714 -p1

%patch1715 -p1

# RHBZ #1256317, IS_IN backports, parts 1 through 22.
%patch1716 -p1
%patch1717 -p1
%patch1718 -p1
%patch1719 -p1
%patch1720 -p1
%patch1721 -p1
%patch1722 -p1
%patch1723 -p1
%patch1724 -p1
%patch1725 -p1
%patch1726 -p1
%patch1727 -p1
%patch1728 -p1
%patch1729 -p1
%patch1730 -p1
%patch1731 -p1
%patch1732 -p1
%patch1733 -p1
%patch1734 -p1
%patch1735 -p1
%patch1736 -p1
%patch1737 -p1

%patch1738 -p1
%patch1739 -p1

# RHBZ #1292018, patches 1 through 10.
%patch1740 -p1
%patch1741 -p1
%patch1742 -p1
%patch1743 -p1
%patch1744 -p1
%patch1745 -p1
%patch1746 -p1
%patch1747 -p1
%patch1748 -p1
%patch1749 -p1

%patch1750 -p1

# RHBZ #1298526, patch 1 of 5.
%patch1751 -p1
%patch1752 -p1
%patch1753 -p1
%patch1754 -p1
%patch1755 -p1
%patch1756 -p1
%patch0068 -p1
%patch1757 -p1
%patch17580 -p1
%patch1758 -p1
%patch1759 -p1
%patch1760 -p1
%patch1761 -p1
%patch1762 -p1
%patch1763 -p1
%patch1764 -p1
%patch1765 -p1
%patch1766 -p1
%patch1767 -p1
%patch1768 -p1
%patch1769 -p1
%patch1770 -p1
%patch1771 -p1
%patch1772 -p1
%patch1773 -p1
%patch1774 -p1
%patch1775 -p1
%patch1776 -p1
%patch1777 -p1
%patch1778 -p1
%patch1779 -p1
%patch1780 -p1
%patch1781 -p1
%patch1782 -p1
%patch1783 -p1
%patch1784 -p1
%patch1785 -p1
%patch1786 -p1
%patch1787 -p1
%patch1788 -p1
%patch1789 -p1
%patch1790 -p1
%patch1791 -p1
%patch1792 -p1
%patch1793 -p1
%patch1794 -p1
%patch1795 -p1
%patch1796 -p1
%patch1797 -p1
%patch1798 -p1
%patch1799 -p1
%patch1800 -p1
%patch1801 -p1
%patch1802 -p1
%patch1803 -p1
%patch1804 -p1
%patch1805 -p1
%patch1806 -p1
%patch1807 -p1
%patch1808 -p1
%patch1809 -p1
%patch1810 -p1
%patch1811 -p1
%patch1812 -p1
%patch1813 -p1
%patch1814 -p1
%patch1815 -p1
%patch1816 -p1
%patch1817 -p1
%patch1818 -p1
%patch1819 -p1
%patch1820 -p1
%patch1821 -p1
%patch1822 -p1
%patch1823 -p1
%patch1824 -p1
%patch1825 -p1
%patch1826 -p1
%patch1827 -p1
%patch1828 -p1
%patch1829 -p1
%patch1830 -p1
%patch1831 -p1
# RHBZ #841653 - Intel lock elision patch set.
%patch1832 -p1
%patch1833 -p1
%patch1834 -p1
%patch1835 -p1
%patch1836 -p1
%patch1837 -p1
%patch1838 -p1
%patch1839 -p1
%patch1840 -p1
%patch1841 -p1
%patch1842 -p1
%patch1843 -p1
%patch1844 -p1
%patch1845 -p1
%patch1846 -p1
%patch1847 -p1
%patch1848 -p1
%patch1849 -p1
# End of Intel lock elision patch set.
# RHBZ #731835 - IBM POWER lock elision patch set.
%patch1850 -p1
%patch1851 -p1
%patch1852 -p1
# End of IBM POWER lock elision patch set.

%patch1853 -p1
%patch1854 -p1

# Built-in list of syscall names.
%patch1855 -p1
%patch1856 -p1

%patch1857 -p1
%patch1858 -p1
%patch1859 -p1
%patch1860 -p1

%patch1861 -p1
%patch1862 -p1
%patch1863 -p1
%patch1864 -p1
%patch1865 -p1
%patch1866 -p1

%patch1867 -p1
%patch1868 -p1
%patch1869 -p1
%patch1870 -p1
%patch1871 -p1
%patch1872 -p1
%patch1873 -p1
%patch1874 -p1
%patch1875 -p1
%patch1876 -p1

%patch1877 -p1
%patch2092 -p1
%patch2093 -p1
%patch2094 -p1
%patch2095 -p1
%patch2096 -p1
%patch2097 -p1
%patch2098 -p1
%patch2099 -p1
%patch2100 -p1
%patch2101 -p1
%patch2102 -p1
%patch2103 -p1
%patch2104 -p1
%patch2105 -p1
%patch2106 -p1
%patch2107 -p1
%patch2108 -p1
%patch2109 -p1
%patch2110 -p1
%patch2111 -p1
%patch2112 -p1
%patch2113 -p1
%patch2114 -p1

%patch1878 -p1
%patch1879 -p1
%patch1880 -p1

%patch1881 -p1
%patch1882 -p1
%patch1883 -p1
%patch1884 -p1
%patch1885 -p1
%patch1886 -p1
%patch1887 -p1
%patch1888 -p1
%patch1889 -p1
%patch1890 -p1
%patch1891 -p1
%patch1892 -p1

%patch1893 -p1
%patch1894 -p1
%patch1895 -p1
%patch1896 -p1
%patch1897 -p1
%patch1898 -p1
%patch1899 -p1
%patch1900 -p1
%patch1901 -p1
%patch1902 -p1
%patch2500 -p1
%patch2501 -p1
%patch2502 -p1
%patch2503 -p1
%patch2504 -p1
%patch2505 -p1
%patch2506 -p1
%patch2507 -p1
%patch2508 -p1
%patch2509 -p1
%patch2510 -p1
%patch2511 -p1
%patch2512 -p1
%patch2513 -p1
%patch2514 -p1
%patch2515 -p1
%patch2516 -p1
%patch2517 -p1
%patch2518 -p1
%patch2519 -p1
%patch2520 -p1
%patch2521 -p1
%patch2522 -p1
%patch2523 -p1
%patch2524 -p1
%patch2525 -p1
%patch2526 -p1
%patch2527 -p1
%patch2528 -p1
%patch2529 -p1
%patch2530 -p1
%patch2531 -p1
%patch2532 -p1
%patch2533 -p1
%patch2534 -p1
%patch2535 -p1
%patch2536 -p1
%patch2537 -p1
%patch2538 -p1
%patch2539 -p1
%patch2540 -p1
%patch2541 -p1
%patch2542 -p1
%patch2543 -p1
%patch0069 -p1
%patch2544 -p1
%patch2545 -p1
%patch2546 -p1
%patch2547 -p1
%patch2548 -p1
%patch2549 -p1
%patch2550 -p1
%patch2551 -p1
%patch2552 -p1
%patch2553 -p1
%patch2554 -p1
%patch2555 -p1
%patch2556 -p1
%patch2557 -p1
%patch2558 -p1
%patch2559 -p1
%patch2560 -p1
%patch2561 -p1
%patch2562 -p1
%patch2563 -p1
%patch2564 -p1
%patch2565 -p1
%patch2566 -p1
%patch2567 -p1
%patch2568 -p1
%patch2569 -p1
%patch2570 -p1
%patch2571 -p1
%patch2572 -p1
%patch2573 -p1
%patch2574 -p1
%patch2575 -p1
%patch2576 -p1
%patch2577 -p1
%patch2578 -p1
%patch2579 -p1
%patch2580 -p1
%patch2581 -p1
%patch2582 -p1
%patch2583 -p1
%patch2584 -p1
%patch2585 -p1
%patch2586 -p1
%patch2587 -p1
%patch2588 -p1
%patch2589 -p1
%patch2590 -p1
%patch2591 -p1
%patch2592 -p1
%patch2593 -p1
%patch2594 -p1
%patch2595 -p1
%patch2596 -p1
%patch2597 -p1
%patch2598 -p1
%patch2599 -p1
%patch2600 -p1
%patch2601 -p1
%patch2602 -p1
%patch2603 -p1
%patch2604 -p1
%patch2605 -p1
%patch2606 -p1
%patch2607 -p1
%patch2608 -p1
%patch2609 -p1
%patch2610 -p1
%patch2611 -p1
%patch2612 -p1
%patch2613 -p1
%patch2614 -p1
%patch2615 -p1
%patch2616 -p1
%patch2617 -p1
%patch2618 -p1
%patch2619 -p1
%patch2620 -p1
%patch2621 -p1
%patch2622 -p1
%patch2623 -p1
%patch2624 -p1
%patch2625 -p1
%patch2626 -p1
%patch2627 -p1
%patch2628 -p1
%patch2629 -p1
%patch2630 -p1
%patch2631 -p1
%patch2632 -p1
%patch2633 -p1
%patch2634 -p1
%patch2635 -p1
%patch2636 -p1
%patch2637 -p1
%patch2638 -p1
%patch2639 -p1
%patch2640 -p1
%patch2641 -p1
%patch2642 -p1
%patch2643 -p1
%patch2644 -p1
%patch2645 -p1
%patch2646 -p1
%patch2647 -p1
%patch2648 -p1
%patch2649 -p1
%patch2650 -p1
%patch2651 -p1
%patch2652 -p1
%patch2653 -p1
%patch2654 -p1
%patch2655 -p1
%patch2656 -p1
%patch2657 -p1
%patch2658 -p1
%patch2659 -p1
%patch2660 -p1
%patch2661 -p1
%patch2662 -p1
%patch2663 -p1
%patch2664 -p1
%patch2665 -p1
%patch2666 -p1
%patch2667 -p1
%patch2668 -p1
%patch2669 -p1
%patch2670 -p1
%patch2671 -p1
%patch2672 -p1
%patch2673 -p1
%patch2674 -p1
%patch2675 -p1
%patch2676 -p1
%patch2677 -p1
%patch2678 -p1
%patch2679 -p1
%patch2680 -p1
%patch2681 -p1
%patch2682 -p1
%patch2683 -p1
%patch2684 -p1
%patch2685 -p1
%patch2686 -p1
%patch2687 -p1
%patch2688 -p1
%patch2689 -p1
%patch2690 -p1
%patch2691 -p1
%patch2692 -p1
%patch2693 -p1
%patch2694 -p1
%patch2695 -p1
%patch2696 -p1
%patch2697 -p1
%patch2698 -p1
%patch2699 -p1
%patch2700 -p1
%patch2701 -p1
%patch2702 -p1
%patch2703 -p1
%patch2704 -p1
%patch2705 -p1

%patch2706 -p1
%patch2707 -p1
%patch2708 -p1

%patch2709 -p1
%patch2710 -p1
%patch2711 -p1
%patch2712 -p1
%patch2713 -p1

%patch2714 -p1
%patch2715 -p1
%patch2716 -p1
%patch2717 -p1
%patch2718 -p1
%patch2719 -p1
%patch2720 -p1
%patch2721 -p1
%patch2722 -p1
%patch2723 -p1
%patch2724 -p1
%patch2725 -p1
%patch2726 -p1
%patch2727 -p1
%patch2728 -p1
%patch2729 -p1
%patch2730 -p1
%patch2731 -p1
%patch2732 -p1
%patch2733 -p1
%patch2734 -p1
%patch2735 -p1
%patch2736 -p1
%patch2737 -p1
%patch2738 -p1
%patch2739 -p1
%patch2740 -p1
%patch2741 -p1
%patch2742 -p1
%patch2743 -p1
%patch2744 -p1
%patch2745 -p1
%patch2746 -p1
%patch2747 -p1
%patch2748 -p1
%patch2749 -p1
%patch2750 -p1
%patch2751 -p1

# PiP
%patch9999 -p1

%description
glibc with patches for PiP

##############################################################################
# Build glibc...
##############################################################################
%build
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

mkdir build &&
(
	cd build &&
	DESTDIR="$RPM_BUILD_ROOT" sh ../build.sh -b %{pip_glibc_dir}
)

%install

(
	cd build &&
	DESTDIR="$RPM_BUILD_ROOT" sh ../build.sh -i %{pip_glibc_dir}
)

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

%post
%{pip_glibc_dir}/bin/piplnlibs -rs

%preun
%{pip_glibc_dir}/bin/piplnlibs -Rs


%files
%defattr(-,root,root)
%{pip_glibc_dir}
