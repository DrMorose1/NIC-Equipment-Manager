from pathlib import Path

src = Path(__file__).resolve().parents[1] / 'src' / 'main.c'
layout = Path(__file__).resolve().parents[1] / 'src' / 'ui_layout.c'
main_text = src.read_text(encoding='utf-8')
layout_text = layout.read_text(encoding='utf-8')

assert 'h_tools_status' in main_text, 'Advanced Tools main-panel status label is missing'
assert '10, 534, 1145, 94' in main_text, 'Advanced Tools group should leave breathing room around all three status rows'
assert '25, 600, 735, 20' in main_text, 'Advanced Tools status row should occupy the third line with title clearance'
assert 'scaled(layout, 634)' in layout_text, 'Activity Log should move down when Advanced Tools is visible'
print('main layout source checks passed')
