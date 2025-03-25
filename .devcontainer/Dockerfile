FROM ubuntu:20.04

RUN sed -i 's@/archive.ubuntu.com/@/mirrors.aliyun.com/@g' /etc/apt/sources.list
RUN sed -i 's@/security.ubuntu.com/@/mirrors.aliyun.com/@g' /etc/apt/sources.list
RUN apt-get update
RUN ln -fs /usr/share/zoneinfo/Etc/UTC /etc/localtime && \
    apt-get update && apt-get install -y \
    git make build-essential gdb-multiarch qemu-system-misc=1:4.2-3ubuntu6 gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu 