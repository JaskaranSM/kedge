FROM ubuntu:22.04 

WORKDIR /tmp

RUN apt update -y 

RUN apt install -y \
autotools-dev \
build-essential \
clang \
cmake \
gcc \
git \
libbz2-dev \
libc++-dev \
libclang-dev \
libicu-dev \
libssl-dev \
python3-dev \
libstdc++-12-dev \
libboost-dev \
libboost-all-dev \
wget

RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz && tar -xvf boost_1_76_0.tar.gz
WORKDIR /tmp/boost_1_76_0
RUN ./bootstrap.sh --prefix=/opt/boost
RUN ./b2 --build-dir=build --prefix=/opt/boost --with-program_options --with-json --with-system --with-thread cxxstd=17 link=static runtime-link=static variant=release threading=multi install
WORKDIR /tmp/boost_1_76_0/tools/build
RUN ../../b2 --build-dir=build --prefix=/opt/boost --with=all cxxstd=17 variant=release install

WORKDIR /tmp
RUN wget https://github.com/arvidn/libtorrent/releases/download/v2.0.8/libtorrent-rasterbar-2.0.8.tar.gz && tar -xvf libtorrent-rasterbar-2.0.8.tar.gz
WORKDIR /tmp/libtorrent-rasterbar-2.0.8
RUN /opt/boost/bin/b2 -j"$(nproc)" --prefix=/usr/local optimization=speed cxxstd=17 variant=release crypto=openssl link=static runtime-link=static install

WORKDIR /tmp
RUN git clone --recurse-submodules https://github.com/JaskaranSM/kedge
WORKDIR /tmp/kedge/build
RUN cmake ..
RUN make
RUN mkdir /root/.config

CMD ["./kedge"]