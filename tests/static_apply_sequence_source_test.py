from pathlib import Path
p = Path(__file__).resolve().parents[1] / 'src' / 'main.c'
s = p.read_text(encoding='utf-8')
reset = s.find('network_build_static_reset_preamble(')
primary = s.find('network_build_netsh_static_primary(')
assert reset != -1, 'static apply path must call network_build_static_reset_preamble'
assert primary != -1, 'static apply path must call network_build_netsh_static_primary'
assert reset < primary, 'DHCP/reset preamble must be generated before static primary address command'
print('static apply sequence source test passed')
