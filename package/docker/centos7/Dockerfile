FROM centos:7.3.1611

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
		flex \
		gcc \
		gcc-c++ \
		gcc-gfortran \
		gdb \
		gettext \
		git \
		indent \
		intltool \
		libtool \
		make \
		man-db \
		patch \
		patchutils \
		rcs \
		redhat-rpm-config \
		rpm-build \
		rpm-sign \
		systemtap && \
	rpm -Uvh https://git.sys.r-ccs.riken.jp/PiP/package/el/7/noarch/pip-1/pip-release-2-0.noarch.rpm && \
	yum install -y pip-glibc && \
	yum install -y --setopt='tsflags=' pip pip-debuginfo && \
	yum install -y pip-gdb && \
	rm -rf /var/cache/yum/* && \
	yum clean all && \
	rm -f /tmp/ks-script-* /tmp/yum.log
