from pathlib import Path
src = Path(__file__).resolve().parents[1] / 'src' / 'main.c'
text = src.read_text(encoding='utf-8')
assert '#define MAIN_MIN_CLIENT_WIDTH 1180' in text
assert '#define MAIN_MIN_CLIENT_HEIGHT 745' in text
assert 'static void get_main_min_track_size' in text
assert 'AdjustWindowRectEx' in text or 'AdjustWindowRectExForDpi' in text
assert 'get_main_min_track_size(hwnd, &limits->ptMinTrackSize);' in text
print('min-window source checks passed')
