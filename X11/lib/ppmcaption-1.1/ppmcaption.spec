%define	name	ppmcaption
%define	version	1.1
%define	release	1
%define	serial	1
%define	prefix	/usr

Summary:	filter for adding text captions to PPM images
Name:		%{name}
Version:	%{version}
Release:	%{release}
Serial:		%{serial}
Group:		X11/Utilities
Copyright:	BSD
URL:		http://www.jwz.org/ppmcaption/
Vendor:		Jamie Zawinski <jwz@jwz.org>
Source:		%{name}-%{version}.tar.gz
Buildroot:	/var/tmp/%{name}-%{version}-root

%description
This program adds text to a PPM, PGM, or PBM image.  Multiple blocks of
text can be placed on the image, with varying fonts, font sizes,
colors, and transparency.
%prep
%setup -q
%build

./configure --prefix=%{prefix} \
            --with-builtin="ncenB24.bdf -scale 0.5 -blur 5"
make

%install

mkdir -p $RPM_BUILD_ROOT%{prefix}/bin
mkdir -p $RPM_BUILD_ROOT%{prefix}/man/man1
make install install_prefix=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)

        %{prefix}/bin/*
        %{prefix}/man/man1/*
