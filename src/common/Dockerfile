FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive TZ=Asia/Colombo
RUN apt-get update && apt-get install -y --no-install-recommends \
    openssh-server sudo curl python3 python3-pip openssl zip unzip \
    tcpdump iproute2 iputils-ping netcat-openbsd dnsutils ca-certificates binutils \
    gcc libc6-dev gdb procps file qpdf testdisk e2fsprogs \
    && pip3 install --no-cache-dir pyftpdlib \
    && rm -rf /var/lib/apt/lists/*
RUN mkdir -p /run/sshd && sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config
COPY entrypoint.sh /usr/local/bin/grid-entrypoint
RUN chmod +x /usr/local/bin/grid-entrypoint
ENTRYPOINT ["/usr/local/bin/grid-entrypoint"]
