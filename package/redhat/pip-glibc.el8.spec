# How to build this RPM:
#
#	$ rpm -Uvh glibc-2.28-72.el8_1.1.src.rpm
#	$ cd .../$SOMEWHERE/.../PiP-glibc
#	$ git diff origin/centos/glibc-2.28-72.el8_1.1.branch centos/glibc-2.28-72.el8_1.1.pip.branch >~/rpmbuild/SOURCES/glibc-el8-pip3.patch
#	$ rpmbuild -bb ~/rpmbuild/SPECS/pip-glibc.el8.spec
#

%define	pip_glibc_dir		/opt/pip
%define	pip_glibc_release	pip3

# disable strip, otherwise the pip-gdb shows the following error:
#	warning: Unable to find libthread_db matching inferior's thread library, thread debugging will not be available.
%define __spec_install_post /usr/lib/rpm/brp-compress
%define __os_install_post /usr/lib/rpm/brp-compress
%define debug_package %{nil}

%define glibcsrcdir glibc-2.28
%define glibcversion 2.28
%define glibcrelease 72%{?dist}.1.%{pip_glibc_release}

##############################################################################
# %%package glibc - The GNU C Library (glibc) core package.
##############################################################################
Summary: The GNU libc libraries
Name: pip-glibc
Version: %{glibcversion}
Release: %{glibcrelease}

# In general, GPLv2+ is used by programs, LGPLv2+ is used for
# libraries.
#
# LGPLv2+ with exceptions is used for things that are linked directly
# into dynamically linked programs and shared libraries (e.g. crt
# files, lib*_nonshared.a).  Historically, this exception also applies
# to parts of libio.
#
# GPLv2+ with exceptions is used for parts of the Arm unwinder.
#
# GFDL is used for the documentation.
#
# Some other licenses are used in various places (BSD, Inner-Net,
# ISC, Public Domain).
#
# HSRL and FSFAP are only used in test cases, which currently do not
# ship in binary RPMs, so they are not listed here.  MIT is used for
# scripts/install-sh, which does not ship, either.
#
# GPLv3+ is used by manual/texinfo.tex, which we do not use.
#
# LGPLv3+ is used by some Hurd code, which we do not build.
#
# LGPLv2 is used in one place (time/timespec_get.c, by mistake), but
# it is not actually compiled, so it does not matter for libraries.
License: LGPLv2+ and LGPLv2+ with exceptions and GPLv2+ and GPLv2+ with exceptions and BSD and Inner-Net and ISC and Public Domain and GFDL

URL: http://www.gnu.org/software/glibc/
Source0: %{?glibc_release_url}%{glibcsrcdir}.tar.xz
Source1: build-locale-archive.c
Source4: nscd.conf
Source7: nsswitch.conf
Source8: power6emul.c
Source9: bench.mk
Source10: glibc-bench-compare
# A copy of localedata/SUPPORTED in the Source0 tarball.  The
# SUPPORTED file is used below to generate the list of locale
# packages, using a Lua snippet.
Source11: SUPPORTED

# Include in the source RPM for reference.
Source12: ChangeLog.old

