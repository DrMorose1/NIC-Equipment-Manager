from pathlib import Path

text = (Path(__file__).resolve().parents[1] / 'src' / 'ui_layout.c').read_text(encoding='utf-8')

# Regression: maximized windows can have a client width below the design/base width
# (for example on a smaller work area or with DPI scaling). The layout must be
# allowed to receive a negative delta so controls contract instead of clipping.
assert 'dx = client_width - layout->base_width;' in text
assert 'if (dx < 0) dx = 0;' not in text, 'negative width deltas are still being clamped, causing clipping'

print('narrow layout source regression test passed')
