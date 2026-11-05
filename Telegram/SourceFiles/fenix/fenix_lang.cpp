#include "fenix/fenix_lang.h"

#include "lang/lang_file_parser.h"
#include "lang/lang_instance.h"
#include "lang/lang_keys.h"

#include <vector>

namespace Fenix {
namespace {

using Entry = std::pair<QString, QString>;
using Table = std::vector<Entry>;

const Table &Uzbek() {
	static const auto data = Table{
		{ u"lng_fenix_features"_q, u"Novagram imkoniyatlari"_q },
		{ u"lng_fenix_section_privacy"_q, u"Maxfiylik"_q },
		{ u"lng_fenix_section_messages"_q, u"Xabarlar"_q },
		{ u"lng_fenix_ghost_mode"_q, u"Sharpa rejimi"_q },
		{ u"lng_fenix_ghost_mode_about"_q, u"Yoqilganda, siz Novagramdan odatdagidek foydalanaverasiz, lekin boshqalarga oflayn koʻrinasiz."_q },
		{ u"lng_fenix_stranger_title"_q, u"Begonalardan himoya"_q },
		{ u"lng_fenix_stranger_shield"_q, u"Begonalardan himoya"_q },
		{ u"lng_fenix_stranger_shield_about"_q, u"Kontaktlaringizda boʻlmagan odamlarning chatlarini berkitadi va ovozsiz qiladi — ular Arxivga oʻtadi, u yerda baribir oʻqib, javob bera olasiz. Telegramning rasmiy chatlari hech qachon berkitilmaydi."_q },
		{ u"lng_fenix_save_deleted_msg"_q, u"Oʻchirilgan xabarlarni saqlash"_q },
		{ u"lng_fenix_save_deleted_msg_about"_q, u"Boshqalar oʻchirgan xabarlar “oʻchirgan” belgisi bilan koʻrinib turadi. Oʻzingiz oʻchirgan xabarlar baribir olib tashlanadi."_q },
		{ u"lng_fenix_deleted_by_other"_q, u"Oʻchirildi: yuboruvchi tomonidan"_q },
		{ u"lng_fenix_deleted_by_me"_q, u"Oʻchirildi: oʻzim tomonimdan"_q },
		{ u"lng_fenix_deleted_by_channel"_q, u"Oʻchirildi: kanal tomonidan"_q },
		{ u"lng_fenix_save_edit_history"_q, u"Tahrir tarixini saqlash"_q },
		{ u"lng_fenix_save_edit_history_about"_q, u"Tahrirlangan xabarlarning oldingi versiyalarini saqlaydi, shunda xabar oʻzgartirilishidan oldin nima deyilganini koʻrishingiz mumkin."_q },
		{ u"lng_fenix_saved_history_clear"_q, u"Saqlangan tarixni tozalash"_q },
		{ u"lng_fenix_saved_history_clear_sure"_q, u"Bu qurilma shu hisob uchun saqlagan oʻchirilgan xabarlar va tahrir tarixi oʻchirib tashlansinmi?\n\nKeyin bu amalni bekor qilib boʻlmaydi."_q },
		{ u"lng_fenix_saved_history_cleared"_q, u"Saqlangan tarix tozalandi."_q },
		{ u"lng_fenix_edit_history"_q, u"Tahrir tarixi"_q },
		{ u"lng_fenix_edit_history_empty"_q, u"Bu xabar uchun oldingi versiyalar saqlanmagan."_q },
		{ u"lng_fenix_edit_history_current"_q, u"Joriy versiya"_q },
		{ u"lng_fenix_auto_translate"_q, u"Avtomatik tarjima"_q },
		{ u"lng_fenix_auto_translate_on"_q, u"Avtomatik tarjima: {lang}"_q },
		{ u"lng_fenix_auto_translate_off"_q, u"Avtomatik tarjimani oʻchirish"_q },
		{ u"lng_fenix_auto_translate_choose"_q, u"Xabarlarni tarjima qilish tili"_q },
		{ u"lng_fenix_auto_translate_progress"_q, u"Tarjima qilinmoqda…"_q },
		{ u"lng_fenix_auto_translate_failed"_q, u"Tarjima qilib boʻlmadi. Asl matnni yuborish uchun “Yuborish”ni qayta bosing."_q },
		{ u"lng_fenix_auto_translate_rich"_q, u"Avtomatik tarjima maqolalarda ishlamaydi. Tarjimasiz yuborish uchun “Yuborish”ni qayta bosing."_q },
		{ u"lng_fenix_save_story"_q, u"Hikoyani saqlash"_q },
		{ u"lng_fenix_story_download"_q, u"Istalgan hikoyani yuklab olish"_q },
		{ u"lng_fenix_story_download_about"_q, u"Hikoya koʻruvchi menyusiga “Hikoyani saqlash” bandini qoʻshadi, bu esa Premium yoki muallif cheklovidan qatʼi nazar istalgan odamning hikoyasini yuklab olishga imkon beradi."_q },
		{ u"lng_fenix_section_data"_q, u"Maʼlumotlar"_q },
		{ u"lng_fenix_stop_auto_download"_q, u"Avtomatik yuklab olishni toʻxtatish"_q },
		{ u"lng_fenix_stop_auto_download_about"_q, u"Chatni ochganingizda foto, video, audio va fayllar avtomatik yuklab olinishini toʻxtatadi — trafik va xotirani tejaydi. Stikerlar baribir yuklanadi."_q },
		{ u"lng_fenix_chat_preview"_q, u"Razm solish (oʻqilmasdan)"_q },
		{ u"lng_fenix_section_interface"_q, u"Interfeys"_q },
		{ u"lng_fenix_hide_folder_tabs"_q, u"Jild yorliqlarini berkitish"_q },
		{ u"lng_fenix_hide_folder_tabs_about"_q, u"Bir nechta jildingiz boʻlsa ham, chatlar roʻyxatidan jild/filtr yorliqlari qatorini berkitadi."_q },
		{ u"lng_fenix_section_stories"_q, u"Hikoyalar"_q },
		{ u"lng_fenix_hide_stories"_q, u"Hikoyalarni berkitish"_q },
		{ u"lng_fenix_story_ghost"_q, u"Hikoyalarni yashirin koʻrish"_q },
		{ u"lng_fenix_story_ghost_about"_q, u"Yoqilganda, hikoya egalari siz ularning hikoyasini koʻrganingizni bilmaydi. Sharpa rejimida bu allaqachon yoqilgan boʻladi."_q },
		{ u"lng_fenix_section_confirm"_q, u"Yuborishdan oldin tasdiqlash"_q },
		{ u"lng_fenix_confirm_voice_toggle"_q, u"Ovozli xabarlarni tasdiqlash"_q },
		{ u"lng_fenix_confirm_sticker_toggle"_q, u"Stikerlarni tasdiqlash"_q },
		{ u"lng_fenix_confirm_gif_toggle"_q, u"GIFlarni tasdiqlash"_q },
		{ u"lng_fenix_confirm_send_about"_q, u"Tanlangan turlarni yuborishdan oldin tasdiq soʻraydi, shunda tasodifan yuborib yubormaysiz."_q },
		{ u"lng_fenix_confirm_voice"_q, u"Ushbu ovozli xabar yuborilsinmi?"_q },
		{ u"lng_fenix_confirm_sticker"_q, u"Ushbu stiker yuborilsinmi?"_q },
		{ u"lng_fenix_confirm_gif"_q, u"Ushbu GIF yuborilsinmi?"_q },
		{ u"lng_fenix_one_time_voice"_q, u"Bir martalik ovoz va video"_q },
		{ u"lng_fenix_one_time_voice_on"_q, u"Bir martalik ovoz va video: yoqilgan ✓"_q },
		{ u"lng_fenix_text_style_title"_q, u"Yozish"_q },
		{ u"lng_fenix_text_style"_q, u"Standart matn uslubi"_q },
		{ u"lng_fenix_text_style_about"_q, u"Tanlangan uslubni yuborishdan oldin butun xabarga qoʻllaydi. Oʻz formatlashingizni saqlash uchun “Oʻchiq”ni tanlang."_q },
		{ u"lng_fenix_text_style_off"_q, u"Oʻchiq"_q },
		{ u"lng_fenix_text_style_regular"_q, u"Oddiy (formatlashni tozalash)"_q },
		{ u"lng_fenix_text_style_bold"_q, u"Qalin"_q },
		{ u"lng_fenix_text_style_italic"_q, u"Qiya"_q },
		{ u"lng_fenix_text_style_mono"_q, u"Monoboʻshliq"_q },
		{ u"lng_fenix_text_style_strike"_q, u"Oʻrtasiga chizilgan"_q },
		{ u"lng_fenix_text_style_underline"_q, u"Tagiga chizilgan"_q },
		{ u"lng_fenix_text_style_spoiler"_q, u"Yashirin"_q },
		{ u"lng_fenix_text_style_quote"_q, u"Iqtibos"_q },
		{ u"lng_fenix_auto_text"_q, u"Avtomatik matn (imzo)"_q },
		{ u"lng_fenix_auto_text_on"_q, u"Avtomatik matn: yoqilgan ✓"_q },
		{ u"lng_fenix_auto_text_placeholder"_q, u"Bu yerda yuboradigan har bir xabaringizga qoʻshiladigan matn"_q },
		{ u"lng_fenix_chat_id"_q, u"Chat ID"_q },
		{ u"lng_fenix_chat_id_copied"_q, u"ID nusxalandi"_q },
		{ u"lng_fenix_first_message"_q, u"Birinchi xabar"_q },
		{ u"lng_fenix_special_forward"_q, u"Maxsus uzatish"_q },
		{ u"lng_fenix_bots"_q, u"Novagram botlari"_q },
		{ u"lng_fenix_bots_media"_q, u"Media va yuklab olish"_q },
		{ u"lng_fenix_bots_editing"_q, u"Media tahrirlash"_q },
		{ u"lng_fenix_bots_tools"_q, u"Uskunalar"_q },
		{ u"lng_fenix_bots_fun"_q, u"Foydali va qiziqarli"_q },
		{ u"lng_fenix_section_tools"_q, u"Uskunalar"_q },
		{ u"lng_fenix_chat_finder"_q, u"Foydalanuvchi nomi boʻyicha chat topish"_q },
		{ u"lng_fenix_chat_finder_about"_q, u"Chatni ochish uchun @username (yoki t.me havolasi) kiriting, hatto hech qachon yozishmagan boʻlsangiz ham."_q },
		{ u"lng_fenix_chat_finder_hint"_q, u"foydalanuvchi nomi"_q },
		{ u"lng_fenix_chat_finder_open"_q, u"Ochish"_q },
		{ u"lng_fenix_chat_finder_available"_q, u"Mavjud"_q },
		{ u"lng_fenix_chat_finder_unavailable"_q, u"Mavjud emas"_q },
		{ u"lng_fenix_auto_answer_title"_q, u"Avtomatik javob"_q },
		{ u"lng_fenix_auto_answer"_q, u"Kiruvchi chatlarga avtomatik javob"_q },
		{ u"lng_fenix_auto_answer_text"_q, u"Avtomatik javob matni"_q },
		{ u"lng_fenix_auto_answer_placeholder"_q, u"Yangi chatlarga avtomatik yuboriladigan javob"_q },
		{ u"lng_fenix_auto_answer_about"_q, u"Har bir shaxsiy chatning birinchi kiruvchi xabariga matningiz bilan bir marta javob beradi. Chatni ochganingizda qayta faollashadi."_q },
		{ u"lng_fenix_join_requests"_q, u"Qoʻshilish soʻrovlari"_q },
		{ u"lng_fenix_join_requests_count#one"_q, u"{count} kishi qoʻshilishni kutmoqda."_q },
		{ u"lng_fenix_join_requests_count#other"_q, u"{count} kishi qoʻshilishni kutmoqda."_q },
		{ u"lng_fenix_join_requests_none"_q, u"Kutilayotgan soʻrovlar yoʻq."_q },
		{ u"lng_fenix_join_requests_approve"_q, u"Hammasini qabul qilish"_q },
		{ u"lng_fenix_join_requests_decline"_q, u"Hammasini rad etish"_q },
		{ u"lng_fenix_join_requests_approved"_q, u"Soʻrovlar qabul qilindi."_q },
		{ u"lng_fenix_join_requests_declined"_q, u"Soʻrovlar rad etildi."_q },
		{ u"lng_fenix_join_requests_error"_q, u"Nimadir xato ketdi."_q },
		{ u"lng_fenix_section_channels"_q, u"Kanallar"_q },
		{ u"lng_fenix_auto_accept_join"_q, u"Soʻrovlarni avtomatik qabul qilish"_q },
		{ u"lng_fenix_auto_accept_join_about"_q, u"Siz aʼzo qoʻsha oladigan kanal va guruhlarga kelgan qoʻshilish soʻrovlarini avtomatik tasdiqlaydi."_q },
		{ u"lng_fenix_join_all_accounts"_q, u"Barcha hisoblar bilan qoʻshilish"_q },
		{ u"lng_fenix_join_all_progress"_q, u"Barcha hisoblar bilan qoʻshilmoqda…"_q },
		{ u"lng_fenix_join_all_none"_q, u"Qoʻshilish uchun boshqa hisob yoʻq."_q },
		{ u"lng_fenix_join_all_done#one"_q, u"{count} ta hisob qoʻshildi."_q },
		{ u"lng_fenix_join_all_done#other"_q, u"{count} ta hisob qoʻshildi."_q },
		{ u"lng_fenix_features_keywords"_q, u"sharpa,yashirin,oflayn,fenix"_q },
		{ u"lng_fenix_bot_login"_q, u"Bot tokeni bilan kirish"_q },
		{ u"lng_fenix_bot_token"_q, u"Bot tokeni"_q },
		{ u"lng_fenix_bot_desc"_q, u"Bot sifatida kirish uchun @BotFatherdan olgan tokenni joylashtiring."_q },
		{ u"lng_fenix_bot_invalid"_q, u"Bot tokeni notoʻgʻri. Uni @BotFatherdan nusxalang (123456789:AA... koʻrinishida)."_q },
		{ u"lng_fenix_chat_lock"_q, u"Chatni qulflash"_q },
		{ u"lng_fenix_chat_unlock"_q, u"Chat qulfini ochish"_q },
		{ u"lng_fenix_chat_locked"_q, u"Chat qulflandi"_q },
		{ u"lng_fenix_chat_unlocked"_q, u"Chat qulfi ochildi"_q },
		{ u"lng_fenix_chat_lock_title"_q, u"Qulflangan chat"_q },
		{ u"lng_fenix_chat_lock_enter"_q, u"Kirish kodi"_q },
		{ u"lng_fenix_chat_lock_confirm"_q, u"Kirish kodini tasdiqlang"_q },
		{ u"lng_fenix_chat_lock_set"_q, u"Kirish kodini oʻrnatish"_q },
		{ u"lng_fenix_chat_lock_open"_q, u"Ochish"_q },
		{ u"lng_fenix_chat_lock_how"_q, u"Bu chatni qanday qulflaysiz?"_q },
		{ u"lng_fenix_chat_lock_common"_q, u"Umumiy parol"_q },
		{ u"lng_fenix_chat_lock_separate"_q, u"Alohida parol"_q },
		{ u"lng_fenix_chat_lock_continue"_q, u"Davom etish"_q },
		{ u"lng_fenix_reminder_title"_q, u"Eslatmalar"_q },
		{ u"lng_fenix_reminder"_q, u"Oʻqilmagan xabar eslatmasi"_q },
		{ u"lng_fenix_reminder_delay"_q, u"Eslatma kechikishi (daqiqa)"_q },
		{ u"lng_fenix_reminder_delay_ph"_q, u"Daqiqa (1–59)"_q },
		{ u"lng_fenix_chat_lock_section"_q, u"Chat qulfi"_q },
		{ u"lng_fenix_chat_lock_change_common"_q, u"Umumiy parolni oʻzgartirish"_q },
		{ u"lng_fenix_chat_lock_changed"_q, u"Umumiy parol oʻzgartirildi"_q },
		{ u"lng_fenix_ocr_scan"_q, u"Matnni skanerlash"_q },
		{ u"lng_fenix_ocr_title"_q, u"Matnni skanerlash"_q },
		{ u"lng_fenix_ocr_recognizing"_q, u"Tanilmoqda…"_q },
		{ u"lng_fenix_ocr_empty"_q, u"Matn topilmadi"_q },
		{ u"lng_fenix_ocr_copy"_q, u"Nusxa olish"_q },
		{ u"lng_fenix_ocr_copied"_q, u"Matn nusxalandi"_q },
		{ u"lng_fenix_about"_q, u"Dastur haqida"_q },
		{ u"lng_fenix_about_tagline"_q, u"Xavfsiz messenjer"_q },
		{ u"lng_fenix_avatar_profile"_q, u"Avatar profilni ochadi"_q },
		{ u"lng_fenix_show_seconds"_q, u"Soniyalarni koʻrsatish"_q },
		{ u"lng_fenix_avatar_profile_about"_q, u"Chatlar roʻyxatida avatarni bossangiz, chat emas, profil ochiladi. Koʻrilmagan hikoyalar boʻlsa, avval oʻshalar ochiladi."_q },
		{ u"lng_fenix_about_version"_q, u"Versiya"_q },
		{ u"lng_fenix_about_version_copied"_q, u"Versiya nusxalandi"_q },
		{ u"lng_fenix_about_channels"_q, u"Bizning kanallarimiz"_q },
		{ u"lng_fenix_about_channel_uz"_q, u"Oʻzbekcha kanal"_q },
		{ u"lng_fenix_about_channel_ru"_q, u"Ruscha kanal"_q },
		{ u"lng_fenix_about_support"_q, u"Qoʻllab-quvvatlash"_q },
		{ u"lng_fenix_about_support_contact"_q, u"Qoʻllab-quvvatlash xizmati bilan bogʻlanish"_q },
		{ u"lng_fenix_about_company"_q, u"Kompaniya"_q },
		{ u"lng_fenix_about_site"_q, u"Novagram sayti"_q },
		{ u"lng_fenix_about_company_site"_q, u"Kompaniya sayti"_q },
		{ u"lng_fenix_about_email"_q, u"Email"_q },
		{ u"lng_fenix_about_email_copied"_q, u"Email nusxalandi"_q },
		{ u"lng_fenix_about_company_text"_q, u"VipAds LLC · 2022-yilda tashkil topgan · Oʻzbekiston Respublikasi. Biz — Oʻzbekistonda rasmiy roʻyxatdan oʻtgan texnologiya kompaniyasimiz. Odamlarga tez, xavfsiz va qulay muloqotni taqdim etuvchi Novagram ilovasini ishlab chiqqanmiz."_q },
	};
	return data;
}

const Table &Russian() {
	static const auto data = Table{
		{ u"lng_fenix_features"_q, u"Возможности Novagram"_q },
		{ u"lng_fenix_section_privacy"_q, u"Конфиденциальность"_q },
		{ u"lng_fenix_section_messages"_q, u"Сообщения"_q },
		{ u"lng_fenix_ghost_mode"_q, u"Режим призрака"_q },
		{ u"lng_fenix_ghost_mode_about"_q, u"Когда включено, вы выглядите офлайн для других, продолжая обычно пользоваться Novagram."_q },
		{ u"lng_fenix_stranger_title"_q, u"Защита от незнакомцев"_q },
		{ u"lng_fenix_stranger_shield"_q, u"Скрывать незнакомцев"_q },
		{ u"lng_fenix_stranger_shield_about"_q, u"Скрывает чаты людей не из ваших контактов и отключает для них уведомления — они уходят в Архив, где вы по-прежнему можете читать и отвечать. Официальные чаты Telegram никогда не скрываются."_q },
		{ u"lng_fenix_save_deleted_msg"_q, u"Сохранять удалённые сообщения"_q },
		{ u"lng_fenix_save_deleted_msg_about"_q, u"Сообщения, удалённые другими, остаются видимыми с пометкой «удалено». Ваши собственные удаления всё равно убираются."_q },
		{ u"lng_fenix_deleted_by_other"_q, u"Удалено: отправителем"_q },
		{ u"lng_fenix_deleted_by_me"_q, u"Удалено: мной"_q },
		{ u"lng_fenix_deleted_by_channel"_q, u"Удалено: каналом"_q },
		{ u"lng_fenix_save_edit_history"_q, u"Сохранять историю изменений"_q },
		{ u"lng_fenix_save_edit_history_about"_q, u"Сохраняет предыдущие версии изменённых сообщений, чтобы вы могли видеть, что было написано до правки."_q },
		{ u"lng_fenix_saved_history_clear"_q, u"Очистить сохранённую историю"_q },
		{ u"lng_fenix_saved_history_clear_sure"_q, u"Забыть удалённые сообщения и историю изменений, сохранённые на этом устройстве для этого аккаунта?\n\nЭто действие нельзя отменить."_q },
		{ u"lng_fenix_saved_history_cleared"_q, u"Сохранённая история очищена."_q },
		{ u"lng_fenix_edit_history"_q, u"История изменений"_q },
		{ u"lng_fenix_edit_history_empty"_q, u"Для этого сообщения нет сохранённых предыдущих версий."_q },
		{ u"lng_fenix_edit_history_current"_q, u"Текущая версия"_q },
		{ u"lng_fenix_auto_translate"_q, u"Автоперевод"_q },
		{ u"lng_fenix_auto_translate_on"_q, u"Автоперевод: {lang}"_q },
		{ u"lng_fenix_auto_translate_off"_q, u"Выключить автоперевод"_q },
		{ u"lng_fenix_auto_translate_choose"_q, u"Переводить сообщения на"_q },
		{ u"lng_fenix_auto_translate_progress"_q, u"Перевод…"_q },
		{ u"lng_fenix_auto_translate_failed"_q, u"Не удалось перевести. Нажмите «Отправить» ещё раз, чтобы отправить оригинал."_q },
		{ u"lng_fenix_auto_translate_rich"_q, u"Автоперевод не работает для rich-сообщений. Нажмите «Отправить» ещё раз, чтобы отправить без перевода."_q },
		{ u"lng_fenix_save_story"_q, u"Сохранить историю"_q },
		{ u"lng_fenix_story_download"_q, u"Скачать любую историю"_q },
		{ u"lng_fenix_story_download_about"_q, u"Добавляет пункт «Сохранить историю» в меню просмотра историй, позволяя скачивать чьи угодно истории независимо от Premium или ограничений автора."_q },
		{ u"lng_fenix_section_data"_q, u"Данные"_q },
		{ u"lng_fenix_stop_auto_download"_q, u"Остановить автозагрузку"_q },
		{ u"lng_fenix_stop_auto_download_about"_q, u"Останавливает автоматическую загрузку фото, видео, аудио и файлов при открытии чата — экономит трафик и память. Стикеры всё равно загружаются."_q },
		{ u"lng_fenix_chat_preview"_q, u"Просмотр (без прочтения)"_q },
		{ u"lng_fenix_section_interface"_q, u"Интерфейс"_q },
		{ u"lng_fenix_hide_folder_tabs"_q, u"Скрыть вкладки папок"_q },
		{ u"lng_fenix_hide_folder_tabs_about"_q, u"Скрывает строку вкладок папок/фильтров из списка чатов, даже если у вас несколько папок."_q },
		{ u"lng_fenix_section_stories"_q, u"Истории"_q },
		{ u"lng_fenix_hide_stories"_q, u"Скрыть истории"_q },
		{ u"lng_fenix_story_ghost"_q, u"Анонимный просмотр историй"_q },
		{ u"lng_fenix_story_ghost_about"_q, u"Когда включено, авторы историй не увидят, что вы их посмотрели. В режиме призрака это уже включено."_q },
		{ u"lng_fenix_section_confirm"_q, u"Подтверждение перед отправкой"_q },
		{ u"lng_fenix_confirm_voice_toggle"_q, u"Подтверждать голосовые сообщения"_q },
		{ u"lng_fenix_confirm_sticker_toggle"_q, u"Подтверждать стикеры"_q },
		{ u"lng_fenix_confirm_gif_toggle"_q, u"Подтверждать GIF"_q },
		{ u"lng_fenix_confirm_send_about"_q, u"Запрашивает подтверждение перед отправкой выбранных типов, чтобы вы не отправили их случайно."_q },
		{ u"lng_fenix_confirm_voice"_q, u"Отправить это голосовое сообщение?"_q },
		{ u"lng_fenix_confirm_sticker"_q, u"Отправить этот стикер?"_q },
		{ u"lng_fenix_confirm_gif"_q, u"Отправить этот GIF?"_q },
		{ u"lng_fenix_one_time_voice"_q, u"Одноразовые голос и видео"_q },
		{ u"lng_fenix_one_time_voice_on"_q, u"Одноразовые голос и видео: вкл ✓"_q },
		{ u"lng_fenix_text_style_title"_q, u"Набор текста"_q },
		{ u"lng_fenix_text_style"_q, u"Стиль текста по умолчанию"_q },
		{ u"lng_fenix_text_style_about"_q, u"Применяет выбранный стиль ко всему сообщению прямо перед отправкой. Выберите «Выкл», чтобы сохранить своё форматирование."_q },
		{ u"lng_fenix_text_style_off"_q, u"Выкл"_q },
		{ u"lng_fenix_text_style_regular"_q, u"Обычный (очистить форматирование)"_q },
		{ u"lng_fenix_text_style_bold"_q, u"Жирный"_q },
		{ u"lng_fenix_text_style_italic"_q, u"Курсив"_q },
		{ u"lng_fenix_text_style_mono"_q, u"Моноширинный"_q },
		{ u"lng_fenix_text_style_strike"_q, u"Зачёркнутый"_q },
		{ u"lng_fenix_text_style_underline"_q, u"Подчёркнутый"_q },
		{ u"lng_fenix_text_style_spoiler"_q, u"Спойлер"_q },
		{ u"lng_fenix_text_style_quote"_q, u"Цитата"_q },
		{ u"lng_fenix_auto_text"_q, u"Автотекст (подпись)"_q },
		{ u"lng_fenix_auto_text_on"_q, u"Автотекст: вкл ✓"_q },
		{ u"lng_fenix_auto_text_placeholder"_q, u"Текст, добавляемый к каждому сообщению, отправленному здесь"_q },
		{ u"lng_fenix_chat_id"_q, u"ID чата"_q },
		{ u"lng_fenix_chat_id_copied"_q, u"ID скопирован"_q },
		{ u"lng_fenix_first_message"_q, u"Первое сообщение"_q },
		{ u"lng_fenix_special_forward"_q, u"Особая пересылка"_q },
		{ u"lng_fenix_bots"_q, u"Боты Novagram"_q },
		{ u"lng_fenix_bots_media"_q, u"Медиа и загрузки"_q },
		{ u"lng_fenix_bots_editing"_q, u"Редактирование медиа"_q },
		{ u"lng_fenix_bots_tools"_q, u"Инструменты"_q },
		{ u"lng_fenix_bots_fun"_q, u"Полезное и интересное"_q },
		{ u"lng_fenix_section_tools"_q, u"Инструменты"_q },
		{ u"lng_fenix_chat_finder"_q, u"Найти чат по username"_q },
		{ u"lng_fenix_chat_finder_about"_q, u"Введите @username (или ссылку t.me), чтобы открыть чат, даже если вы никогда не писали этому человеку."_q },
		{ u"lng_fenix_chat_finder_hint"_q, u"username"_q },
		{ u"lng_fenix_chat_finder_open"_q, u"Открыть"_q },
		{ u"lng_fenix_chat_finder_available"_q, u"Доступен"_q },
		{ u"lng_fenix_chat_finder_unavailable"_q, u"Недоступен"_q },
		{ u"lng_fenix_auto_answer_title"_q, u"Автоответ"_q },
		{ u"lng_fenix_auto_answer"_q, u"Автоответ на входящие чаты"_q },
		{ u"lng_fenix_auto_answer_text"_q, u"Текст автоответа"_q },
		{ u"lng_fenix_auto_answer_placeholder"_q, u"Ответ, автоматически отправляемый в новые чаты"_q },
		{ u"lng_fenix_auto_answer_about"_q, u"Один раз отвечает вашим текстом на первое входящее сообщение каждого личного чата. Перевзводится, когда вы открываете чат."_q },
		{ u"lng_fenix_join_requests"_q, u"Заявки на вступление"_q },
		{ u"lng_fenix_join_requests_count#one"_q, u"{count} человек ждёт вступления."_q },
		{ u"lng_fenix_join_requests_count#few"_q, u"{count} человека ждут вступления."_q },
		{ u"lng_fenix_join_requests_count#many"_q, u"{count} человек ждут вступления."_q },
		{ u"lng_fenix_join_requests_count#other"_q, u"{count} человека ждут вступления."_q },
		{ u"lng_fenix_join_requests_none"_q, u"Нет ожидающих заявок."_q },
		{ u"lng_fenix_join_requests_approve"_q, u"Принять все"_q },
		{ u"lng_fenix_join_requests_decline"_q, u"Отклонить все"_q },
		{ u"lng_fenix_join_requests_approved"_q, u"Заявки приняты."_q },
		{ u"lng_fenix_join_requests_declined"_q, u"Заявки отклонены."_q },
		{ u"lng_fenix_join_requests_error"_q, u"Что-то пошло не так."_q },
		{ u"lng_fenix_section_channels"_q, u"Каналы"_q },
		{ u"lng_fenix_auto_accept_join"_q, u"Автоприём заявок на вступление"_q },
		{ u"lng_fenix_auto_accept_join_about"_q, u"Автоматически одобряет заявки на вступление в каналах и группах, куда вы можете добавлять участников."_q },
		{ u"lng_fenix_join_all_accounts"_q, u"Вступить со всеми аккаунтами"_q },
		{ u"lng_fenix_join_all_progress"_q, u"Вступление со всеми аккаунтами…"_q },
		{ u"lng_fenix_join_all_none"_q, u"Нет других аккаунтов для вступления."_q },
		{ u"lng_fenix_join_all_done#one"_q, u"Вступил {count} аккаунт."_q },
		{ u"lng_fenix_join_all_done#few"_q, u"Вступили {count} аккаунта."_q },
		{ u"lng_fenix_join_all_done#many"_q, u"Вступили {count} аккаунтов."_q },
		{ u"lng_fenix_join_all_done#other"_q, u"Вступили {count} аккаунта."_q },
		{ u"lng_fenix_features_keywords"_q, u"призрак,скрытность,офлайн,fenix"_q },
		{ u"lng_fenix_bot_login"_q, u"Войти по токену бота"_q },
		{ u"lng_fenix_bot_token"_q, u"Токен бота"_q },
		{ u"lng_fenix_bot_desc"_q, u"Вставьте токен, полученный от @BotFather, чтобы войти как бот."_q },
		{ u"lng_fenix_bot_invalid"_q, u"Неверный токен бота. Скопируйте его у @BotFather (вид 123456789:AA...)."_q },
		{ u"lng_fenix_chat_lock"_q, u"Заблокировать чат"_q },
		{ u"lng_fenix_chat_unlock"_q, u"Разблокировать чат"_q },
		{ u"lng_fenix_chat_locked"_q, u"Чат заблокирован"_q },
		{ u"lng_fenix_chat_unlocked"_q, u"Чат разблокирован"_q },
		{ u"lng_fenix_chat_lock_title"_q, u"Заблокированный чат"_q },
		{ u"lng_fenix_chat_lock_enter"_q, u"Пароль"_q },
		{ u"lng_fenix_chat_lock_confirm"_q, u"Подтвердите пароль"_q },
		{ u"lng_fenix_chat_lock_set"_q, u"Установить пароль"_q },
		{ u"lng_fenix_chat_lock_open"_q, u"Открыть"_q },
		{ u"lng_fenix_chat_lock_how"_q, u"Как заблокировать этот чат?"_q },
		{ u"lng_fenix_chat_lock_common"_q, u"Общий пароль"_q },
		{ u"lng_fenix_chat_lock_separate"_q, u"Отдельный пароль"_q },
		{ u"lng_fenix_chat_lock_continue"_q, u"Продолжить"_q },
		{ u"lng_fenix_reminder_title"_q, u"Напоминания"_q },
		{ u"lng_fenix_reminder"_q, u"Напоминание о непрочитанном"_q },
		{ u"lng_fenix_reminder_delay"_q, u"Задержка напоминания (минуты)"_q },
		{ u"lng_fenix_reminder_delay_ph"_q, u"Минуты (1–59)"_q },
		{ u"lng_fenix_chat_lock_section"_q, u"Блокировка чата"_q },
		{ u"lng_fenix_chat_lock_change_common"_q, u"Изменить общий пароль"_q },
		{ u"lng_fenix_chat_lock_changed"_q, u"Общий пароль изменён"_q },
		{ u"lng_fenix_ocr_scan"_q, u"Распознать текст"_q },
		{ u"lng_fenix_ocr_title"_q, u"Распознать текст"_q },
		{ u"lng_fenix_ocr_recognizing"_q, u"Распознавание…"_q },
		{ u"lng_fenix_ocr_empty"_q, u"Текст не найден"_q },
		{ u"lng_fenix_ocr_copy"_q, u"Копировать"_q },
		{ u"lng_fenix_ocr_copied"_q, u"Текст скопирован"_q },
		{ u"lng_fenix_about"_q, u"О приложении"_q },
		{ u"lng_fenix_about_tagline"_q, u"Безопасный мессенджер"_q },
		{ u"lng_fenix_avatar_profile"_q, u"Аватар открывает профиль"_q },
		{ u"lng_fenix_show_seconds"_q, u"Показывать секунды"_q },
		{ u"lng_fenix_avatar_profile_about"_q, u"В списке чатов нажатие на аватар открывает профиль вместо чата. Непросмотренные истории по-прежнему открываются первыми."_q },
		{ u"lng_fenix_about_version"_q, u"Версия"_q },
		{ u"lng_fenix_about_version_copied"_q, u"Версия скопирована"_q },
		{ u"lng_fenix_about_channels"_q, u"Наши каналы"_q },
		{ u"lng_fenix_about_channel_uz"_q, u"Канал на узбекском"_q },
		{ u"lng_fenix_about_channel_ru"_q, u"Канал на русском"_q },
		{ u"lng_fenix_about_support"_q, u"Поддержка"_q },
		{ u"lng_fenix_about_support_contact"_q, u"Связаться с поддержкой"_q },
		{ u"lng_fenix_about_company"_q, u"Компания"_q },
		{ u"lng_fenix_about_site"_q, u"Сайт Novagram"_q },
		{ u"lng_fenix_about_company_site"_q, u"Сайт компании"_q },
		{ u"lng_fenix_about_email"_q, u"Эл. почта"_q },
		{ u"lng_fenix_about_email_copied"_q, u"Эл. почта скопирована"_q },
		{ u"lng_fenix_about_company_text"_q, u"VipAds LLC · Основана в 2022 году · Республика Узбекистан. Официально зарегистрированная в Узбекистане технологическая компания. Мы создали Novagram, чтобы общение было быстрым, безопасным и удобным."_q },
	};
	return data;
}

// Telegram's Uzbek pack is a community beta: it lacks part of the desktop
// strings and calls the app "Telegram Desktop". Novagram ships its own
// translation for those keys in Resources/langs/novagram_uz.strings.
[[nodiscard]] const Table &UzbekCloudFill() {
	static const auto data = [] {
		auto result = Table();
		auto file = QFile(u":/misc/novagram_uz.strings"_q);
		if (!file.open(QIODevice::ReadOnly)) {
			LOG(("Fenix Lang Error: Could not open novagram_uz.strings."));
			return result;
		}
		const auto parser = Lang::FileParser(file.readAll(), [&](
				QLatin1String key,
				const QByteArray &value) {
			result.emplace_back(QString(key), QString::fromUtf8(value));
		});
		if (!parser.errors().isEmpty()) {
			LOG(("Fenix Lang Error: %1").arg(parser.errors()));
		}
		return result;
	}();
	return data;
}

[[nodiscard]] std::vector<const Table*> TablesForCurrentLanguage() {
	const auto id = Lang::GetInstance().id();
	const auto code = id.split('-').front();
	if (code == u"uz"_q) {
		return { &UzbekCloudFill(), &Uzbek() };
	} else if (code == u"ru"_q) {
		return { &Russian() };
	}
	return {};
}

void Apply() {
	const auto tables = TablesForCurrentLanguage();
	if (tables.empty()) {
		// Other languages keep the compiled-in English base.
		return;
	}
	auto &instance = Lang::GetInstance();

	auto strings = QVector<MTPLangPackString>();
	for (const auto table : tables) {
		for (const auto &[key, value] : *table) {
			strings.push_back(MTP_langPackString(MTP_string(key), MTP_string(value)));
		}
	}

	// Build an incremental difference that only touches our own keys, so the
	// real Telegram strings (and any cloud updates) are left untouched. Keeping
	// from_version == to_version == current version satisfies applyDifference's
	// invariants without advancing the stored version number.
	const auto version = instance.version(Lang::Pack::Current);
	const auto difference = MTP_langPackDifference(
		MTP_string(Lang::LanguageIdOrDefault(instance.id())),
		MTP_int(version),
		MTP_int(version),
		MTP_vector<MTPLangPackString>(strings));
	instance.applyDifference(
		Lang::Pack::Current,
		difference.c_langPackDifference());
}

} // namespace

void SetupLang() {
	static auto lifetime = rpl::lifetime();
	static auto applying = false;

	const auto apply = [] {
		if (applying) {
			return;
		}
		applying = true;
		Apply();
		applying = false;
	};
	apply();

	// Re-inject whenever the user switches language (idChanges fires after the
	// instance has reset its values to the English base) and after every cloud
	// update, which may overwrite keys we translate. Apply() fires updated()
	// itself, so the flag stops it from recursing.
	auto &instance = Lang::GetInstance();
	rpl::merge(
		instance.idChanges() | rpl::to_empty,
		instance.updated()
	) | rpl::on_next(apply, lifetime);
}

QString UzbekLanguageId() {
	return u"uz-beta"_q;
}

bool IsUzbekLanguage(const QString &id) {
	return (id.split('-').front() == u"uz"_q);
}

} // namespace Fenix
