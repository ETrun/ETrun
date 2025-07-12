FROM debian:bookworm-slim

LABEL org.opencontainers.image.title="ETrun ET: Legacy Server"
LABEL org.opencontainers.image.description="Docker image running ET: Legacy server with ETrun mod."
LABEL org.opencontainers.image.version="latest"
LABEL org.opencontainers.image.license="MIT"
LABEL org.opencontainers.image.url="https://github.com/ETrun/ETrun"
LABEL org.opencontainers.image.source="https://github.com/ETrun/ETrun/docker"
LABEL maintainer="Nicolas Boutet <amd3002@gmail.com>"

ARG ETRUN_VERSION=latest
ENV ETRUN_VERSION=$ETRUN_VERSION
ENV ZIP_VERSION=${ETRUN_VERSION#v}
ENV ETLEGACY_URL_AMD64=https://www.etlegacy.com/download/file/700
ENV ETLEGACY_URL_ARM64=https://www.etlegacy.com/download/file/710
ENV ETLEGACY_URL_I386=https://www.etlegacy.com/download/file/702
ENV ETDED_PATH=/basepath/etlded

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    wget \
    unzip \
 && useradd -Ms /bin/bash legacy \
 && mkdir -p /basepath/etmain /basepath/etrun /homepath/etrun  \
 && ARCH="$(dpkg --print-architecture)" \
 && if [ "$ARCH" = "amd64" ]; then \
      ETLEGACY_URL="$ETLEGACY_URL_AMD64"; \
      BIN=etlded.x86_64; \
      QAGAME=qagame.mp.x86_64.so; \
    elif [ "$ARCH" = "arm64" ]; then \
      ETLEGACY_URL="$ETLEGACY_URL_ARM64"; \
      BIN=etlded.aarch64; \
      QAGAME=qagame.mp.aarch64.so; \
    elif [ "$ARCH" = "i386" ]; then \
      ETLEGACY_URL="$ETLEGACY_URL_I386"; \
      BIN=etlded.i386; \
      QAGAME=qagame.mp.i386.so; \
    else \
      echo "Unsupported architecture: $ARCH" && exit 1; \
    fi \
 && wget -q "$ETLEGACY_URL" -O etlegacy.tar.gz \
 && tar -xzf etlegacy.tar.gz \
 && cp etlegacy-*/$BIN "$ETDED_PATH" \
 && chmod +x "$ETDED_PATH" \
 && rm -rf etlegacy* etlegacy.tar.gz \
 && wget -q "https://github.com/ETrun/ETrun/releases/download/${ETRUN_VERSION}/ETrun-${ZIP_VERSION}.zip" -O ETrun.zip \
 && unzip -q ETrun.zip \
 && cp ETrun-${ZIP_VERSION}/client/*.pk3 /basepath/etrun/ \
 && cp ETrun-${ZIP_VERSION}/server/$QAGAME /basepath/etrun/$QAGAME \
 && cp -r ETrun-${ZIP_VERSION}/server/custommapscripts /homepath/etrun/ \
 && chown -R legacy:legacy /basepath /homepath \
 && rm -rf ETrun* ETrun.zip \
 && apt-get purge -y --auto-remove wget unzip \
 && rm -rf /var/lib/apt/lists/*

COPY --chmod=755 run.sh /run.sh

USER legacy

WORKDIR /basepath
VOLUME ["/basepath/etmain", "/homepath"]
EXPOSE 27960/UDP
ENTRYPOINT ["/run.sh"]
