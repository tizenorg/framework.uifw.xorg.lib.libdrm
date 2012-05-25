#sbs-git:slp/pkgs/xorg/lib/libdrm libdrm 2.4.27 4df9ab272d6eac089f89ecd9302d39263541a794
Name:           libdrm
Version:        2.4.27
Release:        4
License:        MIT
Summary:        Userspace interface to kernel DRM services
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  kernel-headers
BuildRequires:  pkgconfig(xorg-macros)
BuildRequires:  pkgconfig(pthread-stubs)
BuildRequires:  pkgconfig(pciaccess)

%description
Description: %{summary}

%package devel
Summary:        Userspace interface to kernel DRM services
Group:          Development/Libraries
Requires:       kernel-headers
Requires:       libdrm2
Requires:       libdrm-slp1
Requires:       libkms1
Requires:       libdrm-intel

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

%package intel
Summary:        Userspace interface to intel graphics kernel DRM buffer management
Group:          Development/Libraries

%description intel
Userspace interface to intel graphics kernel DRM buffer management

%prep
%setup -q


%build
%reconfigure --prefix=%{_prefix} --mandir=%{_prefix}/share/man --infodir=%{_prefix}/share/info \
             --enable-static=yes --enable-udev --enable-libkms \
             --disable-nouveau-experimental-api --disable-radeon

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

%post intel -p /sbin/ldconfig
%postun intel -p /sbin/ldconfig

%files devel
%dir %{_includedir}/libdrm
%{_includedir}/*
%{_libdir}/libdrm.so
%{_libdir}/libdrm_slp.so
%{_libdir}/libdrm_intel.so
%{_libdir}/libkms.so
%{_libdir}/pkgconfig/*

%files -n libdrm2
%{_libdir}/libdrm.so.*

%files slp1
%{_libdir}/libdrm_slp*.so.*

%files -n libkms1
%{_libdir}/libkms.so.*

%files intel
%{_libdir}/libdrm_intel.so.*