##############################################################################
# Patches:
# - See each individual patch file for origin and upstream status.
# - For new patches follow template.patch format.
##############################################################################
Patch2: glibc-fedora-nscd.patch
Patch3: glibc-rh697421.patch
Patch4: glibc-fedora-linux-tcsetattr.patch
Patch5: glibc-rh741105.patch
Patch6: glibc-fedora-localedef.patch
Patch7: glibc-fedora-nis-rh188246.patch
Patch8: glibc-fedora-manual-dircategory.patch
Patch9: glibc-rh827510.patch
Patch10: glibc-fedora-locarchive.patch
Patch11: glibc-fedora-streams-rh436349.patch
Patch12: glibc-rh819430.patch
Patch13: glibc-fedora-localedata-rh61908.patch
Patch14: glibc-fedora-__libc_multiple_libcs.patch
Patch15: glibc-rh1070416.patch
Patch16: glibc-nscd-sysconfig.patch
Patch17: glibc-cs-path.patch
Patch18: glibc-c-utf8-locale.patch
Patch23: glibc-python3.patch
Patch24: glibc-with-nonshared-cflags.patch
Patch25: glibc-asflags.patch
Patch27: glibc-rh1614253.patch
Patch28: glibc-rh1577365.patch
Patch29: glibc-rh1615781.patch
Patch30: glibc-rh1615784.patch
Patch31: glibc-rh1615790.patch
Patch32: glibc-rh1622675.patch
Patch33: glibc-rh1622678-1.patch
Patch34: glibc-rh1622678-2.patch
Patch35: glibc-rh1631293-1.patch
Patch36: glibc-rh1631293-2.patch
Patch37: glibc-rh1623536.patch
Patch38: glibc-rh1631722.patch
Patch39: glibc-rh1631730.patch
Patch40: glibc-rh1623536-2.patch
Patch41: glibc-rh1614979.patch
Patch42: glibc-rh1645593.patch
Patch43: glibc-rh1645596.patch
Patch44: glibc-rh1645604.patch
Patch45: glibc-rh1646379.patch
Patch46: glibc-rh1645601.patch
Patch52: glibc-rh1638523-1.patch
Patch47: glibc-rh1638523-2.patch
Patch48: glibc-rh1638523-3.patch
Patch49: glibc-rh1638523-4.patch
Patch50: glibc-rh1638523-5.patch
Patch51: glibc-rh1638523-6.patch
Patch53: glibc-rh1641982.patch
Patch54: glibc-rh1645597.patch
Patch55: glibc-rh1650560-1.patch
Patch56: glibc-rh1650560-2.patch
Patch57: glibc-rh1650563.patch
Patch58: glibc-rh1650566.patch
Patch59: glibc-rh1650571.patch
Patch60: glibc-rh1638520.patch
Patch61: glibc-rh1651274.patch
Patch62: glibc-rh1654010-1.patch
Patch63: glibc-rh1635779.patch
Patch64: glibc-rh1654010-2.patch
Patch65: glibc-rh1654010-3.patch
Patch66: glibc-rh1654010-4.patch
Patch67: glibc-rh1654010-5.patch
Patch68: glibc-rh1654010-6.patch
Patch69: glibc-rh1642094-1.patch
Patch70: glibc-rh1642094-2.patch
Patch71: glibc-rh1642094-3.patch
Patch72: glibc-rh1654872-1.patch
Patch73: glibc-rh1654872-2.patch
Patch74: glibc-rh1651283-1.patch
Patch75: glibc-rh1662843-1.patch
Patch76: glibc-rh1662843-2.patch
Patch77: glibc-rh1623537.patch
Patch78: glibc-rh1577438.patch
Patch79: glibc-rh1664408.patch
Patch80: glibc-rh1651742.patch
Patch81: glibc-rh1672773.patch
Patch82: glibc-rh1651283-2.patch
Patch83: glibc-rh1651283-3.patch
Patch84: glibc-rh1651283-4.patch
Patch85: glibc-rh1651283-5.patch
Patch86: glibc-rh1651283-6.patch
Patch87: glibc-rh1651283-7.patch
Patch88: glibc-rh1659293-1.patch
Patch89: glibc-rh1659293-2.patch
Patch90: glibc-rh1639343-1.patch
Patch91: glibc-rh1639343-2.patch
Patch92: glibc-rh1639343-3.patch
Patch93: glibc-rh1639343-4.patch
Patch94: glibc-rh1639343-5.patch
Patch95: glibc-rh1639343-6.patch
Patch96: glibc-rh1663035.patch
Patch97: glibc-rh1658901.patch
Patch98: glibc-rh1659512-1.patch
Patch99: glibc-rh1659512-2.patch
Patch100: glibc-rh1659438-1.patch
Patch101: glibc-rh1659438-2.patch
Patch102: glibc-rh1659438-3.patch
Patch103: glibc-rh1659438-4.patch
Patch104: glibc-rh1659438-5.patch
Patch105: glibc-rh1659438-6.patch
Patch106: glibc-rh1659438-7.patch
Patch107: glibc-rh1659438-8.patch
Patch108: glibc-rh1659438-9.patch
Patch109: glibc-rh1659438-10.patch
Patch110: glibc-rh1659438-11.patch
Patch111: glibc-rh1659438-12.patch
Patch112: glibc-rh1659438-13.patch
Patch113: glibc-rh1659438-14.patch
Patch114: glibc-rh1659438-15.patch
Patch115: glibc-rh1659438-16.patch
Patch116: glibc-rh1659438-17.patch
Patch117: glibc-rh1659438-18.patch
Patch118: glibc-rh1659438-19.patch
Patch119: glibc-rh1659438-20.patch
Patch120: glibc-rh1659438-21.patch
Patch121: glibc-rh1659438-22.patch
Patch122: glibc-rh1659438-23.patch
Patch123: glibc-rh1659438-24.patch
Patch124: glibc-rh1659438-25.patch
Patch125: glibc-rh1659438-26.patch
Patch126: glibc-rh1659438-27.patch
Patch127: glibc-rh1659438-28.patch
Patch128: glibc-rh1659438-29.patch
Patch129: glibc-rh1659438-30.patch
Patch130: glibc-rh1659438-31.patch
Patch131: glibc-rh1659438-32.patch
Patch132: glibc-rh1659438-33.patch
Patch133: glibc-rh1659438-34.patch
Patch134: glibc-rh1659438-35.patch
Patch135: glibc-rh1659438-36.patch
Patch136: glibc-rh1659438-37.patch
Patch137: glibc-rh1659438-38.patch
Patch138: glibc-rh1659438-39.patch
Patch139: glibc-rh1659438-40.patch
Patch140: glibc-rh1659438-41.patch
Patch141: glibc-rh1659438-42.patch
Patch142: glibc-rh1659438-43.patch
Patch143: glibc-rh1659438-44.patch
Patch144: glibc-rh1659438-45.patch
Patch145: glibc-rh1659438-46.patch
Patch146: glibc-rh1659438-47.patch
Patch147: glibc-rh1659438-48.patch
Patch148: glibc-rh1659438-49.patch
Patch149: glibc-rh1659438-50.patch
Patch150: glibc-rh1659438-51.patch
Patch151: glibc-rh1659438-52.patch
Patch152: glibc-rh1659438-53.patch
Patch153: glibc-rh1659438-54.patch
Patch154: glibc-rh1659438-55.patch
Patch155: glibc-rh1659438-56.patch
Patch156: glibc-rh1659438-57.patch
Patch157: glibc-rh1659438-58.patch
Patch158: glibc-rh1659438-59.patch
Patch159: glibc-rh1659438-60.patch
Patch160: glibc-rh1659438-61.patch
Patch161: glibc-rh1659438-62.patch
Patch162: glibc-rh1702539-1.patch
Patch163: glibc-rh1702539-2.patch
Patch164: glibc-rh1701605-1.patch
Patch165: glibc-rh1701605-2.patch
Patch166: glibc-rh1691528-1.patch
Patch167: glibc-rh1691528-2.patch
Patch168: glibc-rh1706777.patch
Patch169: glibc-rh1710478.patch
Patch170: glibc-rh1670043-1.patch
Patch171: glibc-rh1670043-2.patch
Patch172: glibc-rh1710894.patch
Patch173: glibc-rh1699194-1.patch
Patch174: glibc-rh1699194-2.patch
Patch175: glibc-rh1699194-3.patch
Patch176: glibc-rh1699194-4.patch
Patch177: glibc-rh1727241-1.patch
Patch178: glibc-rh1727241-2.patch
Patch179: glibc-rh1727241-3.patch
Patch180: glibc-rh1717438.patch
Patch181: glibc-rh1727152.patch
Patch182: glibc-rh1724975.patch
Patch183: glibc-rh1722215.patch
Patch184: glibc-rh1777797.patch

