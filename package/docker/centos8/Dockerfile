FROM centos:8

LABEL maintainer="PiP - Process in Process <pip@ml.riken.jp>"
RUN set -x && \
	yum -y install yum-utils &&
	yum-config-manager --enable powertools &&
	yum install -y \
		bison \
		byacc \
		elfutils \
		expat-devel \
		flex \
		gcc \
		gcc-c++ \
		gcc-gfortran \
		gdb \
		gettext \
		git \
		intltool \
		libselinux-devel \
		libtool \
		make \
		man-db \
		mpfr-devel \
		ncurses-devel \
		patch \
		patchutils \
		python3 \
		readline-devel \
		redhat-rpm-config \
		rpm-build \
		rpm-devel \
		rpm-sign \
		systemtap \
		systemtap-sdt-devel \
		texinfo \
		xz-devel \
		zlib-devel \
		&& \
	rm -rf /var/cache/yum/* && \
	yum clean all && \
	rm -f /tmp/ks-script-* /tmp/yum.log && \
	git clone https://github.com/RIKEN-SysSoft/PiP-pip && \
	python3 ./PiP-pip/pip-pip --prefix /opt/process-in-process \
		--yes --nosubdir --notest
