# Troubleshooting

If M1 SSH is unavailable:

```bash
docker ps
docker logs gridshield_m1
nc -vz 127.0.0.1 2221
```

If M1 cannot resolve the C2 host:

```bash
docker exec gridshield_m1 getent hosts ftp.gridvault-records.test
docker exec gridshield_m1 ip route get 198.51.100.20
```

Expected:

```text
198.51.100.20 ftp.gridvault-records.test
```

If FTP capture is empty, wait at least one 20-second interval:

```bash
docker exec -it gridshield_m1 bash
timeout 60 sudo tcpdump -i any -nn -s0 -A 'host 198.51.100.20 and port 21'
```

If `vpn_access.pem` fails:

```bash
chmod 600 vpn_access.pem
ssh-keygen -y -P 'GridVPN#2026' -f vpn_access.pem
```

If final PDF filenames are generic, this is expected. Test recovered PDFs with
the wallet and `md5(wallet)` using `qpdf`.
