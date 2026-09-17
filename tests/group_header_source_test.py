from pathlib import Path
text = (Path(__file__).resolve().parents[1] / 'src' / 'main.c').read_text(encoding='utf-8')
# Section headers must no longer rely on the native BS_GROUPBOX caption renderer,
# which can paint the border through caption text in dark/high-DPI configurations.
assert 'make_groupbox(' in text, 'expected custom main-window groupbox helper'
assert 'make_advanced_groupbox(' in text, 'expected custom advanced-window groupbox helper'
assert 'SetPropA(label, "NIC_GROUP_HEADER"' in text, 'expected group-header marker'
assert 'GetPropA((HWND)lparam, "NIC_GROUP_HEADER")' in text, 'expected opaque group-header painting'
