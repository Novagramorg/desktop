"""Check Novagram's Uzbek translation, for example after an upstream sync.

Telegram's Uzbek desktop pack ("uz-beta" in the app, "uz" on
translations.telegram.org) misses part of lang.strings. novagram_uz.strings
fills those keys and fenix/fenix_lang.cpp translates the lng_fenix_* ones.
This script lists what is still untranslated and checks both for errors.

  python novagram_uz_check.py                    download the pack and report
  python novagram_uz_check.py --pack uz.strings  use an already downloaded pack
  python novagram_uz_check.py --todo todo.strings
                                                 also write the missing keys
                                                 with their English text
Exits with 1 when something is missing or broken.
"""
import argparse, os, re, sys, urllib.request

EXPORT_URL = 'https://translations.telegram.org/uz/tdesktop/export'
OVERRIDES_MARKER = '// Overrides'

here = os.path.dirname(os.path.abspath(__file__))
pair_re = re.compile(r'"([A-Za-z0-9_#]+)"\s*=\s*"((?:[^"\\]|\\.)*)"\s*;', re.S)
tag_re = re.compile(r'\{([A-Za-z0-9_]+)\}')

def base(key):
    return key.split('#')[0]

def pairs(text):
    return pair_re.findall(text)

def read(path):
    with open(path, encoding='utf-8') as f:
        return f.read()

def load_pack(path):
    if path:
        return read(path)
    print(f'Downloading {EXPORT_URL}')
    with urllib.request.urlopen(EXPORT_URL, timeout=60) as response:
        text = response.read().decode('utf-8')
    if '"lng_language_name"' not in text:
        sys.exit('The download is not a .strings file.')
    return text

def fenix_uzbek_keys(source):
    block = re.search(r'const Table &Uzbek\(\) \{(.*?)\n\}', source, re.S)
    return set(re.findall(r'\{ u"(lng_fenix_[A-Za-z0-9_#]+)"_q', block.group(1)))

def check_values(name, values, english, errors, warnings):
    for key, value in values:
        if key not in english:
            continue
        allowed = set(tag_re.findall(english[key]))
        used = tag_re.findall(value)
        for tag in sorted(set(used) - allowed):
            errors.append(f'{name}: {key}: unknown tag {{{tag}}}')
        if len(used) != len(set(used)):
            errors.append(f'{name}: {key}: repeated tag')
        for escape in re.findall(r'\\(.)', value):
            if escape not in ('n', '"', '\\'):
                errors.append(f'{name}: {key}: unknown escape \\{escape}')
        if re.search(r'[Ѐ-ӿ]', value):
            errors.append(f'{name}: {key}: Cyrillic letters')
        if set(used) != allowed:
            warnings.append(f'{name}: {key}: tags differ from English, '
                'maybe the English text changed')

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pack', help='downloaded Uzbek tdesktop export')
    parser.add_argument('--todo', help='write missing keys here')
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding='utf-8')

    english_list = pairs(read(os.path.join(here, 'lang.strings')))
    english = dict(english_list)
    pack_list = pairs(load_pack(args.pack))
    pack = {base(k) for k, _ in pack_list}
    ours_text = read(os.path.join(here, 'novagram_uz.strings'))
    ours = pairs(ours_text)
    fill_text = ours_text.split(OVERRIDES_MARKER)[0]
    fill = {base(k) for k, _ in pairs(fill_text)}
    ours_keys = {base(k) for k, _ in ours}
    fenix = fenix_uzbek_keys(read(os.path.join(
        here, '..', '..', 'SourceFiles', 'fenix', 'fenix_lang.cpp')))

    errors, warnings = [], []
    missing = [(k, v) for k, v in english_list
        if not k.startswith('lng_fenix_')
        and base(k) not in pack
        and base(k) not in ours_keys]
    # The app rejects a cloud value with a tag the English text no longer
    # has and shows English instead, so those keys need an override.
    overridden = {k for k, _ in ours}
    rejected = [(k, english[k]) for k, v in pack_list
        if k in english
        and k not in overridden
        and not set(tag_re.findall(v)) <= set(tag_re.findall(english[k]))]
    missing_fenix = [k for k, _ in english_list
        if k.startswith('lng_fenix_') and k not in fenix]
    stale = sorted({base(k) for k, _ in ours} - {base(k) for k in english})
    check_values('novagram_uz.strings', ours, english, errors, warnings)
    now_in_pack = sorted(fill & pack)

    for key, value in missing:
        print(f'MISSING  {key} = "{value}"')
    for key, value in rejected:
        print(f'REJECTED {key} = "{value}" (the cloud value has old tags)')
    for key in missing_fenix:
        print(f'MISSING  {key} (Uzbek table in fenix_lang.cpp)')
    for key in stale:
        print(f'STALE    {key} is gone from lang.strings, remove it')
    for line in errors:
        print(f'ERROR    {line}')
    for line in warnings:
        print(f'WARNING  {line}')
    if now_in_pack:
        print(f'INFO     the cloud pack now also translates {len(now_in_pack)}'
            ' of our keys; ours still win, drop them to use the cloud text:')
        print('         ' + ', '.join(now_in_pack))

    if args.todo:
        with open(args.todo, 'w', encoding='utf-8', newline='\n') as f:
            for key, value in missing + rejected:
                f.write(f'"{key}" = "{value}";\n')

    upstream = {base(k) for k in english if not k.startswith('lng_fenix_')}
    covered = len(upstream & (pack | ours_keys))
    print(f'Uzbek covers {covered} of {len(upstream)} Telegram strings, '
        f'{len(missing)} missing, {len(rejected)} rejected; '
        f'{len(missing_fenix)} Novagram strings missing; '
        f'{len(errors)} errors.')
    sys.exit(1 if missing or rejected or missing_fenix or stale or errors
        else 0)

main()
