Name:           herofand
Version:        %{version}
Release:        1%{?dist}
Summary:        Fan controller daemon for RHEL servers
%global debug_package %{nil}
License:        MIT
URL:            https://github.com/xnoto/herofand
Source0:        %{name}-%{version}.tar.gz
BuildArch:      x86_64

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  systemd-rpm-macros

%description
A lightweight fan controller daemon for RHEL-compatible systems.

%prep
%autosetup

%build
make %{?_smp_mflags} release

%install
make install DESTDIR=%{buildroot} PREFIX=%{_prefix}

%files
%{_bindir}/herofand
%{_unitdir}/herofand.service
%license LICENSE
%doc README.md

%changelog
* Fri Apr 10 2026 github-actions <41898282+github-actions[bot]@users.noreply.github.com> - 0.1.1-1
- Initial RPM packaging
