# an initial note: this container was created to run as a reproducible
# development environment for OpenCL code applications...
# for the author's machine.
# And the user's machine got linux, x11, systemd, and an intel iris graphics card
# although those dependencies are a bit more evident in the run script
# but yeah if you wanna get any graphics out of this container I hope you have
# a way to run docker inside something that can at least act like x11
# 
# (I was planning on using wayland but it doesn't work as well, sorry :()

FROM ubuntu:22.04
LABEL autor="Sto Cazzo"
# god bless documentation
#   https://docs.docker.com/reference/dockerfile/

# I want the build to be non interactive but I'll use this as a development
# container so after the build is done I'd rather it be interactive at runtime
# just not at build time, so ARG instead of ENV for this
ARG DEBIAN_FRONTEND=noninteractive

# directory containing some tools used for development and setup
COPY ./tools/ /root/tools/

# install apt dependencies
RUN apt update -y && apt upgrade -y
# essentials
RUN apt install -y vim emacs git curl
# compiling c in the container
RUN apt install -y clang gcc
# running python in the container
RUN apt install -y python3 python3-pip python-is-python3
# opencl and intel opencl
RUN apt install -y ocl-icd-libopencl1 ocl-icd-opencl-dev\
                   opencl-headers clinfo intel-opencl-icd
# for displaying images and plots from within the container
# this is ideed overkill, but I can't tell how much overkill
# and I can't tell where is it overkill?
# these are taken from the dependencies needed to compile raylib
# except for libxcb-cursor0, which is required by matplotlib
RUN apt install -y libasound2-dev libx11-dev libxrandr-dev libxi-dev \
                   libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev \
                   libxinerama-dev libxkbcommon-dev libxkbcommon-x11-dev \
                   libxcb-cursor0

# https://stackoverflow.com/questions/68036484
RUN apt-get install -y  '^libxcb.*-dev' libx11-xcb-dev libglu1-mesa-dev \
                        libxrender-dev imv


# install pip dependencies
RUN pip3 install numpy
RUN pip3 install pyopencl pyopencl[oclgrind]
RUN pip3 install PyQt5 matplotlib pillow

# the two directories below will be used as bind mounts
#    https://docs.docker.com/engine/storage/bind-mounts/
# I want their contents to be persisted by the host after the container exits
# so I can
#     run a container from the image
#     rm the container
#     keep the data on the host
#     then spin up another container and run that
# so blank slate everytime and this is actually decently reproducible
# 
# bind mounts are more geared towards this usecase than something
# like volumes, which is what I initially considered
RUN mkdir -p /root/workspace/c
RUN mkdir -p /root/workspace/python
