%define gpgcheck	1

Name: pip-release
Version: 1
Release: 0

Summary: PiP release yum repository settings
Group: System Environment/Libraries
License: BSD
URL: https://github.com/RIKEN-SysSoft/PiP/
Vendor: RIKEN System Software Team
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot

Source0: pip.repo
%if %{gpgcheck}
Source1: RPM-GPG-KEY-PiP
%endif

BuildArch: noarch

%description
Yum repository repository settings for PiP - Process in Process

%prep

%build

%install
rm -rf $RPM_BUILD_ROOT

# pip.repo
install -dm 755 $RPM_BUILD_ROOT%{_sysconfdir}/yum.repos.d
install -pm 644 %{SOURCE0} $RPM_BUILD_ROOT%{_sysconfdir}/yum.repos.d/

# RPM-GPG-KEY-PiP
%if %{gpgcheck}
install -dm 755 $RPM_BUILD_ROOT%{_sysconfdir}/pki/rpm-gpg
install -pm 644 %{SOURCE1} $RPM_BUILD_ROOT%{_sysconfdir}/pki/rpm-gpg/
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%config(noreplace) /etc/yum.repos.d/*
%if %{gpgcheck}
/etc/pki/rpm-gpg/*
%endif
