#pragma once

#include <QtCore/QString>

namespace Fenix {

// Injects Uzbek/Russian translations of all Novagram strings (lng_fenix_*)
// into the lang instance for the current language. English stays as the
// compiled-in base. Re-applies automatically when the user switches language.
// Call once at startup, after the lang pack is loaded.
void SetupLang();

// Uzbek is only a beta pack on Telegram's translations platform, so it is
// missing from the official language list. Lang::CloudManager asks for it by
// this id and lists it with the official ones until Telegram ships its own.
[[nodiscard]] QString UzbekLanguageId();
[[nodiscard]] bool IsUzbekLanguage(const QString &id);

} // namespace Fenix
