FROM ghcr.io/jaskaransm/kedge-base AS builder

WORKDIR /build

RUN git clone --recurse-submodules https://github.com/JaskaranSM/kedge && \
    mkdir kedge/build && \
    cd kedge/build && \
    cmake .. && \
    make -j"$(nproc)"


FROM ubuntu:22.04

WORKDIR /app

RUN mkdir /root/.config

COPY --from=builder /build/kedge/build/kedge .

CMD [ "./kedge" ]
