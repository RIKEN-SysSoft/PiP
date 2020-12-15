%define glibcsrcdir glibc-2.28
%define glibcversion 2.28
%define glibcrelease 72%{?dist}.1
# Pre-release tarballs are pulled in from git using a command that is
# effectively:
#
# git archive HEAD --format=tar --prefix=$(git describe --match 'glibc-*')/ \
#	> $(git describe --match 'glibc-*').tar
# gzip -9 $(git describe --match 'glibc-*').tar
#
# glibc_release_url is only defined when we have a release tarball.
%{lua: if string.match(rpm.expand("%glibcsrcdir"), "^glibc%-[0-9.]+$") then
  rpm.define("glibc_release_url https://ftp.gnu.org/gnu/glibc/") end}
##############################################################################
# We support the following options:
# --with/--without,
# * testsuite - Running the testsuite.
# * benchtests - Running and building benchmark subpackage.
# * bootstrap - Bootstrapping the package.
# * werror - Build with -Werror
# * docs - Build with documentation and the required dependencies.
# * valgrind - Run smoke tests with valgrind to verify dynamic loader.
#
# You must always run the testsuite for production builds.
# Default: Always run the testsuite.
%bcond_without testsuite
# Default: Always build the benchtests.
%bcond_without benchtests
# Default: Not bootstrapping.
%bcond_with bootstrap
# Default: Enable using -Werror
%bcond_without werror
# Default: Always build documentation.
%bcond_without docs

# Default: Always run valgrind tests if there is architecture support.
%ifarch %{valgrind_arches}
%bcond_without valgrind
%else
%bcond_with valgrind
%endif
# Restrict %%{valgrind_arches} further in case there are problems with
# the smoke test.
%if %{with valgrind}
%ifarch ppc64 ppc64p7
# The valgrind smoke test does not work on ppc64, ppc64p7 (bug 1273103).
%undefine with_valgrind
%endif
%endif

%if %{with bootstrap}
# Disable benchtests, -Werror, docs, and valgrind if we're bootstrapping
%undefine with_benchtests
%undefine with_werror
%undefine with_docs
%undefine with_valgrind
%endif
##############################################################################
# Auxiliary arches are those arches that can be built in addition
# to the core supported arches. You either install an auxarch or
# you install the base arch, not both. You would do this in order
# to provide a more optimized version of the package for your arch.
%define auxarches athlon alphaev6

# Only some architectures have static PIE support.
%define pie_arches %{ix86} x86_64

# Build the POWER9 runtime on POWER, but only for downstream.
%ifarch ppc64le
%define buildpower9 0%{?rhel} > 0
%else
%define buildpower9 0
%endif

##############################################################################
# Any architecture/kernel combination that supports running 32-bit and 64-bit
# code in userspace is considered a biarch arch.
%define biarcharches %{ix86} x86_64 %{power64} s390 s390x
##############################################################################
# If the debug information is split into two packages, the core debuginfo
# pacakge and the common debuginfo package then the arch should be listed
# here. If the arch is not listed here then a single core debuginfo package
# will be created for the architecture.
%define debuginfocommonarches %{biarcharches} alpha alphaev6
##############################################################################
# %%package glibc - The GNU C Library (glibc) core package.
##############################################################################
Summary: The GNU libc libraries
Name: glibc
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

##############################################################################
# Continued list of core "glibc" package information:
##############################################################################
Obsoletes: glibc-profile < 2.4
Provides: ldconfig

# The dynamic linker supports DT_GNU_HASH
Provides: rtld(GNU_HASH)
Requires: glibc-common = %{version}-%{release}

# Various components (regex, glob) have been imported from gnulib.
Provides: bundled(gnulib)

Requires(pre): basesystem

# This is for building auxiliary programs like memusage, nscd
# For initial glibc bootstraps it can be commented out
%if %{without bootstrap}
BuildRequires: gd-devel libpng-devel zlib-devel
%endif
%if %{with docs}
# Removing texinfo will cause check-safety.sh test to fail because it seems to
# trigger documentation generation based on dependencies.  We need to fix this
# upstream in some way that doesn't depend on generating docs to validate the
# texinfo.  I expect it's simply the wrong dependency for that target.
BuildRequires: texinfo >= 5.0
%endif
%if %{without bootstrap}
BuildRequires: libselinux-devel >= 1.33.4-3
%endif
BuildRequires: audit-libs-devel >= 1.1.3, sed >= 3.95, libcap-devel, gettext
# We need procps-ng (/bin/ps), util-linux (/bin/kill), and gawk (/bin/awk),
# but it is more flexible to require the actual programs and let rpm infer
# the packages. However, until bug 1259054 is widely fixed we avoid the
# following:
# BuildRequires: /bin/ps, /bin/kill, /bin/awk
# And use instead (which should be reverted some time in the future):
BuildRequires: procps-ng, util-linux, gawk
BuildRequires: systemtap-sdt-devel

%if %{with valgrind}
# Require valgrind for smoke testing the dynamic loader to make sure we
# have not broken valgrind.
BuildRequires: valgrind
%endif

# We use systemd rpm macros for nscd
BuildRequires: systemd

# We use python for the microbenchmarks and locale data regeneration
# from unicode sources (carried out manually). We choose python3
# explicitly because it supports both use cases.  On some
# distributions, python3 does not actually install /usr/bin/python3,
# so we also depend on python3-devel.
BuildRequires: python3 python3-devel

# This is the first GCC version with enhanced valgrind support in the
# inline expansion of string functions (#1532205, #1652929, #1652932).
BuildRequires: gcc >= 8.2.1-3.4
%define enablekernel 3.2
Conflicts: kernel < %{enablekernel}
%define target %{_target_cpu}-redhat-linux
%ifarch %{arm}
%define target %{_target_cpu}-redhat-linuxeabi
%endif
%ifarch %{power64}
%ifarch ppc64le
%define target ppc64le-redhat-linux
%else
%define target ppc64-redhat-linux
%endif
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

%if %{without bootstrap}
%if %{with testsuite}
# The testsuite builds static C++ binaries that require a C++ compiler,
# static C++ runtime from libstdc++-static, and lastly static glibc.
BuildRequires: gcc-c++
BuildRequires: libstdc++-static
# A configure check tests for the ability to create static C++ binaries
# before glibc is built and therefore we need a glibc-static for that
# check to pass even if we aren't going to use any of those objects to
# build the tests.
BuildRequires: glibc-static

# libidn2 (but not libidn2-devel) is needed for testing AI_IDN/NI_IDN.
BuildRequires: libidn2
%endif
%endif

# Filter out all GLIBC_PRIVATE symbols since they are internal to
# the package and should not be examined by any other tool.
%global __filter_GLIBC_PRIVATE 1

# For language packs we have glibc require a virtual dependency
# "glibc-langpack" wich gives us at least one installed langpack.
# If no langpack providing 'glibc-langpack' was installed you'd
# get all of them, and that would make the transition from a
# system without langpacks smoother (you'd get all the locales
# installed). You would then trim that list, and the trimmed list
# is preserved. One problem is you can't have "no" locales installed,
# in that case we offer a "glibc-minimal-langpack" sub-pakcage for
# this purpose.
Requires: glibc-langpack = %{version}-%{release}
Suggests: glibc-all-langpacks = %{version}-%{release}

%description
The glibc package contains standard libraries which are used by
multiple programs on the system. In order to save disk space and
memory, as well as to make upgrading easier, common system code is
kept in one place and shared between programs. This particular package
contains the most important sets of shared libraries: the standard C
library and the standard math library. Without these two libraries, a
Linux system will not function.

######################################################################
# libnsl subpackage
######################################################################

%package -n libnsl
Summary: Legacy support library for NIS
Requires: %{name}%{_isa} = %{version}-%{release}

%description -n libnsl
This package provides the legacy version of libnsl library, for
accessing NIS services.

This library is provided for backwards compatibility only;
applications should use libnsl2 instead to gain IPv6 support.

##############################################################################
# glibc "devel" sub-package
##############################################################################
%package devel
Summary: Object files for development using standard C libraries.
Requires(pre): /sbin/install-info
Requires(pre): %{name}-headers
Requires: %{name}-headers = %{version}-%{release}
Requires: %{name} = %{version}-%{release}
Requires: libgcc%{_isa}
Requires: libxcrypt-devel%{_isa} >= 4.0.0

%description devel
The glibc-devel package contains the object files necessary
for developing programs which use the standard C libraries (which are
used by nearly all programs).  If you are developing programs which
will use the standard C libraries, your system needs to have these
standard object files available in order to create the
executables.

Install glibc-devel if you are going to develop programs which will
use the standard C libraries.

##############################################################################
# glibc "static" sub-package
##############################################################################
%package static
Summary: C library static libraries for -static linking.
Requires: %{name}-devel = %{version}-%{release}
Requires: libxcrypt-static%{?_isa} >= 4.0.0

%description static
The glibc-static package contains the C library static libraries
for -static linking.  You don't need these, unless you link statically,
which is highly discouraged.

##############################################################################
# glibc "headers" sub-package
# - The headers package includes all common headers that are shared amongst
#   the multilib builds. It was created to reduce the download size, and
#   thus avoid downloading one header package per multilib. The package is
#   identical both in content and file list, any difference is an error.
#   Files like gnu/stubs.h which have gnu/stubs-32.h (i686) and gnu/stubs-64.h
#   are included in glibc-headers, but the -32 and -64 files are in their
#   respective i686 and x86_64 devel packages.
##############################################################################
%package headers
Summary: Header files for development using standard C libraries.
Provides: %{name}-headers(%{_target_cpu})
Requires(pre): kernel-headers
Requires: kernel-headers >= 2.2.1, %{name} = %{version}-%{release}
BuildRequires: kernel-headers >= 3.2

%description headers
The glibc-headers package contains the header files necessary
for developing programs which use the standard C libraries (which are
used by nearly all programs).  If you are developing programs which
will use the standard C libraries, your system needs to have these
standard header files available in order to create the
executables.

Install glibc-headers if you are going to develop programs which will
use the standard C libraries.

##############################################################################
# glibc "common" sub-package
##############################################################################
%package common
Summary: Common binaries and locale data for glibc
Requires: %{name} = %{version}-%{release}
Requires: tzdata >= 2003a

%description common
The glibc-common package includes common binaries for the GNU libc
libraries, as well as national language (locale) support.

######################################################################
# File triggers to do ldconfig calls automatically (see rhbz#1380878)
######################################################################

# File triggers for when libraries are added or removed in standard
# paths.
%transfiletriggerin common -P 2000000 -- /lib /usr/lib /lib64 /usr/lib64
/sbin/ldconfig
%end

%transfiletriggerpostun common -P 2000000 -- /lib /usr/lib /lib64 /usr/lib64
/sbin/ldconfig
%end

# We need to run ldconfig manually because __brp_ldconfig assumes that
# glibc itself is always installed in $RPM_BUILD_ROOT, but with sysroots
# we may be installed into a subdirectory of that path.  Therefore we
# unset __brp_ldconfig and run ldconfig by hand with the sysroots path
# passed to -r.
%undefine __brp_ldconfig

######################################################################

%package locale-source
Summary: The sources for the locales
Requires: %{name} = %{version}-%{release}
Requires: %{name}-common = %{version}-%{release}

%description locale-source
The sources for all locales provided in the language packs.
If you are building custom locales you will most likely use
these sources as the basis for your new locale.

