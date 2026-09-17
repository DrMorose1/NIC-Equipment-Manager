from pathlib import Path

src = (Path(__file__).parents[1] / 'src' / 'main.c').read_text(encoding='utf-8')

assert 'advanced_output_subclass_proc' in src, 'Advanced output needs a scroll repaint subclass.'
assert 'SetWindowSubclass(h_adv_output, advanced_output_subclass_proc' in src, 'Advanced output control must install the repaint subclass.'
assert 'RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW' in src, 'Scroll repaint must erase stale pixels before repainting.'
assert 'WM_VSCROLL' in src and 'WM_HSCROLL' in src and 'WM_MOUSEWHEEL' in src, 'Subclass must handle normal scroll paths.'
print('advanced output scroll repaint source checks passed')
