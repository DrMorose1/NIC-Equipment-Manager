from pathlib import Path
text = (Path(__file__).resolve().parents[1] / 'src' / 'main.c').read_text(encoding='utf-8')
assert 'measure_group_header_width' in text, 'headers should size from rendered text rather than fixed guessed widths'
assert 'SS_CENTER | SS_CENTERIMAGE' in text, 'header text should be centered inside its masked border gap'
assert 'title_width + 8' not in text, 'fixed header gap widths leave uneven trailing space'
assert 'make_groupbox("Network Adapter Status", 10, 8, 1145, 88, hwnd, ID_GROUP_HEADER_ADAPTER)' in text
assert 'make_advanced_groupbox("Current Context", 10, 8, 840, 70, hwnd, ID_ADV_GROUP_CONTEXT, ID_ADV_HEADER_CONTEXT)' in text
