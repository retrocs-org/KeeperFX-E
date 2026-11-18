# KeeperFX macOS port changes

This fork tracks the native macOS work in `lurcheous73/keeperfx-macos`.
It is based on upstream KeeperFX and keeps the original game-data requirement.

## 14 September 2026 — native Mac milestone

### Native build and portability
- Added a native Apple/macOS CMake path and platform handling.
- Fixed Apple Clang C/C++ portability errors, including unnamed parameters,
  switch/case declaration scope, signed sentinel values and deprecated patterns.
- Fixed packed runtime structures that left pointers/function pointers
  misaligned on ARM64. Serialized Dungeon Keeper data structures were not
  deliberately repacked.
- Added macOS `iconv` linkage and Apple-compatible dependency handling.
- Native ARM64 KeeperFX now links and reaches the main menu on Apple Silicon.

### macOS runtime layout
- KeeperFX now discovers its resource root from its executable location.
- The engine self-roots its working directory so legacy relative resource
  loads work when launched from Finder, Terminal or another working directory.
- Writable state is separated from the application bundle on macOS.
- Logs, saves/settings and screenshots use the macOS user Application Support
  area rather than mutating a signed `.app` bundle.
### Enhanced FMV path
- Added an Enhanced runtime mode and Enhanced movie override lookup.
- Enhanced can substitute `enhanced/video/<movie>.mp4` while Original keeps
  using the untouched Bullfrog Smacker movie.
- Split FFmpeg audio and video onto separate `AVFrame` objects.
- Added `libswscale` conversion so PAL8, YUV and true-colour video are
  normalised to RGBA instead of assuming every decoded movie is PAL8.
- Replaced old Smacker-specific timing arithmetic with FFmpeg timestamp
  rescaling using best-effort PTS values.
- Added RGBA image presentation to both SDL/Metal software presentation and
  the OpenGL image-present path.
- FMV presentation can own the whole frame so stale game/UI/fade rendering
  cannot draw over cinematics.
- Original mode keeps nearest-neighbour presentation; Enhanced may use smooth
  presentation and higher-resolution restored movie assets.

### Renderer and correctness fixes
- Added true-colour image-present descriptors and correct row-byte copying.
- Added OpenGL RGBA texture upload and true-colour shader handling.
- Fixed embedded-palette texture lifetime/lookup around resource-map growth.
- Added a virtual destructor to the polymorphic GUI message base class.
- Cleaned several Clang warnings and unsafe/obsolete formatting patterns found
  while making the native build compile.
### Reproducible dependency bootstrap
- Added `macos/bootstrap-deps.sh` to fetch the exact source revisions used by
  the validated Mac build:
  - centijson `8c7a5fb`
  - astronomy `865d3da`
  - enet6 `bf0003f`
- Added `macos/astronomy-compat.patch` so the astronomy source matches the
  compatibility edits used by the successful build.

## 15 September 2026 — Universal self-contained release build
- Added `KFX_MAC_SELF_CONTAINED` for distributable macOS builds.
- Produced and live-tested a single Universal `arm64 + x86_64` KeeperFX binary.
- Removed all Homebrew runtime links from the release binary. `otool -L` now
  reports only macOS system libraries/frameworks.
- Replaced the release dependency on LuaJIT with static Lua 5.4.8 plus the
  small KeeperFX compatibility aliases required by the existing scripts.
- Built SDL3, SDL3_image, SDL3_mixer, OpenAL Soft, libspng, minizip,
  miniupnpc and libnatpmp as Universal static dependencies.
- Built a deliberately minimal FFmpeg 9.0.1 static media core containing
  Smacker plus H.264/AAC MP4 support used by Original/Enhanced FMV playback.
- Preserved OpenAL Soft's MS-ADPCM support while using CoreAudio only; no
  PortAudio/Homebrew audio backend is present in the release build.
- Fixed Darwin feature visibility for the POSIX crash handler on modern SDKs.
- Kept KeeperFX's embedded dr_mp3 implementation private so it can coexist
  with SDL_mixer's built-in dr_mp3 decoder in a fully static link.
- Both slices target macOS 13.0. ARM and Intel mode smoke tests reached the
  main menu; Enhanced static-FFmpeg playback was verified against the full
  restored intro with correct colour and timing.
