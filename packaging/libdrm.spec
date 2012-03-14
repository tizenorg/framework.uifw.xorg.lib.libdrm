Name:       libdrm
Summary:    Userspace interface to kernel DRM services -- runtime
Version:    2.4.27
Release:    1
Group:      libs
License:    TO_FILL
Source0:    libdrm-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(pthread-stubs)
BuildRequires:  pkgconfig(pciaccess)
BuildRequires:  automake
BuildRequires:  libtool


%description
Userspace interface to kernel DRM services -- runtime
 This library implements the userspace interface to the kernel DRM
 services.  DRM stands for "Direct Rendering Manager", which is the
 kernelspace portion of the "Direct Rendering Infrastructure" (DRI).
 The DRI is currently used on Linux to provide hardware-accelerated
 OpenGL drivers.
 .
 This package provides the runtime environment for libdrm..



%package devel
Summary:    Userspace interface to kernel DRM services -- development files
Group:      libdevel
Requires:   libdrm = %{version}-%{release}
Requires:   libdrm-slp
Obsoletes:   linux-libc-dev >= 2.6.29

%description devel
Userspace interface to kernel DRM services -- development files
 This library implements the userspace interface to the kernel DRM
 services.  DRM stands for "Direct Rendering Manager", which is the
 kernelspace portion of the "Direct Rendering Infrastructure" (DRI).
 The DRI is currently used on Linux to provide hardware-accelerated
 OpenGL drivers.
 .
 This package provides the development environment for libdrm..


%package slp
Summary:    Userspace interface to slp-specific kernel DRM services -- runtime
Group:      libs
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description slp
Userspace interface to slp-specific kernel DRM services -- runtime
 This library implements the userspace interface to the intel-specific kernel
 DRM services.  DRM stands for "Direct Rendering Manager", which is the
 kernelspace portion of the "Direct Rendering Infrastructure" (DRI). The DRI is
 currently used on Linux to provide hardware-accelerated OpenGL drivers..


%package -n libkms
Summary:    Userspace interface to kernel DRM buffer management
Group:      libs
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description -n libkms
Userspace interface to kernel DRM buffer management
 This library implements a unified userspace interface to the different buffer
 management interfaces of the kernel DRM hardware drivers..



%prep
%setup -q -n %{name}-%{version}


%build
%autogen.sh --disable-static
%configure --disable-static
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install




%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig






%post slp -p /sbin/ldconfig

%postun slp -p /sbin/ldconfig


%post -n libkms -p /sbin/ldconfig

%postun -n libkms -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libdrm.so.*
%{_libdir}/libdrm_intel.so.*
%{_libdir}/libdrm_radeon.so.*


%files devel
%defattr(-,root,root,-)
%{_includedir}/libdrm/*
%{_includedir}/xf86drmMode.h
%{_includedir}/xf86drm.h

%{_libdir}/libdrm.so
%{_libdir}/libdrm_slp.so
%{_libdir}/libdrm_intel.so
%{_libdir}/libdrm_radeon.so
%{_libdir}/pkgconfig/*

%files slp
%defattr(-,root,root,-)
%{_libdir}/libdrm_slp*.so.*

%files -n libkms
%defattr(-,root,root,-)
%{_includedir}/libkms/libkms.h
%{_libdir}/libkms.so.1*
%{_libdir}/libkms*.so

