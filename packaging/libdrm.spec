Name:           libdrm
Version:        2.4.35
Release:        20
License:        MIT
Summary:        Userspace interface to kernel DRM services
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(pthread-stubs)

%description
Description: %{summary}

%package tools
Summary:	Diagnostic utilities for DRI and DRM
Group:          Graphics & UI Framework/Utilities
Obsoletes:      libdrm < %version-%release
Provides:       libdrm = %version-%release

%description tools
Diagnoistic tools to run a test for DRI and DRM

%package devel
Summary:        Userspace interface to kernel DRM services
Group:          Development/Libraries
Requires: libdrm2 = %{version}-%{release}
Requires: libkms1

%description devel
Userspace interface to kernel DRM services

%package -n libdrm2
Summary:        Userspace interface to kernel DRM services
Group:          Development/Libraries

%description -n libdrm2
Userspace interface to kernel DRM services

%package -n libkms1
Summary:        Userspace interface to kernel DRM buffer management
Group:          Development/Libraries

%description -n libkms1
Userspace interface to kernel DRM buffer management

%prep
%setup -q

mkdir -p ./m4

%build
%reconfigure --prefix=%{_prefix} --mandir=%{_prefix}/share/man --infodir=%{_prefix}/share/info \
             --enable-static=yes --enable-udev --enable-libkms --enable-sdp-experimental-api --enable-exynos-experimental-api \
             --disable-nouveau --disable-radeon --disable-intel \
             CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS} -Wl,--hash-style=both -Wl,--as-needed"

make %{?_smp_mflags}
make %{?_smp_mflags} -C tests dristat drmstat

%install
%make_install
make -C tests/modeprint install DESTDIR=$RPM_BUILD_ROOT
make -C tests/modetest install DESTDIR=$RPM_BUILD_ROOT
%{__mkdir} -p $RPM_BUILD_ROOT/usr/bin
%{__install}  \
	tests/.libs/dristat \
        tests/.libs/drmstat \
	tests/modeprint/.libs/modeprint \
	tests/modetest/.libs/modetest $RPM_BUILD_ROOT/usr/bin

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%post -n libdrm2 -p /sbin/ldconfig
%postun -n libdrm2 -p /sbin/ldconfig

%post -n libkms1 -p /sbin/ldconfig
%postun -n libkms1 -p /sbin/ldconfig


%files tools
%_bindir/dristat
%_bindir/drmstat
%_bindir/modeprint
%_bindir/modetest

%files devel
%dir %{_includedir}/libdrm
%{_includedir}/*
%{_includedir}/exynos/*
%{_libdir}/libdrm.so
%{_libdir}/libdrm_exynos.so
%{_libdir}/libkms.so
%{_libdir}/libdrm_vigs.so
%{_libdir}/libdrm_sdp.so
%{_libdir}/pkgconfig/*

%files -n libdrm2
%{_libdir}/libdrm.so.*
%{_libdir}/libdrm_exynos.so.*
%{_libdir}/libdrm_vigs.so.*
%{_libdir}/libdrm_sdp.so.*

%files -n libkms1
%{_libdir}/libkms.so.*
