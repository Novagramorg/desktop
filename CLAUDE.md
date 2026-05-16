# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Canonical references

- **`AGENTS.md`** — repository-wide instructions: build commands, coding style, API usage (`MTP*` requests, TL schema, `.match()`/`.data()`), UI styling (`.style` files, no hardcoded sizes), localization (`tr::lng_*`, `tr::` projectors), and RPL (producers, `rpl::on_next`, `rpl::combine`). Read it before writing any code.
- **`REVIEW.md`** — mechanical style rules that the review subagent enforces: empty line before closing brace, operators at start of continuation lines, prefer direct cast over `is + as`, always initialize basic types, `tr::` over `Ui::Text::`, alphabetical includes (folders first, `styles/style_*.h` last), C++17 nested namespaces, `base::take`, etc.
- **`docs/building-{win,mac,mas,linux}.md`** — full per-platform build setup (dependencies tree, toolchain, signing).
- **`docs/api_credentials.md`** — how `TDESKTOP_API_ID` / `TDESKTOP_API_HASH` are injected.

## Build (Debug only — never Release)

Repo root must be at `<BuildPath>/Telegram/tdesktop/` with siblings `Libraries/`, `win64/Libraries/`, `ThirdParty/`. The `out/` tree is already configured by the platform-specific `prepare` scripts.

```bash
# After out/ is configured:
cmake --build out --config Debug --target Telegram
```

- **WSL/Linux:** `Telegram/build/docker/centos_env/build_debug.sh` (Docker required).
- **macOS:** `export QT=6.8` first; build from Xcode-style `out/`.
- **Windows:** must run from the matching VS 2022 Native Tools Command Prompt (`x64` / `x86` / `ARM64`).

**If build fails with file-locked errors** (`C1041 cannot open program database`, `LNK1104`, `cannot open output file Telegram.exe`) — STOP. Do not retry. Ask the user to close the running `Telegram.exe`/debugger. See `AGENTS.md` § Troubleshooting.

There is no test suite — there are isolated test sources under `Telegram/SourceFiles/tests/` but no `ctest` runner; functional verification is by launching the built binary.

## Architecture map

This is a C++17 / Qt 6 (with Qt 5.15 fallback) desktop client for the Telegram MTProto API. Code is split into a **monolithic app** (`Telegram/SourceFiles/`) and a **sibling library tree** (`Telegram/lib_*/`) that the app `add_subdirectory`s via CMake.

### App layout — `Telegram/SourceFiles/`

| Folder | Responsibility |
|---|---|
| `main.cpp`, `mainwindow.{cpp,h}`, `mainwidget.{cpp,h}` | App entry, top-level window, root widget. |
| `core/` | App lifecycle, launcher, settings (`Core::Settings`), `version.h` (AppId/AppName/AppFile — branding constants), update checker. |
| `mtproto/` | Wire protocol. **`mtproto/scheme/mtproto.tl`** (protocol) and **`mtproto/scheme/api.tl`** (Telegram API) are the canonical schemas — `MTP*` types are generated from these. `mtproto_response.h` has `MTP::IgnoreError` / `MTP::ShowErrorFallback` for HTTP 406 handling. |
| `apiwrap.{cpp,h}`, `api/` | High-level wrapper around MTProto requests. The standard `api().request(MTP...).done(...).fail(...).send()` pattern goes through here. |
| `main/` | Session-level state (`Main::Session`, `Main::SessionSettings`). Per-account data. |
| `data/` | In-memory domain model: peers, users, chats, channels, messages, documents, stickers. `Data::Session` is the root of the runtime graph. |
| `storage/` | On-disk persistence using `QDataStream` sequential binary format. **Append-only rule** — see `AGENTS.md` § Local Storage Serialization. |
| `history/` | Message list rendering, chat history, message views, composer. The biggest single subsystem. |
| `window/` | Window controllers, section navigation, layer/box stack. |
| `dialogs/` | Left chat list (search, folders, archive). |
| `info/` | Profile pages (user/chat/channel info, shared media, members). |
| `boxes/` | Modal dialogs (the "Box" abstraction — confirms, pickers, settings panels). |
| `settings/` | Settings UI sections. |
| `chat_helpers/` | Sticker/GIF/emoji panels, attach menus, bot commands. |
| `media/` | Audio/video playback, voice recording, streaming. |
| `calls/` | Voice and video calls (1:1 and group). Uses `lib_webrtc` + `tg_owt`. |
| `payments/` | Telegram Stars and bot-invoice flows. |
| `platform/` | Platform abstraction. Public headers (`platform_*.h`) declare the interface; `linux/`, `mac/`, `win/` subfolders provide implementations. New OS-specific code goes here, not in `core/`. |
| `ui/` | App-specific UI widgets that build on top of `lib_ui` primitives. |
| `lang/` | `tr::lng_*` lang-key codegen consumer. Strings live in `Telegram/Resources/langs/lang.strings`. |
| `iv/` | Instant View (article reader). |
| `passport/`, `export/`, `intro/`, `inline_bots/`, `overview/`, `editor/`, `support/`, `statistics/`, `tde2e/` | Feature verticals — each largely self-contained. |
| `codegen/` | Build-time code generators for emoji, lang keys, palette, numbers (run by CMake; do not edit generated output). |

### Sibling library tree — `Telegram/lib_*/`