- Added `macos/build-universal-deps.sh` and `macos/build-universal.sh` so the
  validated dependency set and final Universal engine can be rebuilt.

## Current validation state
- Apple Silicon / ARM64 and Intel x86_64: Universal native build proven.
- Release engine: zero `/opt/homebrew` runtime dependencies.
- Original and Enhanced launch paths reach the main menu on the validated Mac.
- Enhanced H.264/MP4 playback: colour, audio and timing proven in live smoke tests.
- Full AI-restored Enhanced intro is generated outside this source repository
  and is not committed because it derives from original game media.
- The combined macOS launcher is maintained in the wider Dungeon Keeper Mac
  project; this repository contains the KeeperFX engine port itself.

## Build note
For a distributable Universal build, run `macos/build-universal.sh`. It invokes
`macos/build-universal-deps.sh` when the pinned static dependency staging tree
is absent and uses `macos/bootstrap-deps.sh` for the source-only KeeperFX donor
dependencies. The older pkg-config/Homebrew route remains available for local
development builds when `KFX_MAC_SELF_CONTAINED` is left disabled.

## 15 November 2026 — online lobby networking
- Replaced the system libcurl in self-contained builds: Apple's library exports
  WebSocket functions but rejects the `ws` and `wss` protocols at runtime.
- Added pinned Universal static curl 8.23.0 and OpenSSL 3.5.9 builds through
  `macos/build-network-deps.sh`, with Apple native certificate verification.
- Verified an authenticated TLS WebSocket upgrade (HTTP 101) against the live
  KeeperFX matchmaking endpoint. A complete multiplayer session still requires
  a second compatible client for verification.

## 15 November 2026 — saved-game path fix
- Fixed directory creation rejecting absolute paths by trying to create an empty
  directory before the initial slash. This prevented saved games and continue
  files from being written to the Mac Application Support directory.
- Check allocation failures and reject existing parent paths that are files.
- Verified save-file creation and write/read roundtrip through the real engine
  file API with an absolute path containing spaces.

## 18 November 2026 — native launcher and installer
- Added a native Cocoa launcher with display mode, window resolution, renderer,
  opening-cinematic and music-volume settings, save/log shortcuts, and a secure
  online-service connection check.
- The signed engine stays in the app, but the launcher sets its runtime root to
  Application Support/GameData. Legacy cache and relative writes now go to user
  data. Existing saves and settings in Application Support remain in place.
- Added a first-run importer that validates all 14 required original data files,
  accepts uppercase mounted-CD layouts, copies optional Smacker movies, and
  preserves the previous import when validation fails. CD files and movies are
  excluded from the distribution app and DMG.
- Added app/DMG notarization, ticket stapling and validation to package-app.sh.
- Added an explicit five-second deadline around the WebSocket HTTP upgrade,
  readable connection errors and Retry/Back controls. Retain the curl multi
  handle for subsequent CONNECT_ONLY WebSocket traffic.
- Fixed the FFmpeg decoder name from smackvid to smacker. Video and available
  audio decode tests pass for all five original movies.
- Verified importer updates preserve configuration and caches, a live online
  connection works, two stalled-server attempts fail in approximately five
  seconds, and gameplay startup leaves the signed app unchanged.

## 18 November 2026 — DualSense and Xbox gamepads
- Forward SDL gamepad add/remove/remap events so controllers work when connected
  after launch. Only the selected controller can send gameplay input; removing a
  different pad does not close the active pad. Reconnect/failover clears held input.
- Enable SDL's macOS GCController and supported HIDAPI Xbox/PS5 backends. Keep
  supplemental mappings in the packaged runtime.
- Apply smooth dead-zone scaling to both sticks and separate trigger thresholds.
  Idle stick drift no longer changes the active input-device indicator.
- Release controller-generated mouse clicks when disconnected or focus is lost.
- Added a native launcher Controllers panel with detection, rescan, pairing advice
  and Xbox/DualSense button equivalents. The diagnostic initializes the native
  application event loop so Bluetooth GCController devices are discovered.
- Virtual Xbox/DualSense tests cover buttons, triggers, stick range, hot-plugging,
  unrelated-device events, failover and disconnect cleanup. A connected physical
  Xbox Series X controller was detected on this Mac. A physical input check
  observed A/B, shoulder/D-pad buttons, both sticks and both triggers (21 button
  presses and 260 substantial axis changes). Physical DualSense testing remains
  outstanding.

