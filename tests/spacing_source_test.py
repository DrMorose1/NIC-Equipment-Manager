from pathlib import Path
text = (Path(__file__).resolve().parents[1] / 'src' / 'main.c').read_text(encoding='utf-8')
layout = (Path(__file__).resolve().parents[1] / 'src' / 'ui_layout.c').read_text(encoding='utf-8')

# High-DPI readability: labels should not terminate exactly where the following
# edit/combo begins. These are representative first-row and device-row gutters.
assert '80, 28, 385, 300' not in text, 'Adapter combo still has zero gutter after Adapter label'
assert '115, 123, 210, 250' not in text, 'Saved profile combo still has zero gutter after label'
assert '355, 224, 110, 24' not in text, 'Prefix field still has zero gutter after label'
assert '690, 359, 105, 24' not in text, 'Device prefix field still has zero gutter after label'
assert '870, 359, 90, 120' not in text, 'Protocol combo still has zero gutter after label'

# Advanced Tools content must start below the custom header overlay rather than
# intersecting its vertical band.
assert '25, 548, 735, 20' not in text, 'Advanced NIC context still crowds the title'

# Activity Log content should have a slightly larger title-to-list gap.
assert 'log_y + scaled(layout, 22)' not in layout, 'Activity log list still crowds the title'
print('spacing source regression test passed')
