FROM debian:stretch-slim

RUN apt-get update \
    && apt-get install --no-install-recommends -y \
    autoconf \
    automake \
    build-essential \
    cmake \
    gcc-multilib \
    libtool-bin \
    m4 \
    zip \
    && rm -r /var/lib/apt/lists/*

VOLUME /code
WORKDIR /code
