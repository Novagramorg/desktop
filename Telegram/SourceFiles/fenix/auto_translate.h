/*
This file is part of FenixUz Desktop, a fork of Telegram Desktop by Vipads LLC.

Per-chat "Auto-translate" target language for OUTGOING messages. When a target
language is set for a chat, pressing Send first translates the typed text to
that language (via Telegram's messages.translateText) and then sends the
translation. Equivalent of the Android org.fenixuz.utils.AutoTranslate engine
(storage half) — the send-path hook lives in HistoryWidget and the language
picker in the chat menu.

Storage: JSON file at cWorkingDir()/tdata/fenix_autotranslate.json
*/
#pragma once

#include <rpl/event_stream.h>

#include <QtCore/QString>

#include <map>

namespace Fenix {

class AutoTranslate final {
public:
	[[nodiscard]] static AutoTranslate &Instance();

	// Two-letter target language code for this chat, or empty when off.
	[[nodiscard]] QString getLang(PeerId peerId) const;
	[[nodiscard]] bool isEnabled(PeerId peerId) const;

	// An empty / blank code turns auto-translate off for the chat.
	void setLang(PeerId peerId, QString lang);

	[[nodiscard]] rpl::producer<> changes() const;

private:
	AutoTranslate();
	~AutoTranslate() = default;

	void load();
	void save();

	std::map<PeerId, QString> _langs;
	rpl::event_stream<> _changes;
};

[[nodiscard]] inline AutoTranslate &GetAutoTranslate() {
	return AutoTranslate::Instance();
}

} // namespace Fenix
