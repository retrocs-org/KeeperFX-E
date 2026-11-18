# KeeperFX-E community services

The optional service adds an in-game server browser, browser-paired accounts,
website profiles/statistics, friends, events, verified tournament brackets,
cooperative challenge completions, replay sharing and buffered live spectating.
LAN, hostname/IP direct connections and the original public lobby service remain
independent. No account or community service is required for LAN/direct play.

## Local development

Python 3.9+ with SQLite is sufficient; there are no third-party Python packages.
From the repository root:

```sh
python3 services/keeperhub/dev.py
```

If that port is already in use, pass `--port 8766` and set the same address in
the game’s Community Service settings and dedicated host configuration.
Open the printed localhost address and create an account if wanted. The service stores its
SQLite database and private administrator key in `.local/keeperhub/`. Neither
belongs in a release or source control. Stop the service with Ctrl-C, or use
`python3 services/keeperhub/dev.py --stop` from another terminal.

In the game choose More Options → Network → Server Browser / Accounts.

- **Internet** lists the selected community service. **Original Lobbies** opens
  the upstream KeeperFX lobby service separately.
- **LAN** discovers local games. **Favourites** and **Recent** retain addresses
  across restarts and resolve refreshed relay identities from the directory.
- Left/right changes browser tabs. Up/down and confirm reach server rows,
  paging, refresh, hosting, accounts, search, sorting and compatibility filters.
- Server details provide direct joining, relay joining, password entry,
  favourites, address copying and spectating. A failed public direct join tries
  the configured service relay once. The relay needs the host's outbound
  registrations; there is no service dependency for ordinary direct joining.
- Sign In opens the local website and displays an eight-character pairing code.
  Approve only the code on your own game. Game session credentials are stored
  separately with mode 0600; changing service addresses clears them.
- Community pages show events, friends, match history, challenges and replay
  listings. Account creation, friend requests, event management, replay upload
  and tournament result entry are available on the website.
- Service settings allow loopback HTTP for development and require HTTPS for
  remote services. TLS certificate verification remains enabled.

Latency queries echo a bounded nonce without creating an ENet player. Numeric
IPv4/IPv6 addresses can be probed; unresolved hostnames show unknown ping.
Engine filtering is an initial check. The existing preflight checks selected
map/campaign bytes and mods before a normal host starts.

## Headless hosting

The current lockstep engine makes the host a player. This implementation uses
**one synchronized AI host and up to three humans**, with no visible window or
sound. It does not claim four human slots or a separate authoritative simulation
protocol. Spectators replay a buffered input stream and consume no player slot.
New KeeperFX-E peers advertise the `+hub1` readiness capability; older peers do not
automatically pass this headless-host preflight. Normal LAN/direct and original
lobby paths remain available.

Prepare a separate writable profile using an installed app and your own imported
Dungeon Keeper files, for example:

```sh
KFX_USER_DATA_DIR="$PWD/.local/dedicated-user" \
  "/Applications/KeeperFX-E.app/Contents/MacOS/KeeperFXE" \
  --import "$PWD/.local/keeperfx"
KFX_USER_DATA_DIR="$PWD/.local/dedicated-user" \
  "/Applications/KeeperFX-E.app/Contents/MacOS/KeeperFXE" --prepare-only
```

Copy `examples/dedicated-local.json` to an ignored local file and adjust paths.
Choose maps with enough player positions. The example runs Classic map 52 with
one human and the AI host. The engine binary and GameData must have matching
assets. The supervisor writes only into the dedicated profile, never the app.

Start the supervisor without printing or copying its administrator key:

```sh
python3 services/keeperhub/run-local-host.py .local/dedicated.json
```

`run-local-host.py` reads `.local/keeperhub/admin.key` into the child environment.
For a different service, set `KFX_HUB_ADMIN_KEY` through your own secret manager
and invoke `dedicated.py CONFIG` directly. An optional `KFX_SERVER_PASSWORD`
sets the game lobby password. Administrator and account tokens are never passed
in command-line arguments.

The supervisor supports JSON commands on stdin: `{"command":"status"}`,
`{"command":"next"}` and `{"command":"stop"}`. It rotates maps after a match,
archives replay files under the dedicated profile, and stops its own child on
shutdown. From another terminal use `run-local-host.py CONFIG --stop`, `--next`
or `--status` to control only that configured host. Spectator publishing is explicitly enabled in its configuration.

## Results and community features

Guest and player-reported games remain **casual**. Consistent player reports can
be **confirmed**, which is still not a verified statistic. Only a dedicated host
registered with the service administrator key may report **verified** results.
Account join tickets are scoped to a server, expire, and bind to actual ENet
peers; players do not send their account bearer tokens through game chat.

The engine records terminal outcomes separately from early departures. The
supervisor cannot count an early disconnect as a verified victory. The trust
boundary is the dedicated-server operator: this is not a claim of cheat-proof
networking. Accounts are private by default; public profile URLs are
`/keeper/ACCOUNT_ID` and show verified statistics and earned achievements.

Tournament organizers can start single-elimination brackets with 2–32 entrants.
Byes advance automatically. Later rounds advance only using a service-verified
match with the correct pair of authenticated players; a match cannot be reused.
Challenge completions require a verified win on the specified map. Cooperative
hosts may report multiple allied winners when explicitly configured.

Opted-in headless hosts publish bounded append-only replay streams. The game
checks the engine version, buffers data, and waits when a stream has not caught
up. Spectators cannot issue live game actions. Finished streams become replay
downloads with SHA-256 checksums; account holders may also upload completed-match
replays up to 2 MiB. Live streams are limited to 16 MiB in this first local
implementation. This is buffered replay spectating, not free-camera rollback or
mid-match player joining.

## Service boundaries

The directory is HTTP/JSON; gameplay remains ENet/UDP. Relay channels require
separate host/client tokens, accept traffic only from authenticated endpoints,
expire when idle and have capacity/bandwidth limits. The relay is transport,
not a game simulator. The service uses password hashing, hashed session tokens,
HttpOnly/SameSite cookies, CSRF and origin/Host checks, bounded bodies and rate
limits. It never distributes original Dungeon Keeper assets.

This is a local-tested implementation. Before public deployment it still needs
an operated HTTPS endpoint, public relay UDP port range, backups, monitoring,
account recovery/email policy and testing across real routers/CGNAT. No public
infrastructure, accounts or ranking data are created by the build.

## Verification

```sh
python3 macos/tests/test-keeperhub.py
python3 macos/tests/run-ingame-options-tests.py
python3 macos/tests/run-readiness-tests.py
python3 macos/tests/test-community-integration.py
```

The integration test uses isolated imported profiles and loopback services. It
starts a real headless engine, deliberately advertises an unreachable direct
port, joins through automatic relay fallback, binds a cloud identity and starts
a separate real spectator without increasing the player count. Only its own
processes are stopped. Its temporary profiles contain private test credentials
and must not be published.
