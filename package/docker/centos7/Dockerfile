FROM centos:7

LABEL maintainer="PiP - Process in Process <pip@ml.riken.jp>"
RUN set -x && \
	yum install -y \
		autoconf \
		automake \
		bison \
		byacc \
		cscope \
		ctags \
		diffstat \
		doxygen \
		elfutils \
		expat-devel \
		flex \
		gcc \
		gcc-c++ \
		gcc-gfortran \
		gdb \
		gettext \
		git \
		indent \
		intltool \
		libselinux-devel \
		libtool \
		make \
		man-db \
		mpfr-devel \
		ncurses-devel \
		patch \
		patchutils \
		rcs \
		readline-devel \
		redhat-rpm-config \
		rpm-build \
		rpm-devel \
		rpm-sign \
		systemtap \
		texinfo \
		which \
		xz-devel \
		zlib-devel \
		&& \
	rm -rf /var/cache/yum/* && \
	yum clean all && \
	rm -f /tmp/ks-script-* /tmp/yum.log && \
	git clone https://github.com/RIKEN-SysSoft/PiP-pip && \
	./PiP-pip/pip-pip --prefix /opt/process-in-process \
		--yes --nosubdir --notest
