FROM archlinux:base-devel

RUN pacman -Syy --noconfirm && pacman -S pacman-contrib --noconfirm
RUN cp /etc/pacman.d/mirrorlist /etc/pacman.d/mirrorlist.bk
RUN rankmirrors -n 6 /etc/pacman.d/mirrorlist.bk > /etc/pacman.d/mirrorlist
RUN pacman -Syyu --noconfirm
RUN pacman -S git qemu-base qemu-emulators-full sudo \
    risc-v riscv64-linux-gnu-gdb riscv64-elf-binutils riscv64-elf-gcc riscv64-elf-gdb riscv64-elf-newlib \
    --noconfirm

# RUN useradd --no-create-home build && usermod -L build
# RUN echo "build ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
# RUN echo "root ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# RUN git clone https://aur.archlinux.org/riscv-gnu-toolchain-bin.git && chmod a+rwx /riscv-gnu-toolchain-bin
# WORKDIR /riscv-gnu-toolchain-bin
# USER build
# RUN makepkg -si --noconfirm

# USER root
WORKDIR /