#!/usr/bin/env bash
set -e
cp /seed/authorized_keys /home/infraadmin/.ssh/authorized_keys
cp /seed/dev_key /home/infraadmin/.ssh/dev_key
chmod 700 /home/infraadmin/.ssh
chmod 600 /home/infraadmin/.ssh/*
chown -R infraadmin:infraadmin /home/infraadmin/.ssh
cp /seed/openvpn.log /var/log/openvpn.log
cp /seed/auth.log /var/log/auth.log
cp /seed/config.hash /etc/openvpn/config.hash
cp /seed/passphrase_candidates.txt /etc/openvpn/passphrase_candidates.txt
cp /seed/operator.ovpn.enc /etc/openvpn/clients/operator.ovpn.enc
cp /seed/README_RELAY_SUPPORT.md /home/infraadmin/README_RELAY_SUPPORT.md
mkdir -p /home/infraadmin/incident_notes
cp /seed/incident_notes/relay_dropouts.md /home/infraadmin/incident_notes/relay_dropouts.md
chown -R infraadmin:infraadmin /home/infraadmin/README_RELAY_SUPPORT.md /home/infraadmin/incident_notes
tail -f /dev/null
