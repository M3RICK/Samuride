# What android/package.sh needs, installed once: builds then run offline and start straight away.
# android/docker.sh builds this image the first time.
FROM ubuntu:24.04
RUN apt-get update -qq \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq openjdk-17-jdk-headless cmake git make unzip zip curl \
    && rm -rf /var/lib/apt/lists/*
