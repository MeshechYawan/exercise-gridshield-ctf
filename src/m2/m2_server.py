import html, http.server, socketserver, sqlite3, subprocess, threading, urllib.parse
from pathlib import Path
from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer

FTP_ROOT = Path("/srv/ftp")
INTERNAL = Path("/internal/secure_mgmt")
SESSION = "gridshield-nsadmin-session"

def render_dashboard(search="", rows=None, error=None):
    value = html.escape(search)
    body = [
        "<h1>NovaSec Agent Activity</h1>",
        "<form method='get' action='/secure_mgmt/dashboard.php'>",
        f"<input name='search' value='{value}' placeholder='agent id'>",
        "<button>Search</button>",
        "</form>",
        "<p>Recent agent marker: ep-009</p>",
    ]
    if error:
        body.append(f"<pre>SQL error in agent activity query: {html.escape(error)}</pre>")
    elif rows is not None:
        row_map = {agent_id: (host, route, notes) for agent_id, host, route, notes in rows}
        if "relay-01" in row_map:
            relay_host, relay_route, relay_notes = row_map["relay-01"]
            body.append("<pre>")
            body.append("Real next host:        M3\n")
            body.append("M3 internal IP:        10.1.1.10\n")
            body.append(f"M3 simulated public:   {html.escape(relay_route.split(' / ')[-1])}\n")
            body.append("Role:                  Blue Transit VPN relay\n")
            body.append("SSH user:              infraadmin\n")
            body.append("SSH private key path:  /internal/secure_mgmt/keys/vpn_access.pem\n")
            body.append("Key passphrase:        GridVPN#2026\n\n")
            body.append("CTF{SQLI_AGENT_ACTIVITY_DUMP_EP009}")
            body.append("</pre>")
        elif rows:
            body.append("<table><tr><th>Agent</th><th>Host</th><th>Route</th><th>Notes</th></tr>")
            for agent_id, host, route, notes in rows:
                body.append(
                    "<tr>"
                    f"<td>{html.escape(agent_id)}</td>"
                    f"<td>{html.escape(host)}</td>"
                    f"<td>{html.escape(route)}</td>"
                    f"<td>{html.escape(notes)}</td>"
                    "</tr>"
                )
            body.append("</table>")
        else:
            body.append("<p>No matching agent activity.</p>")
    return "".join(body)

def query_agent_activity(search):
    conn = sqlite3.connect(":memory:")
    conn.execute(
        "CREATE TABLE agent_activity (agent_id TEXT, host TEXT, route TEXT, notes TEXT)"
    )
    conn.executemany(
        "INSERT INTO agent_activity VALUES (?, ?, ?, ?)",
        [
            (
                "ep-009",
                "cbfs01",
                "active C2 heartbeat",
                "dataset=supervision,payments,hr; status=active",
            ),
            (
                "relay-01",
                "blue-transit-relay",
                "10.1.1.10 / 192.0.2.10",
                "ssh_user=infraadmin; key_path=/internal/secure_mgmt/keys/vpn_access.pem; key_passphrase=GridVPN#2026",
            ),
        ],
    )
    sql = f"SELECT agent_id, host, route, notes FROM agent_activity WHERE agent_id = '{search}'"
    rows = conn.execute(sql).fetchall()
    conn.close()
    return rows

class Web(http.server.BaseHTTPRequestHandler):
    def log_message(self, fmt, *args): pass
    def send(self, code, body, ctype="text/html", headers=None):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        for key, value in (headers or {}).items():
            self.send_header(key, value)
        self.end_headers()
        self.wfile.write(body.encode())
    def authed(self):
        cookie = self.headers.get("Cookie", "")
        return f"gs_session={SESSION}" in cookie
    def require_auth(self):
        if self.authed():
            return True
        self.send(403, "login required: /login.php", "text/plain")
        return False
    def do_GET(self):
        path = urllib.parse.urlparse(self.path).path
        qs = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
        if path in ["/", "/index.html"]:
            self.send(200, "<h1>NovaSec Hosting Solutions</h1><p>Reliable web services and managed hosting.</p>")
        elif path == "/login.php":
            self.send(200, "<form method='post'><input name='username'><input name='password' type='password'><button>Login</button></form>")
        elif path.startswith("/db_admin"):
            self.send(200, "<h1>phpMyAdmin</h1><p>Internal DB administration.</p>")
        elif path.startswith("/secure_mgmt/dashboard.php"):
            if not self.require_auth():
                return
            search = qs.get("search", [""])[0]
            if not search:
                self.send(200, render_dashboard())
            else:
                try:
                    self.send(200, render_dashboard(search, query_agent_activity(search)))
                except sqlite3.Error as exc:
                    self.send(500, render_dashboard(search, error=str(exc)))
        elif path.startswith("/filemanager.php"):
            if not self.require_auth():
                return
            requested = qs.get("path", [""])[0]
            if requested in ["", "/internal/secure_mgmt/keys/"]:
                self.send(200, "<h1>Internal Tools</h1><a href='/filemanager.php?path=/internal/secure_mgmt/keys/vpn_access.pem'>vpn_access.pem</a>")
            elif requested == "/internal/secure_mgmt/keys/vpn_access.pem":
                key = (INTERNAL / "keys/vpn_access.pem").read_text()
                self.send(200, key, "text/plain")
            else:
                self.send(404, "File not found", "text/plain")
        elif path.rstrip("/") == "/secure_mgmt":
            self.send(200, "<h1>NovaSec Secure Management</h1><p>Internal management landing.</p><p>CTF{DIRB_FOUND_SECURE_MGMT_AND_DB_ADMIN}</p><a href='/login.php'>login</a> <a href='/secure_mgmt/dashboard.php'>dashboard</a>")
        elif path.startswith("/secure_mgmt"):
            self.send(404, "Not Found")
        elif path.startswith("/backup"):
            self.send(403, "Forbidden")
        elif path.startswith("/api"):
            self.send(405, "Method Not Allowed")
        else:
            self.send(404, "Not Found")
    def do_POST(self):
        path = urllib.parse.urlparse(self.path).path
        if path != "/login.php":
            self.send(404, "Not Found", "text/plain")
            return
        size = int(self.headers.get("Content-Length", "0") or 0)
        body = self.rfile.read(size).decode(errors="ignore")
        data = urllib.parse.parse_qs(body)
        user = data.get("username", [""])[0]
        pw = data.get("password", [""])[0]
        if user == "nsadmin" and pw == "N0v@S3c!2024":
            self.send(200, "login ok: nsadmin", "text/plain", {"Set-Cookie": f"gs_session={SESSION}; HttpOnly; Path=/"})
        else:
            self.send(403, "login failed", "text/plain")

def ftp():
    FTP_ROOT.mkdir(parents=True, exist_ok=True)
    (FTP_ROOT / "incoming").mkdir(exist_ok=True)
    auth = DummyAuthorizer()
    auth.add_user("sitebackup", "Str0ng#Bkp2024", str(FTP_ROOT), perm="elradfmwMT")
    handler = FTPHandler
    handler.authorizer = auth
    handler.masquerade_address = "198.51.100.20"
    handler.passive_ports = range(30000, 30010)
    FTPServer(("0.0.0.0", 21), handler).serve_forever()
def web(port):
    socketserver.TCPServer.allow_reuse_address = True
    socketserver.TCPServer(("0.0.0.0", port), Web).serve_forever()
threading.Thread(target=ftp, daemon=True).start()
threading.Thread(target=web, args=(80,), daemon=True).start()
threading.Thread(target=web, args=(2083,), daemon=True).start()
subprocess.run(["tail", "-f", "/dev/null"])
