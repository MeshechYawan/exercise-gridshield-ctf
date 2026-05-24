FROM gridshield/common:public
RUN useradd -m -s /bin/bash infraadmin && mkdir -p /home/infraadmin/.ssh /etc/openvpn/clients /var/log \
    && chown -R infraadmin:infraadmin /home/infraadmin
COPY setup.sh /usr/local/bin/m3-setup
RUN chmod +x /usr/local/bin/m3-setup
CMD ["/usr/local/bin/m3-setup"]
