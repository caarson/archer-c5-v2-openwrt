#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Verify the explicitly curated source preview. No network/device access."""
from pathlib import Path
import ast,hashlib,json,re,sys
R=Path(__file__).resolve().parents[1]
errors=[];count=0
manifest=json.loads((R/'manifests/source-files.json').read_text())
manifest.update(json.loads((R/'manifests/release-source-files.json').read_text()))
for name,meta in manifest.items():
 p=R/name
 if not p.is_file() or p.is_symlink() or hashlib.sha256(p.read_bytes()).hexdigest()!=meta['sha256']:errors.append('source identity mismatch: '+name)
for p in R.rglob('*'):
 if '.git' in p.relative_to(R).parts or p.is_dir():continue
 count+=1;name=str(p.relative_to(R))
 if p.is_symlink():errors.append('symlink not allowed: '+name);continue
 if p.suffix in {'.bin','.ko','.fw','.dtb','.trx','.img','.pcap','.pcapng','.key','.pem'}:errors.append('binary/private artifact excluded: '+name)
 if p.stat().st_size>4*1024*1024:errors.append('oversized file: '+name);continue
 try:s=p.read_text()
 except UnicodeDecodeError:errors.append('nontext payload: '+name);continue
 # Generic source-only checks; no secret literals are embedded in this scanner.
 patterns=[r'-----BEGIN (?:OPENSSH |RSA |EC |DSA )?PRIVATE KEY-----',r'(?:ghp_|github_pat_)[A-Za-z0-9_]{20,}',r'/home/[A-Za-z0-9_-]+/',r'/Users/[A-Za-z0-9_-]+/',r'(?im)^\s*option\s+(?:key|password)\s+[\'\"][^\'\"]{5,}[\'\"]']
 for pat in patterns:
  if re.search(pat,s):errors.append('possible private material, review '+name)
 if p.suffix=='.py':
  try:ast.parse(s)
  except SyntaxError as e:errors.append('invalid Python: '+name+': '+str(e))
if not list((R/'source/installed-b43').glob('*.c')):errors.append('installed source missing')
res={'PASS':not errors,'text_files_checked':count,'source_identities_checked':len(manifest),'errors':errors,'limits':'Allowlist/hash/pattern review; not a proof of complete secret detection, licensing clearance or firmware reproducibility.'}
print(json.dumps(res,indent=2));sys.exit(bool(errors))
