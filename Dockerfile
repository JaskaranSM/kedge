FROM ghcr.io/jaskaransm/kedge-base AS builder

WORKDIR /build

RUN git clone --recurse-submodules https://github.com/JaskaranSM/kedge && \
    mkdir kedge/build && \
    cd kedge/build && \
    cmake .. && \
    make -j"$(nproc)"


FROM ubuntu:22.04

WORKDIR /app
RUN apt-get update && apt-get install ca-certificates bash curl coreutils tzdata locales -y

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN mkdir /root/.config

COPY --from=builder /build/kedge/build/kedge .

CMD [ "./kedge" ]
