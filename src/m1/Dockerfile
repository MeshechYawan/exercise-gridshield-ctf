FROM gridshield/common:public
RUN useradd -m -s /bin/bash investigator && echo 'investigator:Inv3st!gate2024' | chpasswd \
    && echo 'investigator ALL=(root) NOPASSWD: /usr/bin/tcpdump' > /etc/sudoers.d/investigator-tcpdump \
    && chmod 440 /etc/sudoers.d/investigator-tcpdump
COPY wupdate.service /etc/systemd/system/wupdate.service
COPY wupdate.c /tmp/wupdate.c
COPY wupd_cred_helper.c /tmp/wupd_cred_helper.c
COPY m1-noise.sh /usr/local/bin/m1-noise
COPY m1-loop.sh /usr/local/bin/m1-loop
RUN gcc -O2 -Wall -Wextra -o /usr/bin/wupdate /tmp/wupdate.c \
    && gcc -O2 -Wall -Wextra -o /usr/local/lib/wupd_cred_helper /tmp/wupd_cred_helper.c \
    && strip /usr/bin/wupdate \
    && strip /usr/local/lib/wupd_cred_helper \
    && rm /tmp/wupdate.c /tmp/wupd_cred_helper.c \
    && chmod 0755 /usr/bin/wupdate /usr/local/bin/m1-loop /usr/local/bin/m1-noise \
    && chmod 0711 /usr/local/lib/wupd_cred_helper
CMD ["/usr/local/bin/m1-loop"]
