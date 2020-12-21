FROM centos:8

LABEL maintainer="PiP - Process in Process <pip@ml.riken.jp>"
RUN set -x && \
	yum install -y \
		bison \
		byacc \
		elfutils \
		flex \
		gcc \
		gcc-c++ \
		gcc-gfortran \
		gdb \
		gettext \
		git \
		intltool \
		libtool \
		make \
		man-db \
		patch \
		patchutils \
		redhat-rpm-config \
		rpm-build \
		rpm-sign \
		systemtap && \
	rpm -Uvh https://git.sys.r-ccs.riken.jp/PiP/package/el/8/noarch/pip-1/pip-release-1-0.noarch.rpm && \
	yum install -y pip-glibc && \
	yum install -y --setopt='tsflags=' pip && \
	yum install -y pip-gdb && \
	rm -rf /var/cache/yum/* && \
	yum clean all && \
	rm -f /tmp/ks-script-* /tmp/yum.log