## 18 November 2026 — controls, safe saves and presentation options
- Added context-sensitive A/Cross confirm and B/Circle back, configurable menu
  buttons and controller pointer speed. The launcher can remap all defined
  game actions, including stick directions and button chords, while retaining
  keyboard bindings and untouched custom controller mappings.
- Mac saves are written to a temporary file, flushed and atomically committed;
  three previous versions are retained as .backup1 through .backup3. Failed
  temporary writes leave the current save intact.
- Added three labelled, rotating single-player autosave slots (off, 2, 5 or 10
  minutes of game time), latest-save launching and restoration of a backup into
  a new slot. Network games, replays and paused games do not autosave.
- Multiplayer diagnostics show the local version/build/fingerprint, secure
  handshake latency and availability of the default local UDP port. The report
  distinguishes lobby reachability from unverified NAT/peer reachability and
  provides version comparison. Existing in-lobby version checks remain active.
- Added a reversible Original/Enhanced choice. Enhanced smooths Software/Metal
  presentation and can use imported H.264/AAC movie replacements. A validator
  rejects unsupported/corrupt imports; a later playback failure falls back to
  the original movie. No replacement movie pack is bundled.
- Added Command-Q/window-close progress warnings in single-player, safe default
  dismissal, aspect-preserving window resizing, refresh-rate updates on display
  changes and windowed fallback when a display is removed. Controller loss
  releases synthetic clicks and opens the pause menu in local games.
- Verified real manual-save backups, four autosaves rotating through three slots,
  loading/resaving an autosave, valid MP4 playback, corrupt-movie fallback,
  controller/menu regressions and TOML preservation. Two-computer multiplayer and
  physical display removal remain hardware-validation tasks.


## In-game controller, performance and save improvements (20261118.7)

- Normal app startup goes directly into KeeperFX after the one-time data import.
- Main and dungeon Options menus open an integrated Controller / Performance /
  Saves / Remap / Button guide panel. F9 or an unassigned right-stick click opens
  the guide. Local play pauses with pause ownership preserved; multiplayer runs.
- User-supplied MIT controller SVGs are adapted for the default KeeperFX layout,
  with derived PNGs bundled for the engine. Xbox/DualSense labels and prompts,
  live dead-zone/cursor/vibration adjustment, physical input/chord remapping.
- Persistent 30/60/120/unlimited frame caps, timings overlay, presentation mode
  and autosave interval. Simulation speed is unchanged.
- New saves have JSON level metadata and PNG previews. Sidecars rotate alongside
  three save backups; browser reads original INFO headers for old saves and
  restores backups to a new slot before loading.
- Source-level panel tests cover rendering, input, persisted preferences, pause
  behavior, legacy metadata and backup recovery. Live isolated-profile checks
  cover manual saves, PNG/JSON sidecars and three-slot autosave rotation.


## Display and cursor corrections (20261118.8)

- Replace the floating frontend text with a centered, art-backed More Options
  button and separate it from the mouse settings and other navigation buttons.
- Open on a new Display tab with windowed/desktop-fullscreen mode, window/render
  resolution, Apply Display and optional windowed cursor confinement. Apply at a
  frame boundary; failed modes restore the prior mode. Save choices for startup.
- Default to an unconfined windowed cursor, always release it in settings, and
  restore the host cursor when leaving the window. Fullscreen transitions
  reevaluate capture intent instead of reusing the last effective grab state.
- Regression coverage includes grab-policy combinations, persistent display
  selection, failed-mode fallback and six-page rendering. Live isolated-profile
  tests verify real 800x600/windowed, desktop-fullscreen and 1280x800/windowed
  transitions and restart into the frontend with persisted settings.


## Menu, safety and accessibility improvements (20261118.9)

- Display changes remain provisional for 15 seconds. Keep saves the choice;
  timeout, Esc/back, closing the panel and explicit Revert restore the previous
  display. Unconfirmed choices are not written to preferences.
- All eight settings pages share the game's bitmap menu font, palette, button
  sprites and panel texture. Dedicated sprite-sheet copies survive mode changes.
- Visible focus, brand-aware/custom-button prompts, held D-pad navigation, safe
  repeat release and remapping action selection. Save auxiliary shortcuts stay
  distinct from all supported confirm/back face-button combinations.
