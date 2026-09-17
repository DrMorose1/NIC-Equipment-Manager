from pathlib import Path

src = (Path(__file__).resolve().parents[1] / 'src' / 'ui_layout.c').read_text(encoding='utf-8')

# The second column in the IPv4 Configuration panel starts with labels at x=275.
# The resize rule must therefore move that entire column, not just controls at x>=330.
needle = 'else if (item->x >= scaled(layout, 275)) x += half;'
assert needle in src, (
    'IPv4 second-column resize rule must start at x=275 so Prefix/mask, DNS 1, '
    'and DNS 2 labels move together with their edit controls.'
)
print('left form column alignment source test passed')
