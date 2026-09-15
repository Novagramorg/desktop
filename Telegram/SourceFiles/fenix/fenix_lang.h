#pragma once

namespace Fenix {

// Injects Uzbek/Russian translations of all Novagram strings (lng_fenix_*)
// into the lang instance for the current language. English stays as the
// compiled-in base. Re-applies automatically when the user switches language.
// Call once at startup, after the lang pack is loaded.
void SetupLang();

} // namespace Fenix