# PiP
Patch9999: glibc-el8-pip3.patch

##############################################################################
# Continued list of core "glibc" package information:
##############################################################################

BuildRequires: zlib-devel
BuildRequires: libselinux-devel >= 1.33.4-3
BuildRequires: sed >= 3.95, gettext
# We need procps-ng (/bin/ps), util-linux (/bin/kill), and gawk (/bin/awk),
# but it is more flexible to require the actual programs and let rpm infer
# the packages. However, until bug 1259054 is widely fixed we avoid the
# following:
# BuildRequires: /bin/ps, /bin/kill, /bin/awk
# And use instead (which should be reverted some time in the future):
BuildRequires: procps-ng, util-linux, gawk
BuildRequires: systemtap-sdt-devel

# We use systemd rpm macros for nscd
BuildRequires: systemd

BuildRequires: gcc >= 8.2.1-3.4
%ifarch %{arm}
%define target %{_target_cpu}-redhat-linuxeabi
%endif

# GNU make 4.0 introduced the -O option.
BuildRequires: make >= 4.0

# The intl subsystem generates a parser using bison.
BuildRequires: bison >= 2.7

# binutils 2.30-51 is needed for z13 support on s390x.
BuildRequires: binutils >= 2.30-51

# Earlier releases have broken support for IRELATIVE relocations
Conflicts: prelink < 0.4.2

%if 0%{?_enable_debug_packages}
BuildRequires: elfutils >= 0.72
BuildRequires: rpm >= 4.2-0.56
%endif

%description
glibc with patches for PiP

##############################################################################
# Prepare for the build.
##############################################################################
%prep
%autosetup -n %{glibcsrcdir} -p1

##############################################################################
# %%prep - Additional prep required...
##############################################################################
# Make benchmark scripts executable
chmod +x benchtests/scripts/*.py scripts/pylint

# Remove all files generated from patching.
find . -type f -size 0 -o -name "*.orig" -exec rm -f {} \;

# Ensure timestamps on configure files are current to prevent
# regenerating them.
touch `find . -name configure`

# Ensure *-kw.h files are current to prevent regenerating them.
touch locale/programs/*-kw.h

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