These are reusable modules consumed by both `Telegram/` and other `desktop-app` projects. They are full submodule-style trees with their own CMake.

| Library | What it provides |
|---|---|
| `lib_base` | Foundation: `base::take`, `base::flat_map`, weak pointers, timers, RAII helpers, `crl::time` clock, `gsl::finally` re-export. |
| `lib_rpl` | The reactive library (`rpl::producer`, `rpl::single`, `rpl::map`, `rpl::combine`, `rpl::on_next`, `rpl::lifetime`). |
| `lib_ui` | Cross-project widget toolkit (`Ui::RpWidget`, text rendering, painter helpers, animation framework, `Ui::Text::*` — but prefer `tr::` projectors). The `.style` DSL is parsed by its codegen. |
| `lib_tl` | TL schema parser + generator. Produces the `MTP*` types consumed by `mtproto/`. |
| `lib_storage` | File-backed key-value and stream storage primitives. |
| `lib_crl` | Cross-thread runtime: `crl::async`, `crl::on_main`, `crl::time`. The standard "do work off main, then hop back" pattern. |
| `lib_webrtc`, `lib_webview` | Wrappers over `tg_owt` (calls) and embedded webview (login flows, Instant View). |
| `lib_spellcheck`, `lib_translate`, `lib_lottie`, `lib_qr` | Feature-specific. |

When adding code, prefer extending these libraries if the functionality is generic (no Telegram-domain knowledge); keep Telegram-specific logic in `Telegram/SourceFiles/`.

### Resources

- `Telegram/Resources/langs/lang.strings` — all user-facing strings (English source). Other locales are crowdsourced via Telegram and not in-repo.
- `Telegram/Resources/winrc/`, `Telegram/Resources/uwp/`, `Telegram/Telegram.plist`, `lib/xdg/org.telegram.desktop.*`, `snap/snapcraft.yaml` — per-platform branding manifests (app name, bundle id, icons, MIME associations).
- `Telegram/Resources/art/` — app icons (raster).
- `Telegram/Resources/icons/` — in-app SVG icons referenced by `.style` files.

## Cross-cutting conventions (callouts)

These are easy to miss — full rules in `AGENTS.md` / `REVIEW.md`:

- **Never hardcode pixels, margins, fonts, or sizes in `.cpp`.** Define in `.style` files, reference as `st::name`. The app supports interface scaling — raw ints don't scale.
- **`Core::Settings` / `SessionSettings` serialization is append-only.** Insertion in the middle silently corrupts older saves. Guard new reads with `!stream.atEnd()`. For simple flags, prefer the `writePref<T>` / `readPref<T>` KV facility.
- **API requests** — always check `api.tl` for method signatures. For multi-constructor responses use `.match()`; for single-constructor use `.data()`. Add `.handleFloodErrors()` only when you want custom flood-wait handling. Swallow HTTP 406 in UI (`MTP::IgnoreError`).
- **`tr::` projectors over `Ui::Text::`** inside `tr::lng_*()` calls (`tr::bold`, `tr::italic`, `tr::rich`, `tr::marked`, `tr::link`, `tr::url`). Never wrap `tr::lng_*()` in `rpl::single()` — that freezes the snapshot and breaks live language switching.
- **No comments that describe the next line.** Comments only for non-obvious algorithms ≥4 lines. Prefer `auto`, `const auto`, `const auto &`. Use `u"..."_q` instead of `QStringLiteral("...")`.
- **Static member functions use `PascalCase`**, non-static use `camelCase`.
- **Check `!isHidden()` for "is this widget shown" logic, not `isVisible()`** — `isVisible()` is false during parent animations.
- **Consolidate multiple `make_state` calls into one `make_state<State>()` struct** — every call is a separate heap allocation.

## Branding touchpoints (if rebranding/forking)

The "Telegram" app identity is concentrated in ~14 files. Authoritative constants:

- `Telegram/SourceFiles/core/version.h` — `AppId` (Inno Setup GUID), `AppName`, `AppFile`.
- `Telegram/SourceFiles/config.h` — `TDESKTOP_API_ID` / `TDESKTOP_API_HASH` injection point (must come from `my.telegram.org`, never commit literal values).
- `Telegram/CMakeLists.txt` — macOS `bundle_identifier` (lines ~2091, 2109, 2111).
- `Telegram/Telegram.plist` (macOS), `Telegram/Resources/winrc/Telegram.rc` (Windows version info), `Telegram/Resources/uwp/AppX/AppxManifest.xml` (Microsoft Store), `Telegram/build/setup.iss` (Inno Setup installer).
- `lib/xdg/org.telegram.desktop.{desktop,metainfo.xml,service}` (Linux/Flatpak), `snap/snapcraft.yaml` (Snap).
- `Telegram/Resources/art/icon*.{png,ico}` and `Telegram/Resources/uwp/AppX/Assets/` — icon assets.

Localization literals like `u"Telegram"_q` and `_telegram`-suffixed lang keys live in `platform/linux/main_window_linux.cpp`, `platform/mac/global_menu_mac.mm`, and `Telegram/Resources/langs/lang.strings` — they don't flow through `AppName` and must be grepped separately.

## Version

Current: `6.8.2` (see `Telegram/build/version`). Update via `Telegram/build/set_version.{sh,bat,py}`.
