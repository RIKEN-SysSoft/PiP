FROM centos:7.3.1611

LABEL maintainer="PiP - Process in Process <pip@ml.riken.jp>"
RUN set -x && \
	rpm -Uvh https://git.sys.r-ccs.riken.jp/PiP/package/el/7/noarch/pip-1/pip-release-1-0.noarch.rpm && \
	yum install -y pip-glibc && \
	yum install -y pip pip-debuginfo && \
	yum install -y pip-gdb && \
	rm -rf /var/cache/yum/* && \
	yum clean all
