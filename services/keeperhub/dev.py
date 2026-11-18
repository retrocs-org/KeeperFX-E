#!/usr/bin/env python3
"""Start an isolated loopback development service; retain its private local key."""
import argparse,os,secrets
from pathlib import Path
import hub
p=argparse.ArgumentParser();p.add_argument('--state',type=Path,default=Path('.local/keeperhub'));p.add_argument('--port',type=int,default=8765);p.add_argument('--stop',action='store_true');a=p.parse_args();os.umask(0o077);a.state.mkdir(parents=True,exist_ok=True,mode=0o700);stopfile=a.state/'stop.request'
if a.stop:stopfile.touch();print('Local community service stop requested.');raise SystemExit(0)
stopfile.unlink(missing_ok=True)
keyfile=a.state/'admin.key'
if not keyfile.exists():keyfile.write_text(secrets.token_urlsafe(32));keyfile.chmod(0o600)
relay=hub.Relay();service=hub.Hub(a.state/'community.sqlite3',relay,keyfile.read_text().strip());server=hub.serve(service,port=a.port);(a.state/'local-url.txt').write_text(server.origin);print('KeeperHub ready at '+server.origin,flush=True)
try:
    server.timeout=.5
    while not stopfile.exists():server.handle_request()
except KeyboardInterrupt:pass
finally:server.server_close();relay.close()
