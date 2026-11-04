# Novagram Desktop

Novagram is an unofficial [Telegram][telegram] client for Windows made by
[Vipads LLC][vipads] (Uzbekistan). It is built on the source code of
[Telegram Desktop][tdesktop] and adds privacy and productivity features, a full
Uzbek interface and its own look.

*Novagram — Telegram Desktop asosidagi norasmiy mijoz: maxfiylik va qulaylik
funksiyalari, toʻliq oʻzbekcha interfeys.*

Novagram is not affiliated with Telegram FZ-LLC. It uses the official
[Telegram API][telegram_api] and the [MTProto][telegram_proto] protocol.

* Website: [novagram.org][site]
* Channels: [@Novagramtg][channel_uz] (Uzbek), [@Novagram_Ru][channel_ru] (Russian)
* Support: [@Avazbekedu][support], [admin@novagram.org](mailto:admin@novagram.org)

## Features

Everything is in **Settings > Novagram Features**.

* **Privacy:** ghost mode (stay offline and send no read receipts or ad
  views), view stories anonymously, preview a chat without marking it read,
  lock a chat with a passcode.
* **Messages:** keep deleted messages, full edit history, one-time voice and
  video, a default text style, an auto-text signature, translation of outgoing
  messages, auto-answer for new chats, a reminder about unread messages,
  scanning text from images, Special forward without the original sender.
* **Confirm before sending:** voice messages, stickers and GIFs.
* **Stories:** download any story, hide stories.
* **Interface:** hide folder tabs, seconds in message times, open the profile
  from a chat list avatar.
* **Channels and groups:** approve or decline all join requests, accept them
  automatically, join with all accounts at once.
* **Tools:** find a chat by username, Chat ID and first message, a curated bot
  list, log in with a bot token, stop automatic downloads.

## Languages

The whole interface is translated into Uzbek. Telegram's community Uzbek pack
covers most strings; [`novagram_uz.strings`][uz_strings] fills the rest and
fixes the app name. The Novagram features are also translated into Russian.

After every upstream update run

    python Telegram/Resources/langs/novagram_uz_check.py

to list new strings that still need an Uzbek translation.

## Building on Windows

1. Prepare the toolchain and the libraries as described in the
   [Telegram Desktop Windows guide][win]. Upstream builds Windows with Qt 6.
2. Get your own `api_id` and `api_hash` at
   [my.telegram.org](https://my.telegram.org) and put them in
   `Telegram/cmake/telegram_options.local.cmake`, which git ignores:

       set(TDESKTOP_API_ID 12345)
       set(TDESKTOP_API_HASH 0123456789abcdef0123456789abcdef)

       set(DESKTOP_APP_DISABLE_AUTOUPDATE ON)
       set(DESKTOP_APP_DISABLE_CRASH_REPORTS ON)

   Never commit these values.
3. Configure and build as in the guide. The result is `Novagram.exe`.

The Novagram blue palette lives in the `lib_ui` submodule, so it is kept in
[`Telegram/cmake/novagram_palette.diff`][palette] and applied automatically
when CMake configures the project.

## Updating from Telegram Desktop

The `novagram` branch follows stable Telegram Desktop releases. Merge the new
release tag from [telegramdesktop/tdesktop][tdesktop_repo], keep the Novagram
changes when resolving conflicts, update the submodules, then run the Uzbek
check above. If configuring stops because the palette no longer applies,
regenerate `novagram_palette.diff` for the new `lib_ui`.

## License

Like Telegram Desktop, the source code is published under GPLv3 with OpenSSL
exception. See [LICENSE][license] and [LEGAL](LEGAL).

## Third-party

* Qt 6 ([LGPL](http://doc.qt.io/qt-6/lgpl.html)) and Qt 5.15 ([LGPL](http://doc.qt.io/qt-5/lgpl.html)) slightly patched
* OpenSSL 3.2.1 ([Apache License 2.0](https://openssl-library.org/source/license/apache-license-2.0.txt))
* WebRTC ([New BSD License](https://github.com/desktop-app/tg_owt/blob/master/LICENSE))
* zlib ([zlib License](http://www.zlib.net/zlib_license.html))
* LZMA SDK 9.20 ([public domain](http://www.7-zip.org/sdk.html))
* liblzma ([public domain](http://tukaani.org/xz/))
* Google Breakpad ([License](https://chromium.googlesource.com/breakpad/breakpad/+/master/LICENSE))
* Google Crashpad ([Apache License 2.0](https://chromium.googlesource.com/crashpad/crashpad/+/master/LICENSE))
* GYP ([BSD License](https://github.com/bnoordhuis/gyp/blob/master/LICENSE))
* Ninja ([Apache License 2.0](https://github.com/ninja-build/ninja/blob/master/COPYING))
* OpenAL Soft ([LGPL](https://github.com/kcat/openal-soft/blob/master/COPYING))
* Opus codec ([BSD License](http://www.opus-codec.org/license/))
* FFmpeg ([LGPL](https://www.ffmpeg.org/legal.html))
* Guideline Support Library ([MIT License](https://github.com/Microsoft/GSL/blob/master/LICENSE))
* Range-v3 ([Boost License](https://github.com/ericniebler/range-v3/blob/master/LICENSE.txt))
* Open Sans font ([Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0.html))
* Vazirmatn font ([SIL Open Font License 1.1](https://github.com/rastikerdar/vazirmatn/blob/master/OFL.txt))
* Emoji alpha codes ([MIT License](https://github.com/emojione/emojione/blob/master/extras/alpha-codes/LICENSE.md))
* xxHash ([BSD License](https://github.com/Cyan4973/xxHash/blob/dev/LICENSE))
* QR Code generator ([MIT License](https://github.com/nayuki/QR-Code-generator#license))
* CMake ([New BSD License](https://github.com/Kitware/CMake/blob/master/Copyright.txt))
* Hunspell ([LGPL](https://github.com/hunspell/hunspell/blob/master/COPYING.LESSER))
* Ada ([Apache License 2.0](https://github.com/ada-url/ada/blob/main/LICENSE-APACHE))
* zxcvbn-c ([MIT License](https://github.com/tsyrogit/zxcvbn-c/blob/master/LICENSE.txt))
* FrequencyWords by Hermit Dave ([Creative Commons Attribution-ShareAlike 4.0](https://creativecommons.org/licenses/by-sa/4.0/))

[//]: # (LINKS)
[telegram]: https://telegram.org
[telegram_api]: https://core.telegram.org
[telegram_proto]: https://core.telegram.org/mtproto
[tdesktop]: https://desktop.telegram.org
[tdesktop_repo]: https://github.com/telegramdesktop/tdesktop
[vipads]: https://vipads.uz
[site]: https://novagram.org
[channel_uz]: https://t.me/Novagramtg
[channel_ru]: https://t.me/Novagram_Ru
[support]: https://t.me/Avazbekedu
[uz_strings]: Telegram/Resources/langs/novagram_uz.strings
[palette]: Telegram/cmake/novagram_palette.diff
[win]: docs/building-win.md
[license]: LICENSE