- Searchable save-slot list, manual/autosave filters, grouped backups, large
  preview, UTF-8-safe text boundaries and orphaned-backup recovery.
- In-game Network diagnostics use a nonblocking signed helper process; connection
  status, lobby ping, retained errors, version comparison and explicit IPv4 join
  are available. The LAN Add Session entry opens this page. No lobby is published
  by diagnostics. Full peer reachability still needs a host/join test.
- Persistent settings/caption text size, cursor/hand size, edge scrolling/border,
  mentor event captions and display duration. Custom/movie speech needs supplied
  subtitle text; event descriptions are not invented verbatim transcripts.
- Verified with settings/input/save boundary tests, virtual-controller tests,
  real 15-second rollback and resolution transitions, muted-voice caption capture,
  native-font menu captures and bounded local-server matchmaking retries.

## 18 November 2026 — original-style section menus

- Restore the original Bullfrog frontend background, rounded gold buttons and
  larger cream native text. Choose a section first rather than crowding eight
  tabs into each page. Long labels shorten without shrinking the font.
- Composite the original Horny pointer after the exclusive menu image so it
  remains visible; use the system pointer if the sprite cannot be loaded.
- Give saves, network reports, version comparison and direct-IP joining their
  own pages, with Back returning to their parent section.
- Keep display confirmation/rollback, existing options, save backups and
  controller navigation. Controller confirm opens the guide's large legend.

## 18 November 2026 — menu, save and window polish

- Reuse the original eight-frame gold border animation and 100 ms cadence for
  hover and controller/keyboard focus; retain parent selections and large text.
- Route the legacy in-game resolution button directly to Display Options.
- Detect active Xbox/DualSense labels automatically, with explicit overrides,
  and build the guide legend from current remapped actions.
- Show save success/failure independently of mentor captions, last autosave time,
  and a recovery page with save-folder and backup access.
- Expose the existing persisted player-name setting, atomically remember validated
  direct-host addresses, and copy connection/support reports to the clipboard.
- Add About & Diagnostics with app/build/engine versions, active controller,
  log/save folder actions, and a copyable report.
- Persist window position, clamp oversized/offscreen windows to usable monitor
  bounds, restore window placement after fullscreen, and release the OS pointer
  on focus loss and letterbox crossings.

## 18 November 2026 — controller camera guide

- Add a Camera Controls page showing current zoom/rotation chords for Xbox and
  DualSense, including custom remaps. Preserve the existing default A/Cross +
  D-pad controls and verify them through the actual game binding matcher.

## Build 20261118.15: controller and game tools

- Controller > Select Controller lists connected devices and remembers the chosen GUID.
- Hold an unassigned R3 in dungeon view; choose with the left stick and release for rooms, spells, map, heart, battle, units or traps. Back cancels. Remapped R3 retains its assigned action.
- Saved Games > Details > Manage adds display-name aliases, protected favourites, backup previews and recoverable group deletion under save/Trash. Protected automatic saves are skipped by rotation.
- Normal host starts compare engine version, selected campaign/map bytes and active mod files asynchronously through existing reliable chat frames. Unknown/older peers require an explicit Start Unchecked override. This is a content compatibility check, not peer authentication.
- Accessibility > Presets offers Standard, Large Text and Cursor, Captions and Comfort, and Reduced Camera Movement. Caption duration and camera pan speed remain adjustable.
- Save aliases and protection use sidecars; original save-format compatibility is preserved.

## Build 20261118.16: community networking

- Integrate Internet/LAN/Favourites/Recent browsing, search, sorting, compatibility
  filtering, direct/relay joining, passwords, no-slot ping and public hosting.
- Add optional browser-paired accounts and website-backed profiles, statistics,
  friends, match history, achievements, event scheduling, verified tournament
  brackets, co-op challenge completion and replay sharing.
- Add supervised headless hosting (one synchronized AI plus up to three humans),
  map rotation, authenticated account tickets and buffered read-only spectating.
- Keep LAN, direct IP/hostname/IPv6 and original lobbies independent of accounts.
- Preserve campaign/map-pack identity in saves and replays, fix replay path aliasing
  and avoid treating disconnects as verified wins.
- Local multi-process engine tests cover failed direct joining, relay fallback,
  cloud identity and a separate spectator without an extra player slot.
- No public deployment is performed. See docs/COMMUNITY.md for local operations.
