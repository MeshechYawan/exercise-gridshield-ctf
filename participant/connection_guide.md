# Connection Guide

The public release does not require a VPN.

Build and start:

```bash
./scripts/build.sh
```

Connect to the starting host:

```bash
ssh investigator@127.0.0.1 -p 2221
```

If your Kali machine is separate from the Docker host, use the Docker host's
host-only IP instead of `127.0.0.1`:

```bash
ssh investigator@192.168.56.11 -p 2221
```

Useful local services after you discover their purpose:

```text
M2 homepage:       http://127.0.0.1:8080/
M2 web console:   http://127.0.0.1:2083/login.php
M3 SSH:           ssh -p 2223 infraadmin@127.0.0.1
M4 SSH:           ssh -p 2224 devuser@127.0.0.1
```

For a separate Kali VM, replace `127.0.0.1` in those service URLs and SSH
commands with the challenge host IP, for example `192.168.56.11`.

Inside M1, the malware still communicates with:

```text
ftp.gridvault-records.test -> 198.51.100.20
```

Capture live malware traffic from M1, not from the Docker host.
