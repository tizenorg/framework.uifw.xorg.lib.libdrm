#sbs-git:slp/pkgs/xorg/lib/libdrm libdrm 2.4.27 4df9ab272d6eac089f89ecd9302d39263541a794
Name:           libdrm
Version:        2.4.35
Release:        1
License:        MIT
Summary:        Userspace interface to kernel DRM services
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  kernel-headers
BuildRequires:  pkgconfig(xorg-macros)
BuildRequires:  pkgconfig(pthread-stubs)

%description
Description: %{summary}

%package devel
Summary:        Userspace interface to kernel DRM services
Group:          Development/Libraries
Requires:       kernel-headers
Requires:       libdrm2
Requires:       libdrm-slp1
Requires:       libkms1

%description devel
Userspace interface to kernel DRM services

%package -n libdrm2
Summary:        Userspace interface to kernel DRM services
Group:          Development/Libraries

%description -n libdrm2
Userspace interface to kernel DRM services

%package slp1
Summary:        Userspace interface to slp-specific kernel DRM services
Group:          Development/Libraries

%description slp1
Userspace interface to slp-specific kernel DRM services

%package -n libkms1
Summary:        Userspace interface to kernel DRM buffer management
Group:          Development/Libraries

%description -n libkms1
Userspace interface to kernel DRM buffer management

%prep
%setup -q


%build
%reconfigure --prefix=%{_prefix} --mandir=%{_prefix}/share/man --infodir=%{_prefix}/share/info \
             --enable-static=yes --enable-udev --enable-libkms --enable-exynos-experimental-api \
             --disable-nouveau --disable-radeon --disable-intel

make %{?_smp_mflags}

%install
%make_install


%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%post -n libdrm2 -p /sbin/ldconfig
%postun -n libdrm2 -p /sbin/ldconfig

%post slp1 -p /sbin/ldconfig
%postun slp1  -p /sbin/ldconfig

%post -n libkms1 -p /sbin/ldconfig
%postun -n libkms1 -p /sbin/ldconfig


%files devel
%dir %{_includedir}/libdrm
%{_includedir}/*
%{_includedir}/exynos/*
%{_libdir}/libdrm.so
%{_libdir}/libdrm_slp.so
%{_libdir}/libdrm_exynos.so
%{_libdir}/libkms.so
%{_libdir}/pkgconfig/*

%files -n libdrm2
%{_libdir}/libdrm.so.*
%{_libdir}/libdrm_exynos.so.*

%files slp1
%{_libdir}/libdrm_slp*.so.*

%files -n libkms1
%{_libdir}/libkms.so.*