%{lua:
-- Array of languages (ISO-639 codes).
local languages = {}
-- Dictionary from language codes (as in the languages array) to arrays
-- of regions.
local supplements = {}
do
   -- Parse the SUPPORTED file.  Eliminate duplicates.
   local lang_region_seen = {}
   for line in io.lines(rpm.expand("%{SOURCE11}")) do
      -- Match lines which contain a language (eo) or language/region
      -- (en_US) strings.
      local lang_region = string.match(line, "^([a-z][^/@.]+)")
      if lang_region ~= nil then
	 if lang_region_seen[lang_region] == nil then
	    lang_region_seen[lang_region] = true

	    -- Split language/region pair.
	    local lang, region = string.match(lang_region, "^(.+)_(.+)")
	    if lang == nil then
	       -- Region is missing, use only the language.
	       lang = lang_region
	    end
	    local suppl = supplements[lang]
	    if suppl == nil then
	       suppl = {}
	       supplements[lang] = suppl
	       -- New language not seen before.
	       languages[#languages + 1] = lang
	    end
	    if region ~= nil then
	       -- New region because of the check against
	       -- lang_region_seen above.
	       suppl[#suppl + 1] = region
	    end
	 end
      end
   end
   -- Sort for determinism.
   table.sort(languages)
   for _, supples in pairs(supplements) do
      table.sort(supplements)
   end
end

-- Compute the Supplements: list for a language, based on the regions.
local function compute_supplements(lang)
   result = "langpacks-" .. lang
   regions = supplements[lang]
   if regions ~= nil then
      for i = 1, #regions do
	 result = result .. " or langpacks-" .. lang .. "_" .. regions[i]
      end
   end
   return result
end

-- Emit the definition of a language pack package.
local function lang_package(lang)
   local suppl = compute_supplements(lang)
   print(rpm.expand([[

%package langpack-]]..lang..[[

Summary: Locale data for ]]..lang..[[

Provides: glibc-langpack = %{version}-%{release}
Requires: %{name} = %{version}-%{release}
Requires: %{name}-common = %{version}-%{release}
Supplements: (glibc and (]]..suppl..[[))
%description langpack-]]..lang..[[

The glibc-langpack-]]..lang..[[ package includes the basic information required
to support the ]]..lang..[[ language in your applications.
%ifnarch %{auxarches}
%files -f langpack-]]..lang..[[.filelist langpack-]]..lang..[[

%endif
]]))
end

for i = 1, #languages do
   lang_package(languages[i])
end
}

# The glibc-all-langpacks provides the virtual glibc-langpack,
# and thus satisfies glibc's requirement for installed locales.
# Users can add one more other langauge packs and then eventually
# uninstall all-langpacks to save space.
%package all-langpacks
Summary: All language packs for %{name}.
Requires: %{name} = %{version}-%{release}
Requires: %{name}-common = %{version}-%{release}
Provides: %{name}-langpack = %{version}-%{release}
%description all-langpacks

# No %files, this is an empty pacakge. The C/POSIX and
# C.UTF-8 files are already installed by glibc. We create
# minimal-langpack because the virtual provide of
# glibc-langpack needs at least one package installed
# to satisfy it. Given that no-locales installed is a valid
# use case we support it here with this package.
%package minimal-langpack
Summary: Minimal language packs for %{name}.
Provides: glibc-langpack = %{version}-%{release}
Requires: %{name} = %{version}-%{release}
Requires: %{name}-common = %{version}-%{release}
%description minimal-langpack
This is a Meta package that is used to install minimal language packs.
This package ensures you can use C, POSIX, or C.UTF-8 locales, but
nothing else. It is designed for assembling a minimal system.
%ifnarch %{auxarches}
%files minimal-langpack
%endif

##############################################################################
# glibc "nscd" sub-package
##############################################################################
%package -n nscd
Summary: A Name Service Caching Daemon (nscd).
Requires: %{name} = %{version}-%{release}
%if %{without bootstrap}
Requires: libselinux >= 1.17.10-1
%endif
Requires: audit-libs >= 1.1.3
Requires(pre): /usr/sbin/useradd, coreutils
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd, /usr/sbin/userdel

%description -n nscd
The nscd daemon caches name service lookups and can improve
performance with LDAP, and may help with DNS as well.

##############################################################################
# Subpackages for NSS modules except nss_files, nss_compat, nss_dns
##############################################################################

# This should remain it's own subpackage or "Provides: nss_db" to allow easy
# migration from old systems that previously had the old nss_db package
# installed. Note that this doesn't make the migration that smooth, the
# databases still need rebuilding because the formats were different.
# The nss_db package was deprecated in F16 and onwards:
# https://lists.fedoraproject.org/pipermail/devel/2011-July/153665.html
# The different database format does cause some issues for users:
# https://lists.fedoraproject.org/pipermail/devel/2011-December/160497.html
%package -n nss_db
Summary: Name Service Switch (NSS) module using hash-indexed files
Requires: %{name}%{_isa} = %{version}-%{release}

%description -n nss_db
The nss_db Name Service Switch module uses hash-indexed files in /var/db
to speed up user, group, service, host name, and other NSS-based lookups.

%package -n nss_hesiod
Summary: Name Service Switch (NSS) module using Hesiod
Requires: %{name}%{_isa} = %{version}-%{release}

%description -n nss_hesiod
The nss_hesiod Name Service Switch module uses the Domain Name System
(DNS) as a source for user, group, and service information, following
the Hesiod convention of Project Athena.

%package nss-devel
Summary: Development files for directly linking NSS service modules
Requires: %{name}%{_isa} = %{version}-%{release}
Requires: nss_db%{_isa} = %{version}-%{release}
Requires: nss_hesiod%{_isa} = %{version}-%{release}

%description nss-devel
The glibc-nss-devel package contains the object files necessary to
compile applications and libraries which directly link against NSS
modules supplied by glibc.

This is a rare and special use case; regular development has to use
the glibc-devel package instead.

##############################################################################
# glibc "utils" sub-package
##############################################################################
%package utils
Summary: Development utilities from GNU C library
Requires: %{name} = %{version}-%{release}

%description utils
The glibc-utils package contains memusage, a memory usage profiler,
mtrace, a memory leak tracer and xtrace, a function call tracer
which can be helpful during program debugging.

If unsure if you need this, don't install this package.

##############################################################################
# glibc core "debuginfo" sub-package
##############################################################################
%if 0%{?_enable_debug_packages}
%define debug_package %{nil}
%define __debug_install_post %{nil}
%global __debug_package 1
# Disable thew new features that glibc packages don't use.
%undefine _debugsource_packages
%undefine _debuginfo_subpackages
%undefine _unique_debug_names
%undefine _unique_debug_srcs

%package debuginfo
Summary: Debug information for package %{name}
AutoReqProv: no
%ifarch %{debuginfocommonarches}
Requires: glibc-debuginfo-common = %{version}-%{release}
%else
%ifarch %{ix86} %{sparc}
Obsoletes: glibc-debuginfo-common
%endif
%endif

%description debuginfo
This package provides debug information for package %{name}.
Debug information is useful when developing applications that use this
package or when debugging this package.

This package also contains static standard C libraries with
debugging information.  You need this only if you want to step into
C library routines during debugging programs statically linked against
one or more of the standard C libraries.
To use this debugging information, you need to link binaries
with -static -L%{_prefix}/lib/debug%{_libdir} compiler options.

##############################################################################
# glibc common "debuginfo-common" sub-package
##############################################################################
%ifarch %{debuginfocommonarches}

%package debuginfo-common
Summary: Debug information for package %{name}
AutoReqProv: no

%description debuginfo-common
This package provides debug information for package %{name}.
Debug information is useful when developing applications that use this
package or when debugging this package.

%endif # %{debuginfocommonarches}
%endif # 0%{?_enable_debug_packages}

%if %{with benchtests}
%package benchtests
Summary: Benchmarking binaries and scripts for %{name}
%description benchtests
This package provides built benchmark binaries and scripts to run
microbenchmark tests on the system.
%endif

##############################################################################
# compat-libpthread-nonshared
# See: https://sourceware.org/bugzilla/show_bug.cgi?id=23500
##############################################################################
%package -n compat-libpthread-nonshared
Summary: Compatibility support for linking against libpthread_nonshared.a.

%description -n compat-libpthread-nonshared
This package provides compatibility support for applications that expect
libpthread_nonshared.a to exist. The support provided is in the form of
an empty libpthread_nonshared.a that allows dynamic links to succeed.
Such applications should be adjusted to avoid linking against
libpthread_nonshared.a which is no longer used. The static library
libpthread_nonshared.a is an internal implementation detail of the C
runtime and should not be expected to exist.

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

# Verify that our copy of localedata/SUPPORTED matches the glibc
# version.
#
# The separate file copy is used by the Lua parser above.
# Patches or new upstream versions may change the list of locales,
# which changes the set of langpacks we need to build.  Verify the
# differences then update the copy of SUPPORTED.  This approach has
# two purposes: (a) avoid spurious changes to the set of langpacks,
# and (b) the Lua snippet can use a fully patched-up version
# of the localedata/SUPPORTED file.
diff -u %{SOURCE11} localedata/SUPPORTED

##############################################################################
# Build glibc...
##############################################################################
%build
# Log system information
uname -a
LD_SHOW_AUXV=1 /bin/true
cat /proc/cpuinfo
cat /proc/sysinfo 2>/dev/null || true
cat /proc/meminfo
df

# We build using the native system compilers.
GCC=gcc
GXX=g++

# Part of rpm_inherit_flags.  Is overridden below.
rpm_append_flag ()
{
    BuildFlags="$BuildFlags $*"
}

# Propagates the listed flags to rpm_append_flag if supplied by
# redhat-rpm-config.
BuildFlags="-O2 -g"
rpm_inherit_flags ()
{
	local reference=" $* "
	local flag
	for flag in $RPM_OPT_FLAGS $RPM_LD_FLAGS ; do
		if echo "$reference" | grep -q -F " $flag " ; then
			rpm_append_flag "$flag"
		fi
	done
}

# Propgate select compiler flags from redhat-rpm-config.  These flags
# are target-dependent, so we use only those which are specified in
# redhat-rpm-config.  We keep the -m32/-m32/-m64 flags to support
# multilib builds.
#
# Note: For building alternative run-times, care is required to avoid
# overriding the architecture flags which go into CC/CXX.  The flags
# below are passed in CFLAGS.

rpm_inherit_flags \
	"-Wp,-D_GLIBCXX_ASSERTIONS" \
	"-fasynchronous-unwind-tables" \
	"-fstack-clash-protection" \
	"-funwind-tables" \
	"-m31" \
	"-m32" \
	"-m64" \
	"-march=i686" \
	"-march=x86-64" \
	"-march=z13" \
	"-march=z14" \
	"-march=zEC12" \
	"-mfpmath=sse" \
	"-msse2" \
	"-mstackrealign" \
	"-mtune=generic" \
	"-mtune=z13" \
	"-mtune=z14" \
	"-mtune=zEC12" \
	"-specs=/usr/lib/rpm/redhat/redhat-annobin-cc1" \

# Propagate additional build flags to BuildFlagsNonshared.  This is
# very special because some of these files are part of the startup
# code.  We essentially hope that these flags have little effect
# there, and only specify the, for consistency, so that annobin
# records the expected compiler flags.
BuildFlagsNonshared=
rpm_append_flag () {
    BuildFlagsNonshared="$BuildFlagsNonshared $*"
}
rpm_inherit_flags \
	"-Wp,-D_FORTIFY_SOURCE=2" \

# Special flag to enable annobin annotations for statically linked
# assembler code.  Needs to be passed to make; not preserved by
# configure.
%define glibc_make_flags_as ASFLAGS="-g -Wa,--generate-missing-build-notes=yes"
%define glibc_make_flags %{glibc_make_flags_as}

##############################################################################
# %%build - Generic options.
##############################################################################
EnableKernel="--enable-kernel=%{enablekernel}"
# Save the used compiler and options into the file "Gcc" for use later
# by %%install.
echo "$GCC" > Gcc

##############################################################################
# build()
#	Build glibc in `build-%{target}$1', passing the rest of the arguments
#	as CFLAGS to the build (not the same as configure CFLAGS). Several
#	global values are used to determine build flags, kernel version,
#	system tap support, etc.
##############################################################################
build()
{
	local builddir=build-%{target}${1:+-$1}
	${1+shift}
	rm -rf $builddir
	mkdir $builddir
	pushd $builddir
	../configure CC="$GCC" CXX="$GXX" CFLAGS="$BuildFlags $*" \
		--prefix=%{_prefix} \
		--with-headers=%{_prefix}/include $EnableKernel \
		--with-nonshared-cflags="$BuildFlagsNonshared" \
		--enable-bind-now \
		--build=%{target} \
		--enable-stack-protector=strong \
%ifarch %{pie_arches}
		--enable-static-pie \
%endif
		--enable-tunables \
		--enable-systemtap \
		${core_with_options} \
%ifarch x86_64 %{ix86}
	       --enable-cet \
%endif
%ifarch %{ix86}
		--disable-multi-arch \
%endif
%if %{without werror}
		--disable-werror \
%endif
		--disable-profile \
%if %{with bootstrap}
		--without-selinux \
%endif
		--disable-crypt ||
		{ cat config.log; false; }

	make %{?_smp_mflags} -O -r %{glibc_make_flags}
	popd
}

# Default set of compiler options.
build

%if %{buildpower9}
(
  GCC="$GCC -mcpu=power9 -mtune=power9"
  GXX="$GXX -mcpu=power9 -mtune=power9"
  core_with_options="--with-cpu=power9"
  build power9
)
%endif

##############################################################################
# Install glibc...
##############################################################################
%install

# The built glibc is installed into a subdirectory of $RPM_BUILD_ROOT.
# For a system glibc that subdirectory is "/" (the root of the filesystem).
# This is called a sysroot (system root) and can be changed if we have a
# distribution that supports multiple installed glibc versions.
%define glibc_sysroot $RPM_BUILD_ROOT

# Remove existing file lists.
find . -type f -name '*.filelist' -exec rm -rf {} \;

# Ensure the permissions of errlist.c do not change.  When the file is
# regenerated the Makefile sets the permissions to 444. We set it to 644
# to match what comes out of git. The tarball of the git archive won't have
# correct permissions because git doesn't track all of the permissions
# accurately (see git-cache-meta if you need that). We also set it to 644 to
# match pre-existing rpms. We do this *after* the build because the build
# might regenerate the file and set the permissions to 444.
chmod 644 sysdeps/gnu/errlist.c

# Reload compiler and build options that were used during %%build.
GCC=`cat Gcc`

%ifarch riscv64
# RISC-V ABI wants to install everything in /lib64/lp64d or /usr/lib64/lp64d.
# Make these be symlinks to /lib64 or /usr/lib64 respectively.  See:
# https://lists.fedoraproject.org/archives/list/devel@lists.fedoraproject.org/thread/DRHT5YTPK4WWVGL3GIN5BF2IKX2ODHZ3/
for d in %{glibc_sysroot}%{_libdir} %{glibc_sysroot}/%{_lib}; do
	mkdir -p $d
	(cd $d && ln -sf . lp64d)
done
%endif

# Build and install:
make -j1 install_root=%{glibc_sysroot} install -C build-%{target}

# If we are not building an auxiliary arch then install all of the supported
# locales.
%ifnarch %{auxarches}
pushd build-%{target}
# Do not use a parallel make here because the hardlink optimization in
# localedef is not fully reproducible when running concurrently.
make install_root=%{glibc_sysroot} \
	install-locales -C ../localedata objdir=`pwd`
popd
%endif

# install_different:
#	Install all core libraries into DESTDIR/SUBDIR. Either the file is
#	installed as a copy or a symlink to the default install (if it is the
#	same). The path SUBDIR_UP is the prefix used to go from
#	DESTDIR/SUBDIR to the default installed libraries e.g.
#	ln -s SUBDIR_UP/foo.so DESTDIR/SUBDIR/foo.so.
#	When you call this function it is expected that you are in the root
#	of the build directory, and that the default build directory is:
#	"../build-%{target}" (relatively).
#	The primary use of this function is to install alternate runtimes
#	into the build directory and avoid duplicating this code for each
#	runtime.
install_different()
{
	local lib libbase libbaseso dlib
	local destdir="$1"
	local subdir="$2"
	local subdir_up="$3"
	local libdestdir="$destdir/$subdir"
	# All three arguments must be non-zero paths.
	if ! [ "$destdir" \
	       -a "$subdir" \
	       -a "$subdir_up" ]; then
		echo "One of the arguments to install_different was emtpy."
		exit 1
	fi
	# Create the destination directory and the multilib directory.
	mkdir -p "$destdir"
	mkdir -p "$libdestdir"
	# Walk all of the libraries we installed...
	for lib in libc math/libm nptl/libpthread rt/librt nptl_db/libthread_db
	do
		libbase=${lib#*/}
		# Take care that `libbaseso' has a * that needs expanding so
		# take care with quoting.
		libbaseso=$(basename %{glibc_sysroot}/%{_lib}/${libbase}-*.so)
		# Only install if different from default build library.
		if cmp -s ${lib}.so ../build-%{target}/${lib}.so; then
			ln -sf "$subdir_up"/$libbaseso $libdestdir/$libbaseso
		else
			cp -a ${lib}.so $libdestdir/$libbaseso
		fi
		dlib=$libdestdir/$(basename %{glibc_sysroot}/%{_lib}/${libbase}.so.*)
		ln -sf $libbaseso $dlib
	done
}

%if %{buildpower9}
pushd build-%{target}-power9
install_different "$RPM_BUILD_ROOT/%{_lib}" power9 ..
popd
%endif

##############################################################################
# Remove the files we don't want to distribute
##############################################################################

# Remove the libNoVersion files.
# XXX: This looks like a bug in glibc that accidentally installed these
#      wrong files. We probably don't need this today.
rm -f %{glibc_sysroot}/%{_libdir}/libNoVersion*
rm -f %{glibc_sysroot}/%{_lib}/libNoVersion*

# Remove the old nss modules.
rm -f %{glibc_sysroot}/%{_lib}/libnss1-*
rm -f %{glibc_sysroot}/%{_lib}/libnss-*.so.1

# This statically linked binary is no longer necessary in a world where
# the default Fedora install uses an initramfs, and further we have rpm-ostree
# which captures the whole userspace FS tree.
# Further, see https://github.com/projectatomic/rpm-ostree/pull/1173#issuecomment-355014583
rm -f %{glibc_sysroot}/{usr/,}sbin/sln

######################################################################
# Run ldconfig to create all the symbolic links we need
######################################################################

# Note: This has to happen before creating /etc/ld.so.conf.

mkdir -p %{glibc_sysroot}/var/cache/ldconfig
truncate -s 0 %{glibc_sysroot}/var/cache/ldconfig/aux-cache

# ldconfig is statically linked, so we can use the new version.
%{glibc_sysroot}/sbin/ldconfig -N -r %{glibc_sysroot}

##############################################################################
# Install info files
##############################################################################

%if %{with docs}
# Move the info files if glibc installed them into the wrong location.
if [ -d %{glibc_sysroot}%{_prefix}/info -a "%{_infodir}" != "%{_prefix}/info" ]; then
  mkdir -p %{glibc_sysroot}%{_infodir}
  mv -f %{glibc_sysroot}%{_prefix}/info/* %{glibc_sysroot}%{_infodir}
  rm -rf %{glibc_sysroot}%{_prefix}/info
fi

# Compress all of the info files.
gzip -9nvf %{glibc_sysroot}%{_infodir}/libc*

%else
rm -f %{glibc_sysroot}%{_infodir}/dir
rm -f %{glibc_sysroot}%{_infodir}/libc.info*
%endif

##############################################################################
# Create locale sub-package file lists
##############################################################################

%ifnarch %{auxarches}
olddir=`pwd`
pushd %{glibc_sysroot}%{_prefix}/lib/locale
rm -f locale-archive
# Intentionally we do not pass --alias-file=, aliases will be added
# by build-locale-archive.
$olddir/build-%{target}/elf/ld.so \
        --library-path $olddir/build-%{target}/ \
        $olddir/build-%{target}/locale/localedef \
        --prefix %{glibc_sysroot} --add-to-archive \
        eo *_*
# Setup the locale-archive template for use by glibc-all-langpacks.  We
# copy the archive in place to keep the size of the file. Even though we
# mark the file with "ghost" the size is used by rpm to compute the
# required free space (see rhbz#1725131). We do this because there is a
# point in the install when build-locale-archive has copied 100% of the
# template into the new locale archive and so this consumes twice the
# amount of diskspace. Note that this doesn't account for copying
# existing compiled locales into the archive, this may consume even more
# disk space and we can't fix that issue. In upstream we have moved away
# from this process, removing build-locale-archive and installing a
# default locale-archive without modification, and leaving compiled
# locales as they are (without inclusion into the archive).
cp locale-archive{,.tmpl}
# Create the file lists for the language specific sub-packages:
for i in eo *_*
do
    lang=${i%%_*}
    if [ ! -e langpack-${lang}.filelist ]; then
        echo "%dir %{_prefix}/lib/locale" >> langpack-${lang}.filelist
    fi
    echo "%dir  %{_prefix}/lib/locale/$i" >> langpack-${lang}.filelist
    echo "%{_prefix}/lib/locale/$i/*" >> langpack-${lang}.filelist
done
popd
pushd %{glibc_sysroot}%{_prefix}/share/locale
for i in */LC_MESSAGES/libc.mo
do
    locale=${i%%%%/*}
    lang=${locale%%%%_*}
    echo "%lang($lang) %{_prefix}/share/locale/${i}" \
         >> %{glibc_sysroot}%{_prefix}/lib/locale/langpack-${lang}.filelist
done
popd
mv  %{glibc_sysroot}%{_prefix}/lib/locale/*.filelist .
%endif

##############################################################################
# Install configuration files for services
##############################################################################

install -p -m 644 %{SOURCE7} %{glibc_sysroot}/etc/nsswitch.conf

%ifnarch %{auxarches}
# This is for ncsd - in glibc 2.2
install -m 644 nscd/nscd.conf %{glibc_sysroot}/etc
mkdir -p %{glibc_sysroot}%{_tmpfilesdir}
install -m 644 %{SOURCE4} %{buildroot}%{_tmpfilesdir}
mkdir -p %{glibc_sysroot}/lib/systemd/system
install -m 644 nscd/nscd.service nscd/nscd.socket %{glibc_sysroot}/lib/systemd/system
%endif

# Include ld.so.conf
echo 'include ld.so.conf.d/*.conf' > %{glibc_sysroot}/etc/ld.so.conf
truncate -s 0 %{glibc_sysroot}/etc/ld.so.cache
chmod 644 %{glibc_sysroot}/etc/ld.so.conf
mkdir -p %{glibc_sysroot}/etc/ld.so.conf.d
%ifnarch %{auxarches}
mkdir -p %{glibc_sysroot}/etc/sysconfig
truncate -s 0 %{glibc_sysroot}/etc/sysconfig/nscd
truncate -s 0 %{glibc_sysroot}/etc/gai.conf
%endif

# Include %{_libdir}/gconv/gconv-modules.cache
truncate -s 0 %{glibc_sysroot}%{_libdir}/gconv/gconv-modules.cache
chmod 644 %{glibc_sysroot}%{_libdir}/gconv/gconv-modules.cache

##############################################################################
# Install debug copies of unstripped static libraries
# - This step must be last in order to capture any additional static
#   archives we might have added.
##############################################################################

# If we are building a debug package then copy all of the static archives
# into the debug directory to keep them as unstripped copies.
%if 0%{?_enable_debug_packages}
mkdir -p %{glibc_sysroot}%{_prefix}/lib/debug%{_libdir}
cp -a %{glibc_sysroot}%{_libdir}/*.a \
	%{glibc_sysroot}%{_prefix}/lib/debug%{_libdir}/
rm -f %{glibc_sysroot}%{_prefix}/lib/debug%{_libdir}/*_p.a
%endif

# Remove any zoneinfo files; they are maintained by tzdata.
rm -rf %{glibc_sysroot}%{_prefix}/share/zoneinfo

# Make sure %config files have the same timestamp across multilib packages.
#
# XXX: Ideally ld.so.conf should have the timestamp of the spec file, but there
# doesn't seem to be any macro to give us that.  So we do the next best thing,
# which is to at least keep the timestamp consistent. The choice of using
# SOURCE0 is arbitrary.
touch -r %{SOURCE0} %{glibc_sysroot}/etc/ld.so.conf
touch -r sunrpc/etc.rpc %{glibc_sysroot}/etc/rpc

pushd build-%{target}
$GCC -Os -g -static -o build-locale-archive %{SOURCE1} \
	../build-%{target}/locale/locarchive.o \
	../build-%{target}/locale/md5.o \
	../build-%{target}/locale/record-status.o \
	-I. -DDATADIR=\"%{_datadir}\" -DPREFIX=\"%{_prefix}\" \
	-L../build-%{target} \
	-B../build-%{target}/csu/ -lc -lc_nonshared
install -m 700 build-locale-archive %{glibc_sysroot}%{_prefix}/sbin/build-locale-archive
popd

# Lastly copy some additional documentation for the packages.
rm -rf documentation
mkdir documentation
cp timezone/README documentation/README.timezone
cp posix/gai.conf documentation/

%ifarch s390x
# Compatibility symlink
mkdir -p %{glibc_sysroot}/lib
ln -sf /%{_lib}/ld64.so.1 %{glibc_sysroot}/lib/ld64.so.1
%endif

%if %{with benchtests}
# Build benchmark binaries.  Ignore the output of the benchmark runs.
pushd build-%{target}
make BENCH_DURATION=1 bench-build
popd

# Copy over benchmark binaries.
mkdir -p %{glibc_sysroot}%{_prefix}/libexec/glibc-benchtests
cp $(find build-%{target}/benchtests -type f -executable) %{glibc_sysroot}%{_prefix}/libexec/glibc-benchtests/
# ... and the makefile.
for b in %{SOURCE9} %{SOURCE10}; do
	cp $b %{glibc_sysroot}%{_prefix}/libexec/glibc-benchtests/
done
# .. and finally, the comparison scripts.
cp benchtests/scripts/benchout.schema.json %{glibc_sysroot}%{_prefix}/libexec/glibc-benchtests/
cp benchtests/scripts/compare_bench.py %{glibc_sysroot}%{_prefix}/libexec/glibc-benchtests/
cp benchtests/scripts/import_bench.py %{glibc_sysroot}%{_prefix}/libexec/glibc-benchtests/
cp benchtests/scripts/validate_benchout.py %{glibc_sysroot}%{_prefix}/libexec/glibc-benchtests/

%if 0%{?_enable_debug_packages}
# The #line directives gperf generates do not give the proper
# file name relative to the build directory.
pushd locale
ln -s programs/*.gperf .
popd
pushd iconv
ln -s ../locale/programs/charmap-kw.gperf .
popd

%if %{with docs}
# Remove the `dir' info-heirarchy file which will be maintained
# by the system as it adds info files to the install.
rm -f %{glibc_sysroot}%{_infodir}/dir
%endif

%ifnarch %{auxarches}
mkdir -p %{glibc_sysroot}/var/{db,run}/nscd
touch %{glibc_sysroot}/var/{db,run}/nscd/{passwd,group,hosts,services}
touch %{glibc_sysroot}/var/run/nscd/{socket,nscd.pid}
%endif

# Move libpcprofile.so and libmemusage.so into the proper library directory.
# They can be moved without any real consequences because users would not use
# them directly.
mkdir -p %{glibc_sysroot}%{_libdir}
mv -f %{glibc_sysroot}/%{_lib}/lib{pcprofile,memusage}.so \
	%{glibc_sysroot}%{_libdir}

# Strip all of the installed object files.
strip -g %{glibc_sysroot}%{_libdir}/*.o

###############################################################################
# Rebuild libpthread.a using --whole-archive to ensure all of libpthread
# is included in a static link. This prevents any problems when linking
# statically, using parts of libpthread, and other necessary parts not
# being included. Upstream has decided that this is the wrong approach to
# this problem and that the full set of dependencies should be resolved
# such that static linking works and produces the most minimally sized
# static application possible.
###############################################################################
pushd %{glibc_sysroot}%{_prefix}/%{_lib}/
$GCC -r -nostdlib -o libpthread.o -Wl,--whole-archive ./libpthread.a
rm libpthread.a
ar rcs libpthread.a libpthread.o
rm libpthread.o
popd

# The xtrace and memusage scripts have hard-coded paths that need to be
# translated to a correct set of paths using the $LIB token which is
# dynamically translated by ld.so as the default lib directory.
for i in %{glibc_sysroot}%{_prefix}/bin/{xtrace,memusage}; do
%if %{with bootstrap}
  test -w $i || continue
%endif
  sed -e 's~=/%{_lib}/libpcprofile.so~=%{_libdir}/libpcprofile.so~' \
      -e 's~=/%{_lib}/libmemusage.so~=%{_libdir}/libmemusage.so~' \
      -e 's~='\''/\\\$LIB/libpcprofile.so~='\''%{_prefix}/\\$LIB/libpcprofile.so~' \
      -e 's~='\''/\\\$LIB/libmemusage.so~='\''%{_prefix}/\\$LIB/libmemusage.so~' \
      -i $i
done

##############################################################################
# Build an empty libpthread_nonshared.a for compatiliby with applications
# that have old linker scripts that reference this file. We ship this only
# in compat-libpthread-nonshared sub-package.
##############################################################################
ar cr %{glibc_sysroot}%{_prefix}/%{_lib}/libpthread_nonshared.a

##############################################################################
# Beyond this point in the install process we no longer modify the set of
# installed files, with one exception, for auxarches we cleanup the file list
# at the end and remove files which we don't intend to ship. We need the file
# list to effect a proper cleanup, and so it happens last.
##############################################################################

##############################################################################
# Build the file lists used for describing the package and subpackages.
##############################################################################
# There are several main file lists (and many more for
# the langpack sub-packages (langpack-${lang}.filelist)):
# * master.filelist
#	- Master file list from which all other lists are built.
# * glibc.filelist
#	- Files for the glibc packages.
# * common.filelist
#	- Flies for the common subpackage.
# * utils.filelist
#	- Files for the utils subpackage.
# * nscd.filelist
#	- Files for the nscd subpackage.
# * devel.filelist
#	- Files for the devel subpackage.
# * headers.filelist
#	- Files for the headers subpackage.
# * static.filelist
#	- Files for the static subpackage.
# * libnsl.filelist
#       - Files for the libnsl subpackage
# * nss_db.filelist
# * nss_hesiod.filelist
#       - File lists for nss_* NSS module subpackages.
# * nss-devel.filelist
#       - File list with the .so symbolic links for NSS packages.
# * compat-libpthread-nonshared.filelist.
#	- File list for compat-libpthread-nonshared subpackage.
# * debuginfo.filelist
#	- Files for the glibc debuginfo package.
# * debuginfocommon.filelist
#	- Files for the glibc common debuginfo package.
#

# Create the main file lists. This way we can append to any one of them later
# wihtout having to create it. Note these are removed at the start of the
# install phase.
touch master.filelist
touch glibc.filelist
touch common.filelist
touch utils.filelist
touch nscd.filelist
touch devel.filelist
touch headers.filelist
touch static.filelist
touch libnsl.filelist
touch nss_db.filelist
touch nss_hesiod.filelist
touch nss-devel.filelist
touch compat-libpthread-nonshared.filelist
touch debuginfo.filelist
touch debuginfocommon.filelist

###############################################################################
# Master file list, excluding a few things.
###############################################################################
{
  # List all files or links that we have created during install.
  # Files with 'etc' are configuration files, likewise 'gconv-modules'
  # and 'gconv-modules.cache' are caches, and we exclude them.
  find %{glibc_sysroot} \( -type f -o -type l \) \
       \( \
	 -name etc -printf "%%%%config " -o \
	 -name gconv-modules \
	 -printf "%%%%verify(not md5 size mtime) %%%%config(noreplace) " -o \
	 -name gconv-modules.cache \
	 -printf "%%%%verify(not md5 size mtime) " \
	 , \
	 ! -path "*/lib/debug/*" -printf "/%%P\n" \)
  # List all directories with a %%dir prefix.  We omit the info directory and
  # all directories in (and including) /usr/share/locale.
  find %{glibc_sysroot} -type d \
       \( -path '*%{_prefix}/share/locale' -prune -o \
       \( -path '*%{_prefix}/share/*' \
%if %{with docs}
	! -path '*%{_infodir}' -o \
%endif
	  -path "*%{_prefix}/include/*" \
       \) -printf "%%%%dir /%%P\n" \)
} | {
  # Also remove the *.mo entries.  We will add them to the
  # language specific sub-packages.
  # libnss_ files go into subpackages related to NSS modules.
  # and .*/share/i18n/charmaps/.*), they go into the sub-package
  # "locale-source":
  sed -e '\,.*/share/locale/\([^/_]\+\).*/LC_MESSAGES/.*\.mo,d' \
      -e '\,.*/share/i18n/locales/.*,d' \
      -e '\,.*/share/i18n/charmaps/.*,d' \
      -e '\,.*/etc/\(localtime\|nsswitch.conf\|ld\.so\.conf\|ld\.so\.cache\|default\|rpc\|gai\.conf\),d' \
      -e '\,.*/%{_libdir}/lib\(pcprofile\|memusage\)\.so,d' \
      -e '\,.*/bin/\(memusage\|mtrace\|xtrace\|pcprofiledump\),d'
} | sort > master.filelist

# The master file list is now used by each subpackage to list their own
# files. We go through each package and subpackage now and create their lists.
# Each subpackage picks the files from the master list that they need.
# The order of the subpackage list generation does not matter.

# Make the master file list read-only after this point to avoid accidental
# modification.
chmod 0444 master.filelist

###############################################################################
# glibc
###############################################################################

# Add all files with the following exceptions:
# - The info files '%{_infodir}/dir'
# - The partial (lib*_p.a) static libraries, include files.
# - The static files, objects, unversioned DSOs, and nscd.
# - The bin, locale, some sbin, and share.
#   - We want iconvconfig in the main package and we do this by using
#     a double negation of -v and [^i] so it removes all files in
#     sbin *but* iconvconfig.
# - All the libnss files (we add back the ones we want later).
# - All bench test binaries.
# - The aux-cache, since it's handled specially in the files section.
# - The build-locale-archive binary since it's in the common package.
cat master.filelist \
	| grep -v \
	-e '%{_infodir}' \
	-e '%{_libdir}/lib.*_p.a' \
	-e '%{_prefix}/include' \
	-e '%{_libdir}/lib.*\.a' \
        -e '%{_libdir}/.*\.o' \
	-e '%{_libdir}/lib.*\.so' \
	-e 'nscd' \
	-e '%{_prefix}/bin' \
	-e '%{_prefix}/lib/locale' \
	-e '%{_prefix}/sbin/[^i]' \
	-e '%{_prefix}/share' \
	-e '/var/db/Makefile' \
	-e '/libnss_.*\.so[0-9.]*$' \
	-e '/libnsl' \
	-e 'glibc-benchtests' \
	-e 'aux-cache' \
	-e 'build-locale-archive' \
	> glibc.filelist

# Add specific files:
# - The nss_files, nss_compat, and nss_db files.
# - The libmemusage.so and libpcprofile.so used by utils.
for module in compat files dns; do
    cat master.filelist \
	| grep -E \
	-e "/libnss_$module(\.so\.[0-9.]+|-[0-9.]+\.so)$" \
	>> glibc.filelist
done
grep -e "libmemusage.so" -e "libpcprofile.so" master.filelist >> glibc.filelist

###############################################################################
# glibc-devel
###############################################################################

%if %{with docs}
# Put the info files into the devel file list, but exclude the generated dir.
grep '%{_infodir}' master.filelist | grep -v '%{_infodir}/dir' > devel.filelist
%endif

# Put some static files into the devel package.
grep '%{_libdir}/lib.*\.a' master.filelist \
  | grep '/lib\(\(c\|pthread\|nldbl\|mvec\)_nonshared\|g\|ieee\|mcheck\)\.a$' \
  >> devel.filelist

# Put all of the object files and *.so (not the versioned ones) into the
# devel package.
grep '%{_libdir}/.*\.o' < master.filelist >> devel.filelist
grep '%{_libdir}/lib.*\.so' < master.filelist >> devel.filelist
# The exceptions are:
# - libmemusage.so and libpcprofile.so in glibc used by utils.
# - libnss_*.so which are in nss-devel.
sed -i -e '\,libmemusage.so,d' \
	-e '\,libpcprofile.so,d' \
	-e '\,/libnss_[a-z]*\.so$,d' \
	devel.filelist

###############################################################################
# glibc-headers
###############################################################################

# The glibc-headers package includes only common files which are identical
# across all multilib packages. We must keep gnu/stubs.h and gnu/lib-names.h
# in the glibc-headers package, but the -32, -64, -64-v1, and -64-v2 versions
# go into the development packages.
grep '%{_prefix}/include/gnu/stubs-.*\.h$' < master.filelist >> devel.filelist || :
grep '%{_prefix}/include/gnu/lib-names-.*\.h$' < master.filelist >> devel.filelist || :
# Put the include files into headers file list.
grep '%{_prefix}/include' < master.filelist \
  | egrep -v '%{_prefix}/include/gnu/stubs-.*\.h$' \
  | egrep -v '%{_prefix}/include/gnu/lib-names-.*\.h$' \
  > headers.filelist

###############################################################################
# glibc-static
###############################################################################

# Put the rest of the static files into the static package.
grep '%{_libdir}/lib.*\.a' < master.filelist \
  | grep -v '/lib\(\(c\|pthread\|nldbl\|mvec\)_nonshared\|g\|ieee\|mcheck\)\.a$' \
  > static.filelist

###############################################################################
# glibc-common
###############################################################################

# All of the bin and certain sbin files go into the common package except
# iconvconfig which needs to go in glibc. Likewise nscd is excluded because
# it goes in nscd. The iconvconfig binary is kept in the main glibc package
# because we use it in the post-install scriptlet to rebuild the
# gconv-modules.cache.
grep '%{_prefix}/bin' master.filelist >> common.filelist
grep '%{_prefix}/sbin' master.filelist \
	| grep -v '%{_prefix}/sbin/iconvconfig' \
	| grep -v 'nscd' >> common.filelist
# All of the files under share go into the common package since they should be
# multilib-independent.
# Exceptions:
# - The actual share directory, not owned by us.
# - The info files which go in devel, and the info directory.
grep '%{_prefix}/share' master.filelist \
	| grep -v \
	-e '%{_prefix}/share/info/libc.info.*' \
	-e '%%dir %{prefix}/share/info' \
	-e '%%dir %{prefix}/share' \
	>> common.filelist

# Add the binary to build locales to the common subpackage.
echo '%{_prefix}/sbin/build-locale-archive' >> common.filelist

###############################################################################
# nscd
###############################################################################

# The nscd binary must go into the nscd subpackage.
echo '%{_prefix}/sbin/nscd' > nscd.filelist

###############################################################################
# glibc-utils
###############################################################################

# Add the utils scripts and programs to the utils subpackage.
cat > utils.filelist <<EOF
%if %{without bootstrap}
%{_prefix}/bin/memusage
%{_prefix}/bin/memusagestat
%endif
%{_prefix}/bin/mtrace
%{_prefix}/bin/pcprofiledump
%{_prefix}/bin/xtrace
EOF

###############################################################################
# nss_db, nss_hesiod
###############################################################################

# Move the NSS-related files to the NSS subpackages.  Be careful not
# to pick up .debug files, and the -devel symbolic links.
for module in db hesiod; do
  grep -E "/libnss_$module(\.so\.[0-9.]+|-[0-9.]+\.so)$" \
    master.filelist > nss_$module.filelist
done

###############################################################################
# nss-devel
###############################################################################

# Symlinks go into the nss-devel package (instead of the main devel
# package).
grep '/libnss_[a-z]*\.so$' master.filelist > nss-devel.filelist

###############################################################################
# libnsl
###############################################################################

# Prepare the libnsl-related file lists.
grep '/libnsl-[0-9.]*.so$' master.filelist > libnsl.filelist
test $(wc -l < libnsl.filelist) -eq 1

###############################################################################
# glibc-benchtests
###############################################################################

# List of benchmarks.
find build-%{target}/benchtests -type f -executable | while read b; do
	echo "%{_prefix}/libexec/glibc-benchtests/$(basename $b)"
done >> benchtests.filelist
# ... and the makefile.
for b in %{SOURCE9} %{SOURCE10}; do
	echo "%{_prefix}/libexec/glibc-benchtests/$(basename $b)" >> benchtests.filelist
done
# ... and finally, the comparison scripts.
echo "%{_prefix}/libexec/glibc-benchtests/benchout.schema.json" >> benchtests.filelist
echo "%{_prefix}/libexec/glibc-benchtests/compare_bench.py*" >> benchtests.filelist
echo "%{_prefix}/libexec/glibc-benchtests/import_bench.py*" >> benchtests.filelist
echo "%{_prefix}/libexec/glibc-benchtests/validate_benchout.py*" >> benchtests.filelist
%endif

###############################################################################
# compat-libpthread-nonshared
###############################################################################
echo "%{_libdir}/libpthread_nonshared.a" >> compat-libpthread-nonshared.filelist

###############################################################################
# glibc-debuginfocommon, and glibc-debuginfo
###############################################################################

find_debuginfo_args='--strict-build-id -g -i'
%ifarch %{debuginfocommonarches}
find_debuginfo_args="$find_debuginfo_args \
	-l common.filelist \
	-l utils.filelist \
	-l nscd.filelist \
	-p '.*/(sbin|libexec)/.*' \
	-o debuginfocommon.filelist \
	-l nss_db.filelist -l nss_hesiod.filelist \
	-l libnsl.filelist -l glibc.filelist \
%if %{with benchtests}
	-l benchtests.filelist
%endif
	"
%endif

/usr/lib/rpm/find-debuginfo.sh $find_debuginfo_args -o debuginfo.filelist

# List all of the *.a archives in the debug directory.
list_debug_archives()
{
	local dir=%{_prefix}/lib/debug%{_libdir}
	find %{glibc_sysroot}$dir -name "*.a" -printf "$dir/%%P\n"
}

%ifarch %{debuginfocommonarches}

# Remove the source files from the common package debuginfo.
sed -i '\#^%{glibc_sysroot}%{_prefix}/src/debug/#d' debuginfocommon.filelist

# Create a list of all of the source files we copied to the debug directory.
find %{glibc_sysroot}%{_prefix}/src/debug \
     \( -type d -printf '%%%%dir ' \) , \
     -printf '%{_prefix}/src/debug/%%P\n' > debuginfocommon.sources

%ifarch %{biarcharches}

# Add the source files to the core debuginfo package.
cat debuginfocommon.sources >> debuginfo.filelist

%else

%ifarch %{ix86}
%define basearch i686
%endif
%ifarch sparc sparcv9
%define basearch sparc
%endif

# The auxarches get only these few source files.
auxarches_debugsources=\
'/(generic|linux|%{basearch}|nptl(_db)?)/|/%{glibcsrcdir}/build|/dl-osinfo\.h'

# Place the source files into the core debuginfo pakcage.
egrep "$auxarches_debugsources" debuginfocommon.sources >> debuginfo.filelist

# Remove the source files from the common debuginfo package.
egrep -v "$auxarches_debugsources" \
  debuginfocommon.sources >> debuginfocommon.filelist

%endif # %{biarcharches}

# Add the list of *.a archives in the debug directory to
# the common debuginfo package.
list_debug_archives >> debuginfocommon.filelist

%endif # %{debuginfocommonarches}

# Remove some common directories from the common package debuginfo so that we
# don't end up owning them.
exclude_common_dirs()
{
	exclude_dirs="%{_prefix}/src/debug"
	exclude_dirs="$exclude_dirs $(echo %{_prefix}/lib/debug{,/%{_lib},/bin,/sbin})"
	exclude_dirs="$exclude_dirs $(echo %{_prefix}/lib/debug%{_prefix}{,/%{_lib},/libexec,/bin,/sbin})"

	for d in $(echo $exclude_dirs | sed 's/ /\n/g'); do
		sed -i "\|^%%dir $d/\?$|d" $1
	done
}

%ifarch %{debuginfocommonarches}
exclude_common_dirs debuginfocommon.filelist
%endif
exclude_common_dirs debuginfo.filelist

%endif # 0%{?_enable_debug_packages}

##############################################################################
# Delete files that we do not intended to ship with the auxarch.
# This is the only place where we touch the installed files after generating
# the file lists.
##############################################################################
%ifarch %{auxarches}
echo Cutting down the list of unpackaged files
sed -e '/%%dir/d;/%%config/d;/%%verify/d;s/%%lang([^)]*) //;s#^/*##' \
	common.filelist devel.filelist static.filelist headers.filelist \
	utils.filelist nscd.filelist \
%ifarch %{debuginfocommonarches}
	debuginfocommon.filelist \
%endif
	| (cd %{glibc_sysroot}; xargs --no-run-if-empty rm -f 2> /dev/null || :)
%endif # %{auxarches}

##############################################################################
# Run the glibc testsuite
##############################################################################
%check
%if %{with testsuite}

# Run the glibc tests. If any tests fail to build we exit %check with
# an error, otherwise we print the test failure list and the failed
# test output and continue.  Write to standard error to avoid
# synchronization issues with make and shell tracing output if
# standard output and standard error are different pipes.
run_tests () {
  # This hides a test suite build failure, which should be fatal.  We
  # check "Summary of test results:" below to verify that all tests
  # were built and run.
  make %{?_smp_mflags} -O check |& tee rpmbuild.check.log >&2
  test -n tests.sum
  if ! grep -q '^Summary of test results:$' rpmbuild.check.log ; then
    echo "FAIL: test suite build of target: $(basename "$(pwd)")" >& 2
    exit 1
  fi
  set +x
  grep -v ^PASS: tests.sum > rpmbuild.tests.sum.not-passing || true
  if test -n rpmbuild.tests.sum.not-passing ; then
    echo ===================FAILED TESTS===================== >&2
    echo "Target: $(basename "$(pwd)")" >& 2
    cat rpmbuild.tests.sum.not-passing >&2
    while read failed_code failed_test ; do
      for suffix in out test-result ; do
        if test -e "$failed_test.$suffix"; then
	  echo >&2
          echo "=====$failed_code $failed_test.$suffix=====" >&2
          cat -- "$failed_test.$suffix" >&2
	  echo >&2
        fi
      done
    done <rpmbuild.tests.sum.not-passing
  fi

  # Unconditonally dump differences in the system call list.
  echo "* System call consistency checks:" >&2
  cat misc/tst-syscall-list.out >&2
  set -x
}

# Increase timeouts
export TIMEOUTFACTOR=16
parent=$$
echo ====================TESTING=========================

# Default libraries.
pushd build-%{target}
run_tests
popd

%if %{buildpower9}
echo ====================TESTING -mcpu=power9=============
pushd build-%{target}-power9
run_tests
popd
%endif



echo ====================TESTING END=====================
PLTCMD='/^Relocation section .*\(\.rela\?\.plt\|\.rela\.IA_64\.pltoff\)/,/^$/p'
echo ====================PLT RELOCS LD.SO================
readelf -Wr %{glibc_sysroot}/%{_lib}/ld-*.so | sed -n -e "$PLTCMD"
echo ====================PLT RELOCS LIBC.SO==============
readelf -Wr %{glibc_sysroot}/%{_lib}/libc-*.so | sed -n -e "$PLTCMD"
echo ====================PLT RELOCS END==================

# Finally, check if valgrind runs with the new glibc.
# We want to fail building if valgrind is not able to run with this glibc so
# that we can then coordinate with valgrind to get it fixed before we update
# glibc.
pushd build-%{target}

# Show the auxiliary vector as seen by the new library
# (even if we do not perform the valgrind test).
LD_SHOW_AUXV=1 elf/ld.so --library-path .:elf:nptl:dlfcn /bin/true

%if %{with valgrind}
elf/ld.so --library-path .:elf:nptl:dlfcn \
	/usr/bin/valgrind --error-exitcode=1 \
	elf/ld.so --library-path .:elf:nptl:dlfcn /usr/bin/true
%endif
popd

%endif # %{run_glibc_tests}


%pre -p <lua>
-- Check that the running kernel is new enough
required = '%{enablekernel}'
rel = posix.uname("%r")
if rpm.vercmp(rel, required) < 0 then
  error("FATAL: kernel too old", 0)
end

%post -p <lua>
-- We use lua's posix.exec because there may be no shell that we can
-- run during glibc upgrade.
function post_exec (program, ...)
  local pid = posix.fork ()
  if pid == 0 then
    assert (posix.exec (program, ...))
  elseif pid > 0 then
    posix.wait (pid)
  end
end

-- (1) Remove multilib libraries from previous installs.
-- In order to support in-place upgrades, we must immediately remove
-- obsolete platform directories after installing a new glibc
-- version.  RPM only deletes files removed by updates near the end
-- of the transaction.  If we did not remove the obsolete platform
-- directories here, they may be preferred by the dynamic linker
-- during the execution of subsequent RPM scriptlets, likely
-- resulting in process startup failures.

-- Full set of libraries glibc may install.
install_libs = { "anl", "BrokenLocale", "c", "dl", "m", "mvec",
		 "nss_compat", "nss_db", "nss_dns", "nss_files",
		 "nss_hesiod", "pthread", "resolv", "rt", "SegFault",
		 "thread_db", "util" }

-- We are going to remove these libraries. Generally speaking we remove
-- all core libraries in the multilib directory.
-- We employ a tight match where X.Y is in [2.0,9.9*], so we would
-- match "libc-2.0.so" and so on up to "libc-9.9*".
remove_regexps = {}
for i = 1, #install_libs do
  remove_regexps[i] = ("lib" .. install_libs[i]
                       .. "%%-[2-9]%%.[0-9]+%%.so$")
end

-- Two exceptions:
remove_regexps[#install_libs + 1] = "libthread_db%%-1%%.0%%.so"
remove_regexps[#install_libs + 2] = "libSegFault%%.so"

-- We are going to search these directories.
local remove_dirs = { "%{_libdir}/i686",
		      "%{_libdir}/i686/nosegneg",
		      "%{_libdir}/power6",
		      "%{_libdir}/power7",
		      "%{_libdir}/power8" }

-- Walk all the directories with files we need to remove...
for _, rdir in ipairs (remove_dirs) do
  if posix.access (rdir) then
    -- If the directory exists we look at all the files...
    local remove_files = posix.files (rdir)
    for rfile in remove_files do
      for _, rregexp in ipairs (remove_regexps) do
	-- Does it match the regexp?
	local dso = string.match (rfile, rregexp)
        if (dso ~= nil) then
	  -- Removing file...
	  os.remove (rdir .. '/' .. rfile)
	end
      end
    end
  end
end

-- (2) Update /etc/ld.so.conf
-- Next we update /etc/ld.so.conf to ensure that it starts with
-- a literal "include ld.so.conf.d/*.conf".

local ldsoconf = "/etc/ld.so.conf"
local ldsoconf_tmp = "/etc/glibc_post_upgrade.ld.so.conf"

if posix.access (ldsoconf) then

  -- We must have a "include ld.so.conf.d/*.conf" line.
  local have_include = false
  for line in io.lines (ldsoconf) do
    -- This must match, and we don't ignore whitespace.
    if string.match (line, "^include ld.so.conf.d/%%*%%.conf$") ~= nil then
      have_include = true
    end
  end

  if not have_include then
    -- Insert "include ld.so.conf.d/*.conf" line at the start of the
    -- file. We only support one of these post upgrades running at
    -- a time (temporary file name is fixed).
    local tmp_fd = io.open (ldsoconf_tmp, "w")
    if tmp_fd ~= nil then
      tmp_fd:write ("include ld.so.conf.d/*.conf\n")
      for line in io.lines (ldsoconf) do
        tmp_fd:write (line .. "\n")
      end
      tmp_fd:close ()
      local res = os.rename (ldsoconf_tmp, ldsoconf)
      if res == nil then
        io.stdout:write ("Error: Unable to update configuration file (rename).\n")
      end
    else
      io.stdout:write ("Error: Unable to update configuration file (open).\n")
    end
  end
end

-- (3) Rebuild ld.so.cache early.
-- If the format of the cache changes then we need to rebuild
-- the cache early to avoid any problems running binaries with
-- the new glibc.

-- Note: We use _prefix because Fedora's UsrMove says so.
post_exec ("%{_prefix}/sbin/ldconfig")

-- (4) Update gconv modules cache.
-- If the /usr/lib/gconv/gconv-modules.cache exists, then update it
-- with the latest set of modules that were just installed.
-- We assume that the cache is in _libdir/gconv and called
-- "gconv-modules.cache".

local iconv_dir = "%{_libdir}/gconv"
local iconv_cache = iconv_dir .. "/gconv-modules.cache"
if (posix.utime (iconv_cache) == 0) then
  post_exec ("%{_prefix}/sbin/iconvconfig",
	     "-o", iconv_cache,
	     "--nostdlib",
	     iconv_dir)
else
  io.stdout:write ("Error: Missing " .. iconv_cache .. " file.\n")
end

%posttrans all-langpacks -e -p <lua>
-- If at the end of the transaction we are still installed
-- (have a template of non-zero size), then we rebuild the
-- locale cache (locale-archive) from the pre-populated
-- locale cache (locale-archive.tmpl) i.e. template.
if posix.stat("%{_prefix}/lib/locale/locale-archive.tmpl", "size") > 0 then
  pid = posix.fork()
  if pid == 0 then
    posix.exec("%{_prefix}/sbin/build-locale-archive", "--install-langs", "%%{_install_langs}")
  elseif pid > 0 then
    posix.wait(pid)
  end
end

%postun all-langpacks -p <lua>
-- In the postun we remove the locale cache if unstalling.
-- (build-locale-archive will delete the archive during an upgrade.)
if arg[2] == 0 then
  os.remove("%{_prefix}/lib/locale/locale-archive")
end

%if %{with docs}
%post devel
/sbin/install-info %{_infodir}/libc.info.gz %{_infodir}/dir > /dev/null 2>&1 || :
%endif

%pre headers
# this used to be a link and it is causing nightmares now
if [ -L %{_prefix}/include/scsi ] ; then
  rm -f %{_prefix}/include/scsi
fi

%if %{with docs}
%preun devel
if [ "$1" = 0 ]; then
  /sbin/install-info --delete %{_infodir}/libc.info.gz %{_infodir}/dir > /dev/null 2>&1 || :
fi
%endif

%pre -n nscd
getent group nscd >/dev/null || /usr/sbin/groupadd -g 28 -r nscd
getent passwd nscd >/dev/null ||
  /usr/sbin/useradd -M -o -r -d / -s /sbin/nologin \
		    -c "NSCD Daemon" -u 28 -g nscd nscd

%post -n nscd
%systemd_post nscd.service

%preun -n nscd
%systemd_preun nscd.service

%postun -n nscd
if test $1 = 0; then
  /usr/sbin/userdel nscd > /dev/null 2>&1 || :
fi
%systemd_postun_with_restart nscd.service

%files -f glibc.filelist
%dir %{_prefix}/%{_lib}/audit
%if %{buildpower9}
%dir /%{_lib}/power9
%endif
%ifarch s390x
/lib/ld64.so.1
%endif
%verify(not md5 size mtime) %config(noreplace) /etc/nsswitch.conf
%verify(not md5 size mtime) %config(noreplace) /etc/ld.so.conf
%verify(not md5 size mtime) %config(noreplace) /etc/rpc
%dir /etc/ld.so.conf.d
%dir %{_prefix}/libexec/getconf
%dir %{_libdir}/gconv
%dir %attr(0700,root,root) /var/cache/ldconfig
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/cache/ldconfig/aux-cache
%attr(0644,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /etc/ld.so.cache
%attr(0644,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /etc/gai.conf
%doc README NEWS INSTALL elf/rtld-debugger-interface.txt
# If rpm doesn't support %license, then use %doc instead.
%{!?_licensedir:%global license %%doc}
%license COPYING COPYING.LIB LICENSES

%ifnarch %{auxarches}
%files -f common.filelist common
%dir %{_prefix}/lib/locale
%dir %{_prefix}/lib/locale/C.utf8
%{_prefix}/lib/locale/C.utf8/*
%doc documentation/README.timezone
%doc documentation/gai.conf

%files all-langpacks
%attr(0644,root,root) %verify(not md5 size mtime) %{_prefix}/lib/locale/locale-archive.tmpl
%attr(0644,root,root) %verify(not md5 size mtime mode) %ghost %{_prefix}/lib/locale/locale-archive

%files locale-source
%dir %{_prefix}/share/i18n/locales
%{_prefix}/share/i18n/locales/*
%dir %{_prefix}/share/i18n/charmaps
%{_prefix}/share/i18n/charmaps/*

%files -f devel.filelist devel

%files -f static.filelist static

%files -f headers.filelist headers

%files -f utils.filelist utils

%files -f nscd.filelist -n nscd
%config(noreplace) /etc/nscd.conf
%dir %attr(0755,root,root) /var/run/nscd
%dir %attr(0755,root,root) /var/db/nscd
/lib/systemd/system/nscd.service
/lib/systemd/system/nscd.socket
%{_tmpfilesdir}/nscd.conf
%attr(0644,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/nscd.pid
%attr(0666,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/socket
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/passwd
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/group
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/hosts
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/services
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/db/nscd/passwd
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/db/nscd/group
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/db/nscd/hosts
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/db/nscd/services
%ghost %config(missingok,noreplace) /etc/sysconfig/nscd
%endif

%files -f nss_db.filelist -n nss_db
/var/db/Makefile
%files -f nss_hesiod.filelist -n nss_hesiod
%doc hesiod/README.hesiod
%files -f nss-devel.filelist nss-devel

%files -f libnsl.filelist -n libnsl
/%{_lib}/libnsl.so.1

%if 0%{?_enable_debug_packages}
%files debuginfo -f debuginfo.filelist
%ifarch %{debuginfocommonarches}
%ifnarch %{auxarches}
%files debuginfo-common -f debuginfocommon.filelist
%endif
%endif
%endif

%if %{with benchtests}
%files benchtests -f benchtests.filelist
%endif

%files -f compat-libpthread-nonshared.filelist -n compat-libpthread-nonshared

%changelog
* Thu Nov 28 2019 Florian Weimer <fweimer@redhat.com> - 2.28-72.1
- s390x: Fix z15 strstr for patterns crossing pages (#1777797)

* Mon Jul 22 2019 Carlos O'Donell <carlos@redhat.com> - 2.28-72
- Skip wide buffer handling for legacy stdio handles (#1722215)

* Mon Jul 22 2019 Carlos O'Donell <carlos@redhat.com> - 2.28-71
- Remove copy_file_range emulation (#1724975)

* Mon Jul 22 2019 Carlos O'Donell <carlos@redhat.com> - 2.28-70
- Avoid nscd assertion failure during persistent db check (#1727152)

* Mon Jul 22 2019 Carlos O'Donell <carlos@redhat.com> - 2.28-69
- Fix invalid free under valgrind with libdl (#1717438)

* Thu Jul 18 2019 Carlos O'Donell <carlos@redhat.com> - 2.28-68
- Account for size of locale-archive in rpm package (#1725131)

* Thu Jul 18 2019 Carlos O'Donell <carlos@redhat.com> - 2.28-67
- Reject IP addresses with trailing characters in getaddrinfo (#1727241)

* Fri Jun 14 2019 Florian Weimer <fweimer@redhat.com> - 2.28-66
- Avoid header conflict between <sys/stat.h> and <linux/stat.h> (#1699194)

* Wed Jun 12 2019 Florian Weimer <fweimer@redhat.com> - 2.28-65
- glibc-all-langpacks: Do not delete locale archive during update (#1717347)
- Do not mark /usr/lib/locale/locale-archive as a configuration file
  because it is always automatically overwritten by build-locale-archive.

* Mon Jun 10 2019 DJ Delorie <dj@redhat.com> - 2.28-64
- Avoid ABI exposure of the NSS service_user type (#1710894)

* Thu Jun  6 2019 Patsy Griffin Franklin <patsy@redhat.com> - 2.28-63
- Enable full ICMP errors for UDP DNS sockets. (#1670043)

* Mon Jun  3 2019 Carlos O'Donell <carlos@redhat.com> - 2.28-62
- Convert post-install binary to rpm lua scriptlet (#1639346)

* Mon Jun  3 2019 Florian Weimer <fweimer@redhat.com> - 2.28-61
- Fix crash during wide stream buffer flush (#1710478)

* Fri May 31 2019 Carlos O'Donell <carlos@redhat.com> - 2.28-60
- Add PF_XDP, AF_XDP and SOL_XDP from Linux 4.18 (#1706777)

* Wed May 22 2019 DJ Delorie <dj@redhat.com> - 2.28-59
- Add .gdb_index to debug information (#1612448)

* Wed May 22 2019 DJ Delorie <dj@redhat.com) - 2.28-58
- iconv, localedef: avoid floating point rounding differences (#1691528)

* Wed May 22 2019 Arjun Shankar <arjun@redhat.com> - 2.28-57
- locale: Add LOCPATH diagnostics to the locale program (#1701605)

* Fri May 17 2019 Patsy Griffin Franklin <patsy@redhat.com> - 2.28-56
- Fix hang in pldd.  (#1702539)

* Mon May 13 2019 Florian Weimer <fweimer@redhat.com> - 2.28-55
- s390x string function improvements (#1659438)

* Thu May  2 2019 Patsy Griffin Franklin <patsy@redhat.com> - 2.28-54
- Fix test suite failures due to race conditions in posix/tst-spawn
  spawned processes. (#1659512)

* Wed May  1 2019 DJ Delorie <dj@redhat.com> - 2.28-53
- Add missing CFI data to __mpn_* functions on ppc64le (#1658901)

* Fri Apr 26 2019 Arjun Shankar <arjun@redhat.com> - 2.28-52
- intl: Do not return NULL on asprintf failure in gettext (#1663035)

* Fri Apr 26 2019 Florian Weimer <fweimer@redhat.com> - 2.28-51
- Increase BIND_NOW coverage (#1639343)

* Tue Apr 23 2019 Carlos O'Donell <carlos@redhat.com> - 2.28-50
- Fix pthread_rwlock_trywrlock and pthread_rwlock_tryrdlock stalls (#1659293)

* Tue Apr 23 2019 Arjun Shankar <arjun@redhat.com> - 2.28-49
- malloc: Improve bad chunk detection (#1651283)

* Mon Apr 22 2019 Patsy Griffin Franklin <patsy@redhat.com> - 2.28-48
- Add compiler barriers around modifications of the robust mutex list
  for pthread_mutex_trylock. (#1672773)

* Tue Apr 16 2019 DJ Delorie <dj@redhat.com> - 2.28-47
- powerpc: Only enable HTM if kernel supports PPC_FEATURE2_HTM_NOSC (#1651742)

* Fri Apr 12 2019 Florian Weimer <fweimer@redhat.com> - 2.28-46
- Only build libm with -fno-math-errno (#1664408)

* Tue Apr  2 2019 Florian Weimer <fweimer@redhat.com> - 2.28-45
- ja_JP: Add new Japanese Era name (#1577438)

* Wed Mar  6 2019 Florian Weimer <fweimer@redhat.com> - 2.28-44
- math: Add XFAILs for some IBM 128-bit long double fma tests (#1623537)

* Fri Mar  1 2019 Florian Weimer <fweimer@redhat.com> - 2.28-43
- malloc: realloc ncopies integer overflow (#1662843)

* Fri Dec 14 2018 Florian Weimer <fweimer@redhat.com> - 2.28-42
- Fix rdlock stall with PREFER_WRITER_NONRECURSIVE_NP (#1654872)

* Fri Dec 14 2018 Florian Weimer <fweimer@redhat.com> - 2.28-41
- malloc: Implement double-free check for the thread cache (#1642094)

* Thu Dec 13 2018 Florian Weimer <fweimer@redhat.com> - 2.28-40
- Add upstream test case for CVE-2018-19591 (#1654010)

* Thu Dec 13 2018 Florian Weimer <fweimer@redhat.com> - 2.28-39
- Add GCC dependency for new inline string functions on ppc64le (#1652932)

* Sat Dec 01 2018 Carlos O'Donell <carlos@redhat.com> - 2.28-38
- Add requires on explicit glibc version for glibc-nss-devel (#1649890)

* Fri Nov 30 2018 Carlos O'Donell <carlos@redhat.com> - 2.28-37
- Fix data race in dynamic loader when using LD_AUDIT (#1635779)

* Wed Nov 28 2018 Florian Weimer <fweimer@redhat.com> - 2.28-36
- CVE-2018-19591: File descriptor leak in if_nametoindex (#1654010)

* Mon Nov 26 2018 Florian Weimer <fweimer@redhat.com> - 2.28-35
- Do not use parallel make for building locales (#1652229)

* Tue Nov 20 2018 Florian Weimer <fweimer@redhat.com> - 2.28-34
- support: Print timestamps in timeout handler (#1651274)

* Tue Nov 20 2018 Florian Weimer <fweimer@redhat.com> - 2.28-33
- Increase test timeout for  libio/tst-readline (#1638520)

* Tue Nov 20 2018 Florian Weimer <fweimer@redhat.com> - 2.28-32
- Fix tzfile low-memory assertion failure (#1650571)

* Tue Nov 20 2018 Florian Weimer <fweimer@redhat.com> - 2.28-31
- Add newlines in __libc_fatal calls (#1650566)

* Tue Nov 20 2018 Florian Weimer <fweimer@redhat.com> - 2.28-30
- nscd: Fix use-after-free in addgetnetgrentX (#1650563)

* Tue Nov 20 2018 Florian Weimer <fweimer@redhat.com> - 2.28-29
- Update syscall names to Linux 4.19 (#1650560)

* Tue Nov 13 2018 Florian Weimer <fweimer@redhat.com> - 2.28-28
- kl_GL: Fix spelling of Sunday, should be "sapaat" (#1645597)

* Tue Nov 13 2018 Florian Weimer <fweimer@redhat.com> - 2.28-27
- Fix x86 CPU flags analysis for string function selection (#1641982)

* Fri Nov  9 2018 Florian Weimer <fweimer@redhat.com> - 2.28-26
- Reduce RAM requirements for stdlib/test-bz22786 (#1638523)

* Fri Nov  9 2018 Florian Weimer <fweimer@redhat.com> - 2.28-25
- x86: Improve enablement for 32-bit code using CET (#1645601)

* Fri Nov  9 2018 Florian Weimer <fweimer@redhat.com> - 2.28-24
- Fix crash in getaddrinfo_a when thread creation fails (#1646379)

* Fri Nov  9 2018 Florian Weimer <fweimer@redhat.com> - 2.28-23
- Fix race in pthread_mutex_lock related to PTHREAD_MUTEX_ELISION_NP (#1645604)

* Fri Nov  9 2018 Florian Weimer <fweimer@redhat.com> - 2.28-22
- Fix misreported errno on preadv2/pwritev2 (#1645596)

* Fri Nov  9 2018 Florian Weimer <fweimer@redhat.com> - 2.28-21
- Fix posix/tst-spawn4-compat test case (#1645593)

* Fri Nov  9 2018 Florian Weimer <fweimer@redhat.com> - 2.28-20
- Disable CET for binaries created by older link editors (#1614979)

* Fri Nov  2 2018 Mike FABIAN <mfabian@redhat.com> - 2.28-19
- Include Esperanto (eo) in glibc-all-langpacks (#1644303)

* Thu Sep 27 2018 Florian Weimer <fweimer@redhat.com> - 2.28-18
- stdlib/tst-setcontext9 test suite failure on ppc64le (#1623536)

* Wed Sep 26 2018 Florian Weimer <fweimer@redhat.com> - 2.28-17
- Add missing ENDBR32 in start.S (#1631730)

* Wed Sep 26 2018 Florian Weimer <fweimer@redhat.com> - 2.28-16
- Fix bug in generic strstr with large needles (#1631722)

* Wed Sep 26 2018 Florian Weimer <fweimer@redhat.com> - 2.28-15
- stdlib/tst-setcontext9 test suite failure (#1623536)

* Wed Sep 26 2018 Florian Weimer <fweimer@redhat.com> - 2.28-14
- gethostid: Missing NULL check for gethostbyname_r (#1631293)

* Wed Sep  5 2018 Carlos O'Donell <carlos@redhat.com> - 2.28-13
- Provide compatibility support for linking against libpthread_nonshared.a
  (#1614439)

* Wed Sep  5 2018 Florian Weimer <fweimer@redhat.com> - 2.28-12
- Add python3-devel build dependency (#1625592)

* Wed Aug 29 2018 Florian Weimer <fweimer@redhat.com> - 2.28-11
- Drop glibc-ldflags.patch and valgrind bug workaround (#1623456)

* Wed Aug 29 2018 Florian Weimer <fweimer@redhat.com> - 2.28-10
- regex: Fix memory overread when pattern contains NUL byte (#1622678)

* Wed Aug 29 2018 Florian Weimer <fweimer@redhat.com> - 2.28-9
- nptl: Fix waiters-after-spinning case in pthread_cond_broadcast (#1622675)

* Tue Aug 14 2018 Florian Weimer <fweimer@redhat.com> - 2.28-8
- nss_files aliases database file stream leak (#1615790)

* Tue Aug 14 2018 Florian Weimer <fweimer@redhat.com> - 2.28-7
- Fix static analysis warning in nscd user name allocation (#1615784)

* Tue Aug 14 2018 Florian Weimer <fweimer@redhat.com> - 2.28-6
- error, error_at_line: Add missing va_end calls (#1615781)

* Mon Aug 13 2018 Carlos O'Donell <carlos@redhat.com> - 2.28-5
- Remove abort() warning in manual (#1577365)

* Fri Aug 10 2018 Florian Weimer <fweimer@redhat.com> - 2.28-4
- Fix regression in readdir64@GLIBC_2.1 compat symbol (#1614253)

* Thu Aug  2 2018 Florian Weimer <fweimer@redhat.com> - 2.28-3
- Log /proc/sysinfo if available (on s390x)

* Thu Aug  2 2018 Florian Weimer <fweimer@redhat.com> - 2.28-2
- Honor %%{valgrind_arches}

* Wed Aug 01 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-43
- Update to glibc 2.28 release tarball:
- Translation updates
- x86/CET: Fix property note parser (swbz#23467)
- x86: Add tst-get-cpu-features-static to $(tests) (swbz#23458)

* Mon Jul 30 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-42
- Auto-sync with upstream branch master,
  commit af86087f02a5522d8801a11d8381e04f95e33162:
- x86/CET: Don't parse beyond the note end
- Fix Linux fcntl OFD locks tests on unsupported kernels
- x86: Populate COMMON_CPUID_INDEX_80000001 for Intel CPUs (swbz#23459)
- x86: Correct index_cpu_LZCNT (swbz#23456)
- Fix string/tst-xbzero-opt if build with gcc head

* Thu Jul 26 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-41
- Build with --enable-cet on x86_64, i686
- Auto-sync with upstream branch master,
  commit cfba5dbb10cc3abde632b46c60c10b2843917035:
- Keep expected behaviour for [a-z] and [A-z] (#1607286)
- Additional ucontext tests
- Intel CET enhancements
- ISO C11 threads support
- Fix out-of-bounds access in IBM-1390 converter (swbz#23448)
- New locale Yakut (Sakha) for Russia (sah_RU) (swbz#22241)
- os_RU: Add alternative month names (swbz#23140)
- powerpc64: Always restore TOC on longjmp (swbz#21895)
- dsb_DE locale: Fix syntax error and add tests (swbz#23208)
- Improve performance of the generic strstr implementation
- regcomp: Fix off-by-one bug in build_equiv_class (swbz#23396)
- Fix out of bounds access in findidxwc (swbz#23442)

* Fri Jul 13 2018 Carlos O'Donell <carlos@redhat.com> - 2.27.9000-40
- Fix file list for glibc RPM packaging (#1601011).

* Wed Jul 11 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-39
- Add POWER9 multilib (downstream only)

* Wed Jul 11 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-38
- Auto-sync with upstream branch master,
  commit 93304f5f7a32f73b551266c5a181db51d97a71e4:
- Install <bits/statx.h> header
- Put the correct Unicode version number 11.0.0 into the generated files

* Wed Jul 11 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-37
- Work around valgrind issue on i686 (#1600034)

* Tue Jul 10 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-36
- Auto-sync with upstream branch master,
  commit fd70af45528d59a00eb3190ef6706cb299488fcd:
- Add the statx function
- regexec: Fix off-by-one bug in weight comparison (#1582229)
- nss_files: Fix re-reading of long lines (swbz#18991)
- aarch64: add HWCAP_ATOMICS to HWCAP_IMPORTANT
- aarch64: Remove HWCAP_CPUID from HWCAP_IMPORTANT
- conform/conformtest.pl: Escape literal braces in regular expressions
- x86: Use AVX_Fast_Unaligned_Load from Zen onwards.

* Fri Jul  6 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-35
- Remove ppc64 multilibs

* Fri Jul 06 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-34
- Auto-sync with upstream branch master,
  commit 3a885c1f51b18852869a91cf59a1b39da1595c7a.

* Thu Jul  5 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-33
- Enable build flags inheritance for nonshared flags

* Wed Jul  4 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-32
- Add annobin annotations to assembler code (#1548438)

* Wed Jul  4 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-31
- Enable -D_FORTIFY_SOURCE=2 for nonshared code

* Mon Jul 02 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-30
- Auto-sync with upstream branch master,
  commit b7b88cea4151d85eafd7ababc2e4b7ae1daeedf5:
- New locale: dsb_DE (Lower Sorbian)

* Fri Jun 29 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-29
- Drop glibc-deprecate_libcrypt.patch.  Variant applied upstream. (#1566464)
- Drop glibc-linux-timespec-header-compat.patch.  Upstreamed.
- Auto-sync with upstream branch master,
  commit e69d994a63afc2d367f286a2a7df28cbf710f0fe.

* Thu Jun 28 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-28
- Drop glibc-rh1315108.patch.  extend_alloca was removed upstream. (#1315108)
- Auto-sync with upstream branch master,
  commit c49e18222e4c40f21586dabced8a49732d946917.

* Thu Jun 21 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-27
- Compatibility fix for <sys/stat.h> and <linux/time.h>

* Thu Jun 21 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-26
- Auto-sync with upstream branch master,
  commit f496b28e61d0342f579bf794c71b80e9c7d0b1b5.

* Mon Jun 18 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-25
- Auto-sync with upstream branch master,
  commit f2857da7cdb65bfad75ee30981f5b2fde5bbb1dc.

* Mon Jun 18 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-24
- Auto-sync with upstream branch master,
  commit 14beef7575099f6373f9a45b4656f1e3675f7372:
- iconv: Make IBM273 equivalent to ISO-8859-1 (#1592270)

* Mon Jun 18 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-23
- Inherit the -msse2 build flag as well (#1592212)

* Fri Jun 01 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-22
- Modernise nsswitch.conf defaults (#1581809)
- Adjust build flags inheritence from redhat-rpm-config
- Auto-sync with upstream branch master,
  commit 104502102c6fa322515ba0bb3c95c05c3185da7a.

* Fri May 25 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-21
- Auto-sync with upstream branch master,
  commit c1dc1e1b34873db79dfbfa8f2f0a2abbe28c0514.

* Wed May 23 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-20
- Auto-sync with upstream branch master,
  commit 7f9f1ecb710eac4d65bb02785ddf288cac098323:
- CVE-2018-11237: Buffer overflow in __mempcpy_avx512_no_vzeroupper (#1581275)
- Drop glibc-rh1452750-allocate_once.patch,
  glibc-rh1452750-libidn2.patch.  Applied upstream.

* Wed May 23 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-19
- Auto-sync with upstream branch master,
  commit 8f145c77123a565b816f918969e0e35ee5b89153.

* Thu May 17 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-18
- Do not run telinit u on upgrades (#1579225)
- Auto-sync with upstream branch master,
  commit 632a6cbe44cdd41dba7242887992cdca7b42922a.

* Fri May 11 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-17
- Avoid exporting some Sun RPC symbols with default versions (#1577210)
- Inherit the -mstackrealign flag if it is set
- Inherit compiler flags in the original order
- Auto-sync with upstream branch master,
  commit 89aacb513eb77549a29df2638913a0f8178cf3f5:
- CVE-2018-11236: realpath: Fix path length overflow (#1581270, swbz#22786)

* Fri May 11 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-16
- Use /usr/bin/python3 for benchmarks scripts (#1577223)

* Thu Apr 19 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-15
- Auto-sync with upstream branch master,
  commit 0085be1415a38b40a5a1a12e49368498f1687380.

* Mon Apr 09 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-14
- Auto-sync with upstream branch master,
  commit 583a27d525ae189bdfaa6784021b92a9a1dae12e.

* Thu Mar 29 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-13
- Auto-sync with upstream branch master,
  commit d39c0a459ef32a41daac4840859bf304d931adab:
- CVE-2017-18269: memory corruption in i386 memmove (#1580934)

* Mon Mar 19 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-12
- Auto-sync with upstream branch master,
  commit fbce6f7260c3847f14dfa38f60c9111978fb33a5.

* Fri Mar 16 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-11
- Auto-sync with upstream branch master,
  commit 700593fdd7aef1e36cfa8bad969faab76a6facda.

* Wed Mar 14 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-10
- Auto-sync with upstream branch master,
  commit 7108f1f944792ac68332967015d5e6418c5ccc88.

* Mon Mar 12 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-9
- Auto-sync with upstream branch master,
  commit da6d4404ecfd7eacba8c096b0761a5758a59da4b.

* Tue Mar  6 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-8
- Enable annobin annotations (#1548438)

* Thu Mar 01 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-7
- Auto-sync with upstream branch master,
  commit 1a2f44a848663036c8a14671fe0faa3fed0b2a25:
- Remove spurios reference to libpthread_nonshared.a

* Thu Mar 01 2018 Florian Weimer <fweimer@redhat.com> - 2.27.9000-6
- Switch back to upstream master branch
- Drop glibc-rh1013801.patch, applied upstream.
- Drop glibc-fedora-nptl-linklibc.patch, no longer needed.
- Auto-sync with upstream branch master,
  commit bd60ce86520b781ca24b99b2555e2ad389bbfeaa.

* Wed Feb 28 2018 Florian Weimer <fweimer@redhat.com> - 2.27-5
- Inherit as many flags as possible from redhat-rpm-config (#1550914)

* Mon Feb 19 2018 Richard W.M. Jones <rjones@redhat.com> - 2.27-4
- riscv64: Add symlink from /usr/lib64/lp64d -> /usr/lib64 for ABI compat.
- riscv64: Disable valgrind smoke test on this architecture.

* Wed Feb 14 2018 Florian Weimer <fweimer@redhat.com> - 2.27-3
- Spec file cleanups:
  - Remove %%defattr(-,root,root)
  - Use shell to run ldconfig %%transfiletrigger
  - Move %%transfiletrigger* to the glibc-common subpackage
  - Trim changelog
  - Include ChangeLog.old in the source RPM

* Wed Feb  7 2018 Florian Weimer <fweimer@redhat.com> - 2.27-2.1
- Linux: use reserved name __key in pkey_get (#1542643)
- Auto-sync with upstream branch release/2.27/master,
  commit 56170e064e2b21ce204f0817733e92f1730541ea.

* Wed Feb 07 2018 Fedora Release Engineering <releng@fedoraproject.org>
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Mon Feb 05 2018 Carlos O'Donell <carlos@redhat.com> - 2.27-1
- Update to released glibc 2.27.
- Auto-sync with upstream branch master,
  commit 23158b08a0908f381459f273a984c6fd328363cb.

* Tue Jan 30 2018 Richard W.M. Jones <rjones@redhat.com> - 2.26.9000-52
- Disable -fstack-clash-protection on riscv64:
  not supported even by GCC 7.3.1 on this architecture.

* Mon Jan 29 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-51
- Explicitly run ldconfig in the buildroot
- Do not run ldconfig from scriptlets
- Put triggers into the glibc-common package, do not pass arguments to ldconfig

* Mon Jan 29 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-50
- Auto-sync with upstream branch master,
  commit cdd14619a713ab41e26ba700add4880604324dbb:
- libnsl: Turn remaining symbols into compat symbols (swbz#22701)
- be_BY, be_BY@latin, lt_LT, el_CY, el_GR, ru_RU, ru_UA, uk_UA:
  Add alternative month names (swbz#10871)
- x86: Revert Intel CET changes to __jmp_buf_tag (swbz#22743)
- aarch64: Revert the change of the __reserved member of mcontext_t

* Mon Jan 29 2018 Igor Gnatenko <ignatenkobrain@fedoraproject.org> - 2.26.9000-49
- Add file triggers to do ldconfig calls automatically

* Mon Jan 22 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-48
- Auto-sync with upstream branch master,
  commit 21c0696cdef617517de6e25711958c40455c554f:
- locale: Implement alternative month names (swbz#10871)
- locale: Change month names for pl_PL (swbz#10871)

* Mon Jan 22 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-47
- Unconditionally build without libcrypt

* Fri Jan 19 2018 Björn Esser <besser82@fedoraproject.org> - 2.26.9000-46
- Remove deprecated libcrypt, gets replaced by libxcrypt
- Add applicable Requires on libxcrypt

* Fri Jan 19 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-45
- Drop static PIE support on aarch64.  It leads to crashes at run time.
- Remove glibc-rpcgen subpackage.  See rpcsvc-proto.  (#1531540)

* Fri Jan 19 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-44
- Correct the list of static PIE architectures (#1247050)
- glibc_post_upgrade: Remove process restart logic
- glibc_post_upgrade: Integrate into the build process
- glibc_post_upgrade: Do not clean up tls subdirectories
- glibc_post_upgrade: Drop ia64 support
- Remove architecture-specific symbolic link for iconvconfig
- Auto-sync with upstream branch master,
  commit 4612268a0ad8e3409d8ce2314dd2dd8ee0af5269:
- powerpc: Fix syscalls during early process initialization (swbz#22685)

* Fri Jan 19 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-43
- Enable static PIE support on i386, x86_64 (#1247050)
- Remove add-on support (already gone upstream)
- Rework test suite status reporting
- Auto-sync with upstream branch master,
  commit 64f63cb4583ecc1ba16c7253aacc192b6d088511:
- malloc: Fix integer overflows in memalign and malloc functions (swbz#22343)
- x86-64: Properly align La_x86_64_retval to VEC_SIZE (swbz#22715)
- aarch64: Update bits/hwcap.h for Linux 4.15
- Add NT_ARM_SVE to elf.h

* Wed Jan 17 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-42
- CVE-2017-14062, CVE-2016-6261, CVE-2016-6263:
  Use libidn2 for IDNA support (#1452750)

* Mon Jan 15 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-41
- CVE-2018-1000001: Make getcwd fail if it cannot obtain an absolute path
  (#1533837)
- elf: Synchronize DF_1_* flags with binutils (#1439328)
- Auto-sync with upstream branch master,
  commit 860b0240a5645edd6490161de3f8d1d1f2786025:
- aarch64: fix static pie enabled libc when main is in a shared library
- malloc: Ensure that the consolidated fast chunk has a sane size

* Fri Jan 12 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-40
- libnsl: Do not install libnsl.so, libnsl.a (#1531540)
- Use unversioned Supplements: for langpacks (#1490725)
- Auto-sync with upstream branch master,
  commit 9a08a366a7e7ddffe62113a9ffe5e50605ea0924:
- hu_HU locale: Avoid double space (swbz#22657)
- math: Make default libc_feholdsetround_noex_ctx use __feholdexcept
  (swbz#22702)

* Thu Jan 11 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-39
- nptl: Open libgcc.so with RTLD_NOW during pthread_cancel (#1527887)
- Introduce libnsl subpackage and remove NIS headers (#1531540)
- Use versioned Obsoletes: for libcrypt-nss.
- Auto-sync with upstream branch master,
  commit 08c6e95234c60a5c2f37532d1111acf084f39345:
- nptl: Add tst-minstack-cancel, tst-minstack-exit (swbz#22636)
- math: ldbl-128ibm log1pl (-qNaN) spurious "invalid" exception (swbz#22693)

* Wed Jan 10 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-38
- nptl: Fix stack guard size accounting (#1527887)
- Remove invalid Obsoletes: on glibc-header provides
- Require python3 instead of python during builds
- Auto-sync with upstream branch master,
  commit 09085ede12fb9650f286bdcd805609ae69f80618:
- math: ldbl-128ibm lrintl/lroundl missing "invalid" exceptions (swbz#22690)
- x86-64: Add sincosf with vector FMA

* Mon Jan  8 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-37
- Add glibc-rpcgen subpackage, until the replacement is packaged (#1531540)

* Mon Jan 08 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-36
- Auto-sync with upstream branch master,
  commit 579396ee082565ab5f42ff166a264891223b7b82:
- nptl: Add test for callee-saved register restore in pthread_exit
- getrlimit64: fix for 32-bit configurations with default version >= 2.2
- elf: Add linux-4.15 VDSO hash for RISC-V
- elf: Add RISC-V dynamic relocations to elf.h
- powerpc: Fix error message during relocation overflow
- prlimit: Replace old_rlimit RLIM64_INFINITY with RLIM_INFINITY (swbz#22678)

* Fri Jan 05 2018 Florian Weimer <fweimer@redhat.com> - 2.26.9000-35
- Remove sln (#1531546)
- Remove Sun RPC interfaces (#1531540)
- Rebuild with newer GCC to fix pthread_exit stack unwinding issue (#1529549)
- Auto-sync with upstream branch master,
  commit f1a844ac6389ea4e111afc019323ca982b5b027d:
- CVE-2017-16997: elf: Check for empty tokens before DST expansion (#1526866)
- i386: In makecontext, align the stack before calling exit (swbz#22667)
- x86, armhfp: sync sys/ptrace.h with Linux 4.15 (swbz#22433)
- elf: check for rpath emptiness before making a copy of it
- elf: remove redundant is_path argument
- elf: remove redundant code from is_dst
- elf: remove redundant code from _dl_dst_substitute
- scandir: fix wrong assumption about errno (swbz#17804)
- Deprecate external use of libio.h and _G_config.h

* Fri Dec 22 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-34
- Auto-sync with upstream branch master,
  commit bad7a0c81f501fbbcc79af9eaa4b8254441c4a1f:
- copy_file_range: New function to copy file data
- nptl: Consolidate pthread_{timed,try}join{_np}
- nptl: Implement pthread_self in libc.so (swbz#22635)
- math: Provide a C++ version of iseqsig (swbz#22377)
- elf: remove redundant __libc_enable_secure check from fillin_rpath
- math: Avoid signed shift overflow in pow (swbz#21309)
- x86: Add feature_1 to tcbhead_t (swbz#22563)
- x86: Update cancel_jmp_buf to match __jmp_buf_tag (swbz#22563)
- ld.so: Examine GLRO to detect inactive loader (swbz#20204)
- nscd: Fix nscd readlink argument aliasing (swbz#22446)
- elf: do not substitute dst in $LD_LIBRARY_PATH twice (swbz#22627)
- ldconfig: set LC_COLLATE to C (swbz#22505)
- math: New generic sincosf
- powerpc: st{r,p}cpy optimization for aligned strings
- CVE-2017-1000409: Count in expanded path in _dl_init_path (#1524867)
- CVE-2017-1000408: Compute correct array size in _dl_init_paths (#1524867)
- x86-64: Remove sysdeps/x86_64/fpu/s_cosf.S
- aarch64: Improve strcmp unaligned performance

* Wed Dec 13 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-33
- Remove power6 platform directory (#1522675)

* Wed Dec 13 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-32
- Obsolete the libcrypt-nss subpackage (#1525396)
- armhfp: Disable -fstack-clash-protection due to GCC bug (#1522678)
- ppc64: Disable power6 multilib due to GCC bug (#1522675)
- Auto-sync with upstream branch master,
  commit 243b63337c2c02f30ec3a988ecc44bc0f6ffa0ad:
- libio: Free backup area when it not required (swbz#22415)
- math: Fix nextafter and nexttoward declaration (swbz#22593)
- math: New generic cosf
- powerpc: POWER8 memcpy optimization for cached memory
- x86-64: Add sinf with FMA
- x86-64: Remove sysdeps/x86_64/fpu/s_sinf.S
- math: Fix ctanh (0 + i NaN), ctanh (0 + i Inf) (swbz#22568)
- lt_LT locale: Base collation on copy "iso14651_t1" (swbz#22524)
- math: Add _Float32 function aliases
- math: Make cacosh (0 + iNaN) return NaN + i pi/2 (swbz#22561)
- hsb_DE locale: Base collation on copy "iso14651_t1" (swbz#22515)

* Wed Dec 06 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-31
- Add elision tunables.  Drop related configure flag.  (#1383986)
- Auto-sync with upstream branch master,
  commit 37ac8e635a29810318f6d79902102e2e96b2b5bf:
- Linux: Implement interfaces for memory protection keys
- math: Add _Float64, _Float32x function aliases
- math: Use sign as double for reduced case in sinf
- math: fix sinf(NAN)
- math: s_sinf.c: Replace floor with simple casts
- et_EE locale: Base collation on iso14651_t1 (swbz#22517)
- tr_TR locale: Base collation on iso14651_t1 (swbz#22527)
- hr_HR locale: Avoid single code points for digraphs in LC_TIME (swbz#10580)
- S390: Fix backtrace in vdso functions

* Mon Dec 04 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-30
- Add build dependency on bison
- Auto-sync with upstream branch master,
  commit 7863a7118112fe502e8020a0db0fa74fef281f29:
- math: New generic sinf (swbz#5997)
- is_IS locale: Base collation on iso14651_t1 (swbz#22519)
- intl: Improve reproducibility by using bison (swbz#22432)
- sr_RS, bs_BA locales: make collation rules the same as for hr_HR (wbz#22534)
- hr_HR locale: various updates (swbz#10580)
- x86: Make a space in jmpbuf for shadow stack pointer
- CVE-2017-17426: malloc: Fix integer overflow in tcache (swbz#22375)
- locale: make forward accent sorting the default in collating (swbz#17750)

* Wed Nov 29 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-29
- Enable -fstack-clash-protection (#1512531)
- Auto-sync with upstream branch master,
  commit a55430cb0e261834ce7a4e118dd9e0f2b7fb14bc:
- elf: Properly compute offsets of note descriptor and next note (swbz#22370)
- cs_CZ locale: Base collation on iso14651_t1 (swbz#22336)
- Implement the mlock2 function
- Add _Float64x function aliases
- elf: Consolidate link map sorting
- pl_PL locale: Base collation on iso14651_t1 (swbz#22469)
- nss: Export nscd hash function as __nss_hash (swbz#22459)

* Thu Nov 23 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-28
- Auto-sync with upstream branch master,
  commit cccb6d4e87053ed63c74aee063fa84eb63ebf7b8:
- sigwait can fail with EINTR (#1516394)
- Add memfd_create function
- resolv: Fix p_secstodate overflow handling (swbz#22463)
- resolv: Obsolete p_secstodate
- Avoid use of strlen in getlogin_r (swbz#22447)
- lv_LV locale: fix collation (swbz#15537)
- S390: Add cfi information for start routines in order to stop unwinding
- aarch64: Optimized memset for falkor

* Sun Nov 19 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-27
- Auto-sync with upstream branch master,
  commit f6e965ee94b37289f64ecd3253021541f7c214c3:
- powerpc: AT_HWCAP2 bit PPC_FEATURE2_HTM_NO_SUSPEND
- aarch64: Add HWCAP_DCPOP bit
- ttyname, ttyname_r: Don't bail prematurely (swbz#22145)
- signal: Optimize sigrelse implementation
- inet: Check length of ifname in if_nametoindex (swbz#22442)
- malloc: Account for all heaps in an arena in malloc_info (swbz#22439)
- malloc: Add missing arena lock in malloc_info (swbz#22408)
- malloc: Use __builtin_tgmath in tgmath.h with GCC 8 (swbz#21660)
- locale: Replaced unicode sequences in the ASCII printable range
- resolv: More precise checks in res_hnok, res_dnok (swbz#22409, swbz#22412)
- resolv: ns_name_pton should report trailing \ as error (swbz#22413)
- locale: mfe_MU, miq_NI, an_ES, kab_DZ, om_ET: Escape / in d_fmt (swbz#22403)

* Tue Nov 07 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-26
- Auto-sync with upstream branch master,
  commit 6b86036452b9ac47b4ee7789a50f2f37df7ecc4f:
- CVE-2017-15804: glob: Fix buffer overflow during GLOB_TILDE unescaping
- powerpc: Use latest string function optimization for internal function calls
- math: No _Float128 support for ppc64le -mlong-double-64 (swbz#22402)
- tpi_PG locale: Fix wrong d_fmt
- aarch64: Disable lazy symbol binding of TLSDESC
- tpi_PG locale: fix syntax error (swbz#22382)
- i586: Use conditional branches in strcpy.S (swbz#22353)
- ffsl, ffsll: Declare under __USE_MISC, not just __USE_GNU
- csb_PL locale: Fix abmon/mon for March (swbz#19485)
- locale: Various yesstr/nostr/yesexpr/noexpr fixes (swbz#15260, swbz#15261)
- localedef: Add --no-warnings/--warnings option
- powerpc: Replace lxvd2x/stxvd2x with lvx/stvx in P7's memcpy/memmove
- locale: Use ASCII as much as possible in LC_MESSAGES
- Add new locale yuw_PG (swbz#20952)
- malloc: Add single-threaded path to malloc/realloc/calloc/memalloc
- i386: Replace assembly versions of e_powf with generic e_powf.c
- i386: Replace assembly versions of e_log2f with generic e_log2f.c
- x86-64: Add powf with FMA
- x86-64: Add logf with FMA
- i386: Replace assembly versions of e_logf with generic e_logf.c
- i386: Replace assembly versions of e_exp2f with generic e_exp2f.c
- x86-64: Add exp2f with FMA
- i386: Replace assembly versions of e_expf with generic e_expf.c

* Sat Oct 21 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-25
- Auto-sync with upstream branch master,
  commit 797ba44ba27521261f94cc521f1c2ca74f650147:
- math: Add bits/floatn.h defines for more _FloatN / _FloatNx types
- posix: Fix improper assert in Linux posix_spawn (swbz#22273)
- x86-64: Use fxsave/xsave/xsavec in _dl_runtime_resolve (swbz#21265)
- CVE-2017-15670: glob: Fix one-byte overflow (#1504807)
- malloc: Add single-threaded path to _int_free
- locale: Add new locale kab_DZ (swbz#18812)
- locale: Add new locale shn_MM (swbz#13605)

* Fri Oct 20 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-24
- Use make -O to serialize make output
- Auto-sync with upstream branch master,
  commit 63b4baa44e8d22501c433c4093aa3310f91b6aa2:
- sysconf: Fix missing definition of UIO_MAXIOV on Linux (#1504165)
- Install correct bits/long-double.h for MIPS64 (swbz#22322)
- malloc: Fix deadlock in _int_free consistency check
- x86-64: Don't set GLRO(dl_platform) to NULL (swbz#22299)
- math: Add _Float128 function aliases
- locale: Add new locale mjw_IN (swbz#13994)
- aarch64: Rewrite elf_machine_load_address using _DYNAMIC symbol
- powerpc: fix check-before-set in SET_RESTORE_ROUND
- locale: Use U+202F as thousands separators in pl_PL locale (swbz#16777)
- math: Use __f128 to define FLT128_* constants in include/float.h for old GCC
- malloc: Improve malloc initialization sequence (swbz#22159)
- malloc: Use relaxed atomics for malloc have_fastchunks
- locale: New locale ca_ES@valencia (swbz#2522)
- math: Let signbit use the builtin in C++ mode with gcc < 6.x (swbz#22296)
- locale: Place monetary symbol in el_GR, el_CY after the amount (swbz#22019)

* Tue Oct 17 2017 Florian Weimer <fweimer@redhat.com> - 2.26.9000-23
- Switch to .9000 version numbers during development

* Tue Oct 17 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-22
- Auto-sync with upstream branch master,
  commit c38a4bfd596db2be2b9c1f96715bdc833eab760a:
- malloc: Use compat_symbol_reference in libmcheck (swbz#22050)

* Mon Oct 16 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-21
- Auto-sync with upstream branch master,
  commit 596f70134a8f11967c65c1d55a94a3a2718c731d:
- Silence -O3 -Wall warning in malloc/hooks.c with GCC 7 (swbz#22052)
- locale: No warning for non-symbolic character (swbz#22295)
- locale: Allow "" int_curr_Symbol (swbz#22294)
- locale: Fix localedef exit code (swbz#22292)
- nptl: Preserve error in setxid thread broadcast in coredumps (swbz#22153)
- powerpc: Avoid putting floating point values in memory (swbz#22189)
- powerpc: Fix the carry bit on mpn_[add|sub]_n on POWER7 (swbz#22142)
- Support profiling PIE (swbz#22284)

* Wed Oct 11 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-20
- Auto-sync with upstream branch master,
  commit d8425e116cdd954fea0c04c0f406179b5daebbb3:
- nss_files performance issue in multi mode (swbz#22078)
- Ensure C99 and C11 interfaces are available for C++ (swbz#21326)

* Mon Oct 09 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-19
- Move /var/db/Makefile to nss_db (#1498900)
- Auto-sync with upstream branch master,
  commit 645ac9aaf89e3311949828546df6334322f48933:
- openpty: use TIOCGPTPEER to open slave side fd

* Fri Oct 06 2017 Carlos O'Donell <carlos@systemhalted.org> - 2.26.90-18
- Auto-sync with upstream master,
  commit 1e26d35193efbb29239c710a4c46a64708643320.
- malloc: Fix tcache leak after thread destruction (swbz#22111)
- powerpc:  Fix IFUNC for memrchr.
- aarch64: Optimized implementation of memmove for Qualcomm Falkor
- Always do locking when iterating over list of streams (swbz#15142)
- abort: Do not flush stdio streams (swbz#15436)

* Wed Oct 04 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-17
- Move nss_compat to the main glibc package (#1400538)
- Auto-sync with upstream master,
  commit 11c4f5010c58029e73e656d5df4f8f42c9b8e877:
- crypt: Use NSPR header files in addition to NSS header files (#1489339)
- math: Fix yn(n,0) without SVID wrapper (swbz#22244)
- math: Fix log2(0) and log(10) in downward rounding (swbz#22243)
- math: Add C++ versions of iscanonical for ldbl-96, ldbl-128ibm (swbz#22235)
- powerpc: Optimize memrchr for power8
- Hide various internal functions (swbz#18822)

* Sat Sep 30 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-16
- Auto-sync with upstream master,
  commit 1e2bffd05c36a9be30d7092d6593a9e9aa009ada:
- Add IBM858 charset (#1416405)
- Update kernel version in syscall-names.list to 4.13
- Add Linux 4.13 constants to bits/fcntl-linux.h
- Add fcntl sealing interfaces from Linux 3.17 to bits/fcntl-linux.h
- math: New generic powf, log2f, logf
- Fix nearbyint arithmetic moved before feholdexcept (swbz#22225)
- Mark __dso_handle as hidden (swbz#18822)
- Skip PT_DYNAMIC segment with p_filesz == 0 (swbz#22101)
- glob now matches dangling symbolic links (swbz#866, swbz#22183)
- nscd: Release read lock after resetting timeout (swbz#22161)
- Avoid __MATH_TG in C++ mode with -Os for fpclassify (swbz#22146)
- Fix dlclose/exit race (swbz#22180)
- x86: Add SSE4.1 trunc, truncf (swbz#20142)
- Fix atexit/exit race (swbz#14333)
- Use execveat syscall in fexecve (swbz#22134)
- Enable unwind info in libc-start.c and backtrace.c
- powerpc: Avoid misaligned stores in memset
- powerpc: build some IFUNC math functions for libc and libm (swbz#21745)
- Removed redundant data (LC_TIME and LC_MESSAGES) for niu_NZ (swbz#22023)
- Fix LC_TELEPHONE for az_AZ (swbz#22112)
- x86: Add MathVec_Prefer_No_AVX512 to cpu-features (swbz#21967)
- x86: Add x86_64 to x86-64 HWCAP (swbz#22093)
- Finish change from “Bengali” to “Bangla” (swbz#14925)
- posix: fix glob bugs with long login names (swbz#1062)
- posix: Fix getpwnam_r usage (swbz#1062)
- posix: accept inode 0 is a valid inode number (swbz#19971)
- Remove redundant LC_TIME data in om_KE (swbz#22100)
- Remove remaining _HAVE_STRING_ARCH_* definitions (swbz#18858)
- resolv: Fix memory leak with OOM during resolv.conf parsing (swbz#22095)
- Add miq_NI locale for Miskito (swbz#20498)
- Fix bits/math-finite.h exp10 condition (swbz#22082)

* Mon Sep 04 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-15
- Auto-sync with upstream master,
  commit b38042f51430974642616a60afbbf96fd0b98659:
- Implement tmpfile with O_TMPFILE (swbz#21530)
- Obsolete pow10 functions
- math.h: Warn about an already-defined log macro

* Fri Sep 01 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-14
- Build glibc with -O2 (following the upstream default).
- Auto-sync with upstream master,
  commit f4a6be2582b8dfe8adfa68da3dd8decf566b3983:
- malloc: Abort on heap corruption, without a backtrace (swbz#21754)
- getaddrinfo: Return EAI_NODATA for gethostbyname2_r with NO_DATA (swbz#21922)
- getaddrinfo: Fix error handling in gethosts (swbz#21915) (swbz#21922)
- Place $(elf-objpfx)sofini.os last (swbz#22051)
- Various locale fixes (swbz#15332, swbz#22044)

* Wed Aug 30 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-13
- Drop glibc-rh952799.patch, applied upstream (#952799, swbz#22025)
- Auto-sync with upstream master,
  commit 5f9409b787c5758fc277f8d1baf7478b752b775d:
- Various locale fixes (swbz#22022, swbz#22038, swbz#21951, swbz#13805,
  swbz#21971, swbz#21959)
- MIPS/o32: Fix internal_syscall5/6/7 (swbz#21956)
- AArch64: Fix procfs.h not to expose stdint.h types
- iconv_open: Fix heap corruption on gconv_init failure (swbz#22026)
- iconv: Mangle __btowc_fct even without __init_fct (swbz#22025)
- Fix bits/math-finite.h _MSUF_ expansion namespace (swbz#22028)
- Provide a C++ version of iszero that does not use __MATH_TG (swbz#21930)

* Mon Aug 28 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-12
- Auto-sync with upstream master,
  commit 2dba5ce7b8115d6a2789bf279892263621088e74.

* Fri Aug 25 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-11
- Auto-sync with upstream master,
  commit 3d7b66f66cb223e899a7ebc0f4c20f13e711c9e0:
- string/stratcliff.c: Replace int with size_t (swbz#21982)
- Fix tgmath.h handling of complex integers (swbz#21684)

* Thu Aug 24 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-10
- Use an architecture-independent system call list (#1484729)
- Drop glibc-fedora-include-bits-ldbl.patch (#1482105)

* Tue Aug 22 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-9
- Auto-sync with upstream master,
  commit 80f91666fed71fa3dd5eb5618739147cc731bc89.

* Mon Aug 21 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-8
- Auto-sync with upstream master,
  commit a8410a5fc9305c316633a5a3033f3927b759be35:
- Obsolete matherr, _LIB_VERSION, libieee.a.

* Mon Aug 21 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-7
- Auto-sync with upstream master,
  commit 4504783c0f65b7074204c6126c6255ed89d6594e.

* Mon Aug 21 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-6
- Auto-sync with upstream master,
  commit b5889d25e9bf944a89fdd7bcabf3b6c6f6bb6f7c:
- assert: Support types without operator== (int) (#1483005)

* Mon Aug 21 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-5
- Auto-sync with upstream master,
  commit 2585d7b839559e665d5723734862fbe62264b25d:
- Do not use generic selection in C++ mode
- Do not use __builtin_types_compatible_p in C++ mode (#1481205)
- x86-64: Check FMA_Usable in ifunc-mathvec-avx2.h (swbz#21966)
- Various locale fixes (swbz#21750, swbz#21960, swbz#21959, swbz#19852)
- Fix sigval namespace (swbz#21944)
- x86-64: Optimize e_expf with FMA (swbz#21912)
- Adjust glibc-rh827510.patch.

* Wed Aug 16 2017 Tomasz Kłoczko <kloczek@fedoraproject.org> - 2.26-4
- Remove 'Buildroot' tag, 'Group' tag, and '%%clean' section, and don't
  remove the buildroot in '%%install', all per Fedora Packaging Guidelines
  (#1476839)

* Wed Aug 16 2017 Florian Weimer <fweimer@redhat.com> - 2.26.90-3
- Auto-sync with upstream master,
  commit 403143e1df85dadd374f304bd891be0cd7573e3b:
- x86-64: Align L(SP_RANGE)/L(SP_INF_0) to 8 bytes (swbz#21955)
- powerpc: Add values from Linux 4.8 to <elf.h>
- S390: Add new s390 platform z14.
- Various locale fixes (swbz#14925, swbz#20008, swbz#20482, swbz#12349
  swbz#19982, swbz#20756, swbz#20756, swbz#21836, swbz#17563, swbz#16905,
  swbz#21920, swbz#21854)
- NSS: Replace exported NSS lookup functions with stubs (swbz#21962)
- i386: Do not set internal_function
- assert: Suppress pedantic warning caused by statement expression (swbz#21242)
- powerpc: Restrict xssqrtqp operands to Vector Registers (swbz#21941)
- sys/ptrace.h: remove obsolete PTRACE_SEIZE_DEVEL constant (swbz#21928)
- Remove __qaddr_t, __long_double_t
- Fix uc_* namespace (swbz#21457)
- nss: Call __resolv_context_put before early return in get*_r (swbz#21932)
- aarch64: Optimized memcpy for Qualcomm Falkor processor
- manual: Document getcontext uc_stack value on Linux (swbz#759)
- i386: Add <startup.h> (swbz#21913)
- Don't use IFUNC resolver for longjmp or system in libpthread (swbz#21041)
- Fix XPG4.2 bits/sigaction.h namespace (swbz#21899)
- x86-64: Add FMA multiarch functions to libm
- i386: Support static PIE in start.S
- Compile tst-prelink.c without PIE (swbz#21815)
- x86-64: Use _dl_runtime_resolve_opt only with AVX512F (swbz#21871)
- x86: Remove __memset_zero_constant_len_parameter (swbz#21790)

* Wed Aug 16 2017 Florian Weimer <fweimer@redhat.com> - 2.26-2
- Disable multi-arch (IFUNC string functions) on i686 (#1471427)
- Remove nosegneg 32-bit Xen PV support libraries (#1482027)
- Adjust spec file to RPM changes

* Thu Aug 03 2017 Carlos O'Donell <carlos@systemhalted.org> - 2.26-1
- Update to released glibc 2.26.
- Auto-sync with upstream master,
  commit 2aad4b04ad7b17a2e6b0e66d2cb4bc559376617b.
- getaddrinfo: Release resolver context on error in gethosts (swbz#21885)
