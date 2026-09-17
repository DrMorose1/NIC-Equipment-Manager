from pathlib import Path
s = (Path(__file__).resolve().parents[1] / 'src' / 'main.c').read_text(encoding='utf-8')
static_branch = s[s.find('if (settings.dhcp) {'):s.find('SetCursor(LoadCursor', s.find('if (settings.dhcp) {'))]
assert 'network_build_persistent_dhcp(' in static_branch, 'apply path must update persistent EnableDHCP state'
assert 'network_build_static_verification(' in static_branch, 'static apply path must verify DHCP/address readback'
assert 'network_build_dhcp_verification(' in static_branch, 'DHCP apply path must verify DHCP readback'
# Must set persistent DHCP before the static reset / address commands.
persist = static_branch.rfind('network_build_persistent_dhcp(', 0, static_branch.find('network_build_static_reset_preamble('))
reset = static_branch.find('network_build_static_reset_preamble(')
assert persist != -1 and persist < reset, 'static path must set EnableDHCP=0 before reset/static address commands'
print('persistent DHCP sequence source test passed')
