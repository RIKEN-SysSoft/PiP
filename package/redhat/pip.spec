# How to build this RPM:
#
#	env QA_RPATHS=3 rpmbuild -bb pip.spec
#

%define glibc_libdir	/opt/pip/lib
%define libpip_version	0
%define docdir		/usr/share/doc/%{name}-%{version}

Name: pip
Version: 1.1.0
Release: 1%{?dist}
Epoch: 1
Source: %{name}-%{version}.tar.gz

Summary: PiP - Process in Process
Group: System Environment/Libraries
License: BSD
URL: https://github.com/RIKEN-SysSoft/PiP/
Vendor: RIKEN System Software Team
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildRequires: pip-glibc

%description
PiP is a user-level library to have the best of the both worlds of
multi-process and multi-thread parallel execution models.
PiP allows a process to create sub-processes into the same virtual
address space where the parent process runs.
The parent process and sub-processes share the same address space,
however, each process has its own variable set.  So, each process runs
independently from the other process. If some or all processes agree,
then data own by a process can be accessed by the other processes.
Those processes share the same address space, just like pthreads, and
each process has its own variables like a process. The parent process
is called PiP process and a sub-process are called a PiP task.

%prep

%setup -n %{name}-%{version}

%build
./configure --prefix=%{_prefix} --with-glibc-libdir=%{glibc_libdir}
make default_libdir=%{_libdir}

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

make DESTDIR="$RPM_BUILD_ROOT" \
	default_docdir=%{docdir} \
	default_libdir=%{_libdir} \
	default_mandir=%{_mandir} install

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%attr(0755,root,root) %{_bindir}/pipcc
%attr(0755,root,root) %{_bindir}/piprun
%attr(0755,root,root) %{_bindir}/pipmap
%attr(0644,root,root) %{_mandir}/man1*/[ABD-Zabcd-z]*
%attr(0644,root,root) %{_mandir}/man3*/[ABD-Zabcd-z]*
%attr(0644,root,root) %{docdir}/*
# libs
%defattr(-,root,root)
%attr(0755,root,root) %{_bindir}/piplnlibs
%attr(0755,root,root) %{_libdir}/libpip.so.%{libpip_version}
%attr(0755,root,root) %{_libdir}/pip_preload.so
# devel
%{_prefix}/include
%{_libdir}/libpip.so

%post
%{_bindir}/piplnlibs -rs

%preun
%{_bindir}/piplnlibs -Rs
