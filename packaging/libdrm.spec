#sbs-git:slp/pkgs/xorg/lib/libdrm libdrm 2.4.27 4df9ab272d6eac089f89ecd9302d39263541a794
Name:           libdrm
Version:        2.4.27
Release:        4
License:        MIT
Summary:        Userspace interface to kernel DRM services
Group:          System/Libraries
Source0:        %{name}-%{version}.tar.gz
Source1001:     libdrm.manifest
BuildRequires:  kernel-headers
BuildRequires:  pkgconfig(pciaccess)
BuildRequires:  pkgconfig(pthread-stubs)
BuildRequires:  pkgconfig(xorg-macros)

%description
Description: %{summary}

%package devel
Summary:        Userspace interface to kernel DRM services
Group:          Development/Libraries
Requires:       kernel-headers
Requires:       libdrm
Requires:       libdrm-intel
Requires:       libdrm-slp1
Requires:       libkms

%description devel
Userspace interface to kernel DRM services

%package slp1
Summary:        Userspace interface to slp-specific kernel DRM services
Group:          Development/Libraries

%description slp1
Userspace interface to slp-specific kernel DRM services

%package -n libkms
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
cp %{SOURCE1001} .
%reconfigure --prefix=%{_prefix} --mandir=%{_mandir} --infodir=%{_infodir} \
             --enable-static=yes --enable-udev --enable-libkms \
             --disable-nouveau-experimental-api --disable-radeon

make %{?_smp_mflags}

%install
%make_install


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%post slp1 -p /sbin/ldconfig

%postun slp1  -p /sbin/ldconfig

%post -n libkms -p /sbin/ldconfig

%postun -n libkms -p /sbin/ldconfig

%post intel -p /sbin/ldconfig

%postun intel -p /sbin/ldconfig

%files
%manifest libdrm.manifest
%{_libdir}/libdrm.so.*

%files devel
%manifest libdrm.manifest
%dir %{_includedir}/libdrm
%{_includedir}/*
%{_libdir}/libdrm.so
%{_libdir}/libdrm_slp.so
%{_libdir}/libdrm_intel.so
%{_libdir}/libkms.so
%{_libdir}/pkgconfig/*


%files slp1
%manifest libdrm.manifest
%{_libdir}/libdrm_slp*.so.*

%files -n libkms
%manifest libdrm.manifest
%{_libdir}/libkms.so.*

%files intel
%manifest libdrm.manifest
%{_libdir}/libdrm_intel.so.*
