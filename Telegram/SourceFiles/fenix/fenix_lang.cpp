#include "fenix/fenix_lang.h"

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
		{ u"lng_fenix_ghost_mode_about"_q, u"Yoqilganda, siz Novagram'dan odatdagidek foydalanaverasiz, lekin boshqalarga oflayn ko'rinasiz."_q },
		{ u"lng_fenix_stranger_title"_q, u"Begonalardan himoya"_q },
		{ u"lng_fenix_stranger_shield"_q, u"Begonalarni yashirish"_q },
		{ u"lng_fenix_stranger_shield_about"_q, u"Kontaktlaringizda bo'lmagan odamlarning suhbatlarini yashiradi va ovozsiz qiladi — ular Arxivga o'tadi, u yerda baribir o'qib, javob bera olasiz. Telegram'ning rasmiy suhbatlari hech qachon yashirilmaydi."_q },
		{ u"lng_fenix_save_deleted_msg"_q, u"O'chirilgan xabarlarni saqlash"_q },
		{ u"lng_fenix_save_deleted_msg_about"_q, u"Boshqalar o'chirgan xabarlar 'o'chirgan' belgisi bilan ko'rinib turadi. O'zingiz o'chirgan xabarlar baribir olib tashlanadi."_q },
		{ u"lng_fenix_deleted_by_other"_q, u"O'chirildi: yuboruvchi tomonidan"_q },
		{ u"lng_fenix_deleted_by_me"_q, u"O'chirildi: o'zim tomonimdan"_q },
		{ u"lng_fenix_deleted_by_channel"_q, u"O'chirildi: kanal tomonidan"_q },
		{ u"lng_fenix_save_edit_history"_q, u"Tahrir tarixini saqlash"_q },
		{ u"lng_fenix_save_edit_history_about"_q, u"Tahrirlangan xabarlarning oldingi versiyalarini saqlaydi, shunda xabar o'zgartirilishidan oldin nima deyilganini ko'rishingiz mumkin."_q },
		{ u"lng_fenix_edit_history"_q, u"Tahrir tarixi"_q },
		{ u"lng_fenix_edit_history_empty"_q, u"Bu xabar uchun oldingi versiyalar saqlanmagan."_q },
		{ u"lng_fenix_edit_history_current"_q, u"Joriy versiya"_q },
		{ u"lng_fenix_auto_translate"_q, u"Avto-tarjima"_q },
		{ u"lng_fenix_auto_translate_on"_q, u"Avto-tarjima: {lang}"_q },
		{ u"lng_fenix_auto_translate_off"_q, u"Avto-tarjimani o'chirish"_q },
		{ u"lng_fenix_auto_translate_choose"_q, u"Xabarlarni tarjima qilish tili"_q },
		{ u"lng_fenix_auto_translate_progress"_q, u"Tarjima qilinmoqda…"_q },
		{ u"lng_fenix_auto_translate_failed"_q, u"Tarjima qilib bo'lmadi. Asl matnni yuborish uchun Yuborish'ni qayta bosing."_q },
		{ u"lng_fenix_auto_translate_rich"_q, u"Avto-tarjima rich xabarlarda ishlamaydi. Tarjimasiz yuborish uchun Yuborish'ni qayta bosing."_q },
		{ u"lng_fenix_save_story"_q, u"Storini saqlash"_q },
		{ u"lng_fenix_story_download"_q, u"Istalgan storini yuklab olish"_q },
		{ u"lng_fenix_story_download_about"_q, u"Stori ko'ruvchi menyusiga 'Storini saqlash' bandini qo'shadi, bu esa Premium yoki muallif cheklovidan qat'i nazar istalgan odamning storisini yuklab olishga imkon beradi."_q },
		{ u"lng_fenix_section_data"_q, u"Ma'lumotlar"_q },
		{ u"lng_fenix_stop_auto_download"_q, u"Avtomatik yuklab olishni to'xtatish"_q },
		{ u"lng_fenix_stop_auto_download_about"_q, u"Suhbatni ochganingizda foto, video, audio va fayllar avtomatik yuklab olinishini to'xtatadi — trafik va xotirani tejaydi. Stikerlar baribir yuklanadi."_q },
		{ u"lng_fenix_chat_preview"_q, u"Ko'rib chiqish (o'qilmasdan)"_q },
		{ u"lng_fenix_section_interface"_q, u"Interfeys"_q },
		{ u"lng_fenix_hide_folder_tabs"_q, u"Papka yorliqlarini yashirish"_q },
		{ u"lng_fenix_hide_folder_tabs_about"_q, u"Bir nechta papkangiz bo'lsa ham, suhbatlar ro'yxatidan papka/filtr yorliqlari qatorini yashiradi."_q },
		{ u"lng_fenix_section_stories"_q, u"Storilar"_q },
		{ u"lng_fenix_hide_stories"_q, u"Storilarni yashirish"_q },
		{ u"lng_fenix_section_confirm"_q, u"Yuborishdan oldin tasdiqlash"_q },
		{ u"lng_fenix_confirm_voice_toggle"_q, u"Ovozli xabarlarni tasdiqlash"_q },
		{ u"lng_fenix_confirm_sticker_toggle"_q, u"Stikerlarni tasdiqlash"_q },
		{ u"lng_fenix_confirm_gif_toggle"_q, u"GIF'larni tasdiqlash"_q },
		{ u"lng_fenix_confirm_send_about"_q, u"Tanlangan turlarni yuborishdan oldin tasdiq so'raydi, shunda tasodifan yuborib yubormaysiz."_q },
		{ u"lng_fenix_confirm_voice"_q, u"Ushbu ovozli xabar yuborilsinmi?"_q },
		{ u"lng_fenix_confirm_sticker"_q, u"Ushbu stiker yuborilsinmi?"_q },
		{ u"lng_fenix_confirm_gif"_q, u"Ushbu GIF yuborilsinmi?"_q },
		{ u"lng_fenix_one_time_voice"_q, u"Bir martalik ovoz va video"_q },
		{ u"lng_fenix_one_time_voice_on"_q, u"Bir martalik ovoz va video: yoqilgan ✓"_q },
		{ u"lng_fenix_text_style_title"_q, u"Yozish"_q },
		{ u"lng_fenix_text_style"_q, u"Standart matn uslubi"_q },
		{ u"lng_fenix_text_style_about"_q, u"Tanlangan uslubni yuborishdan oldin butun xabarga qo'llaydi. O'z formatlashingizni saqlash uchun O'chiq'ni tanlang."_q },
		{ u"lng_fenix_text_style_off"_q, u"O'chiq"_q },
		{ u"lng_fenix_text_style_regular"_q, u"Oddiy (formatlashni tozalash)"_q },
		{ u"lng_fenix_text_style_bold"_q, u"Qalin"_q },
		{ u"lng_fenix_text_style_italic"_q, u"Kursiv"_q },
		{ u"lng_fenix_text_style_mono"_q, u"Monoshrift"_q },
		{ u"lng_fenix_text_style_strike"_q, u"Chizilgan"_q },
		{ u"lng_fenix_text_style_underline"_q, u"Tagiga chizilgan"_q },
		{ u"lng_fenix_text_style_spoiler"_q, u"Spoiler"_q },
		{ u"lng_fenix_text_style_quote"_q, u"Iqtibos"_q },
		{ u"lng_fenix_auto_text"_q, u"Avto-matn (imzo)"_q },
		{ u"lng_fenix_auto_text_on"_q, u"Avto-matn: yoqilgan ✓"_q },
		{ u"lng_fenix_auto_text_placeholder"_q, u"Bu yerda yuboradigan har bir xabaringizga qo'shiladigan matn"_q },
		{ u"lng_fenix_chat_id"_q, u"Chat ID"_q },
		{ u"lng_fenix_chat_id_copied"_q, u"ID nusxalandi"_q },
		{ u"lng_fenix_first_message"_q, u"Birinchi xabar"_q },
		{ u"lng_fenix_special_forward"_q, u"Maxsus uzatish"_q },
		{ u"lng_fenix_bots"_q, u"Novagram botlari"_q },
		{ u"lng_fenix_bots_media"_q, u"Media va yuklab olish"_q },
		{ u"lng_fenix_bots_editing"_q, u"Media tahrirlash"_q },
		{ u"lng_fenix_bots_tools"_q, u"Vositalar"_q },
		{ u"lng_fenix_bots_fun"_q, u"Foydali va qiziqarli"_q },
		{ u"lng_fenix_section_tools"_q, u"Vositalar"_q },
		{ u"lng_fenix_chat_finder"_q, u"Username bo'yicha suhbat topish"_q },
		{ u"lng_fenix_chat_finder_about"_q, u"Suhbatni ochish uchun @username (yoki t.me havolasi) kiriting, hatto hech qachon yozishmagan bo'lsangiz ham."_q },
		{ u"lng_fenix_chat_finder_hint"_q, u"username"_q },
		{ u"lng_fenix_chat_finder_open"_q, u"Ochish"_q },
		{ u"lng_fenix_chat_finder_available"_q, u"Mavjud"_q },
		{ u"lng_fenix_chat_finder_unavailable"_q, u"Mavjud emas"_q },
		{ u"lng_fenix_auto_answer_title"_q, u"Avto-javob"_q },
		{ u"lng_fenix_auto_answer"_q, u"Kiruvchi suhbatlarga avto-javob"_q },
		{ u"lng_fenix_auto_answer_text"_q, u"Avto-javob matni"_q },
		{ u"lng_fenix_auto_answer_placeholder"_q, u"Yangi suhbatlarga avtomatik yuboriladigan javob"_q },
		{ u"lng_fenix_auto_answer_about"_q, u"Har bir shaxsiy suhbatning birinchi kiruvchi xabariga matningiz bilan bir marta javob beradi. Suhbatni ochganingizda qayta faollashadi."_q },
		{ u"lng_fenix_join_requests"_q, u"A'zolik so'rovlari"_q },
		{ u"lng_fenix_join_requests_none"_q, u"Kutilayotgan so'rovlar yo'q."_q },
		{ u"lng_fenix_join_requests_approve"_q, u"Hammasini qabul qilish"_q },
		{ u"lng_fenix_join_requests_decline"_q, u"Hammasini rad etish"_q },
		{ u"lng_fenix_join_requests_approved"_q, u"So'rovlar qabul qilindi."_q },
		{ u"lng_fenix_join_requests_declined"_q, u"So'rovlar rad etildi."_q },
		{ u"lng_fenix_join_requests_error"_q, u"Xatolik yuz berdi."_q },
		{ u"lng_fenix_join_all_accounts"_q, u"Barcha akkauntlar bilan qo'shilish"_q },
		{ u"lng_fenix_join_all_progress"_q, u"Barcha akkauntlar bilan qo'shilmoqda…"_q },
		{ u"lng_fenix_join_all_none"_q, u"Qo'shilish uchun boshqa akkaunt yo'q."_q },
		{ u"lng_fenix_features_keywords"_q, u"sharpa,yashirin,oflayn,fenix"_q },
		{ u"lng_fenix_bot_login"_q, u"Bot tokeni bilan kirish"_q },
		{ u"lng_fenix_bot_token"_q, u"Bot tokeni"_q },
		{ u"lng_fenix_bot_desc"_q, u"Bot sifatida kirish uchun @BotFather'dan olgan tokenni joylashtiring."_q },
		{ u"lng_fenix_bot_invalid"_q, u"Bot tokeni noto'g'ri. Uni @BotFather'dan nusxalang (123456789:AA... ko'rinishida)."_q },
		{ u"lng_fenix_chat_lock"_q, u"Suhbatni qulflash"_q },
		{ u"lng_fenix_chat_unlock"_q, u"Suhbatni ochish"_q },
		{ u"lng_fenix_chat_locked"_q, u"Suhbat qulflandi"_q },
		{ u"lng_fenix_chat_unlocked"_q, u"Suhbat ochildi"_q },
		{ u"lng_fenix_chat_lock_title"_q, u"Qulflangan suhbat"_q },
		{ u"lng_fenix_chat_lock_enter"_q, u"Parol"_q },
		{ u"lng_fenix_chat_lock_confirm"_q, u"Parolni tasdiqlang"_q },
		{ u"lng_fenix_chat_lock_set"_q, u"Parol o'rnatish"_q },
		{ u"lng_fenix_chat_lock_open"_q, u"Ochish"_q },
		{ u"lng_fenix_chat_lock_how"_q, u"Bu suhbatni qanday qulflaysiz?"_q },
		{ u"lng_fenix_chat_lock_common"_q, u"Umumiy parol"_q },
		{ u"lng_fenix_chat_lock_separate"_q, u"Alohida parol"_q },
		{ u"lng_fenix_chat_lock_continue"_q, u"Davom etish"_q },
		{ u"lng_fenix_reminder_title"_q, u"Eslatmalar"_q },
		{ u"lng_fenix_reminder"_q, u"O'qilmagan xabar eslatmasi"_q },
		{ u"lng_fenix_reminder_delay"_q, u"Eslatma kechikishi (daqiqa)"_q },
		{ u"lng_fenix_reminder_delay_ph"_q, u"Daqiqa (1–59)"_q },
		{ u"lng_fenix_chat_lock_section"_q, u"Suhbat qulfi"_q },
		{ u"lng_fenix_chat_lock_change_common"_q, u"Umumiy parolni o'zgartirish"_q },
		{ u"lng_fenix_chat_lock_changed"_q, u"Umumiy parol o'zgartirildi"_q },
		{ u"lng_fenix_ocr_scan"_q, u"Matnni skanerlash"_q },
		{ u"lng_fenix_ocr_title"_q, u"Matnni skanerlash"_q },
		{ u"lng_fenix_ocr_recognizing"_q, u"Tanilmoqda…"_q },
		{ u"lng_fenix_ocr_empty"_q, u"Matn topilmadi"_q },
		{ u"lng_fenix_ocr_copy"_q, u"Nusxalash"_q },
		{ u"lng_fenix_ocr_copied"_q, u"Matn nusxalandi"_q },
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
		{ u"lng_fenix_join_requests_none"_q, u"Нет ожидающих заявок."_q },
		{ u"lng_fenix_join_requests_approve"_q, u"Принять все"_q },
		{ u"lng_fenix_join_requests_decline"_q, u"Отклонить все"_q },
		{ u"lng_fenix_join_requests_approved"_q, u"Заявки приняты."_q },
		{ u"lng_fenix_join_requests_declined"_q, u"Заявки отклонены."_q },
		{ u"lng_fenix_join_requests_error"_q, u"Что-то пошло не так."_q },
		{ u"lng_fenix_join_all_accounts"_q, u"Вступить со всеми аккаунтами"_q },
		{ u"lng_fenix_join_all_progress"_q, u"Вступление со всеми аккаунтами…"_q },
		{ u"lng_fenix_join_all_none"_q, u"Нет других аккаунтов для вступления."_q },
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
	};
	return data;
}

[[nodiscard]] const Table *TableForCurrentLanguage() {
	const auto id = Lang::GetInstance().id();
	const auto code = id.split('-').front();
	if (code == u"uz"_q) {
		return &Uzbek();
	} else if (code == u"ru"_q) {
		return &Russian();
	}
	return nullptr;
}

void Apply() {
	const auto table = TableForCurrentLanguage();
	if (!table) {
		// Other languages keep the compiled-in English base.
		return;
	}
	auto &instance = Lang::GetInstance();

	auto strings = QVector<MTPLangPackString>();
	strings.reserve(int(table->size()));
	for (const auto &[key, value] : *table) {
		strings.push_back(MTP_langPackString(MTP_string(key), MTP_string(value)));
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

	Apply();

	// Re-inject whenever the user switches language. idChanges fires after the
	// instance has reset its values to the English base, so our strings layer
	// cleanly on top. (We deliberately do NOT listen to updated() — Apply()
	// itself fires it, which would recurse.)
	Lang::GetInstance().idChanges(
	) | rpl::on_next([](const QString &) {
		Apply();
	}, lifetime);
}

} // namespace Fenix
