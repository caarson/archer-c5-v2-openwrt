#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Check or apply original-layout source integration to an exact clean checkout.
Never builds/deploys firmware. Historical integration is not release-ready.
"""
from pathlib import Path
import subprocess,json,argparse,sys
R=Path(__file__).resolve().parents[1];BASE='64900e756efa667774214f9c6b17c113222e01f7'
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--apply',action='store_true');a=p.parse_args();root=a.source.resolve()
def git(*args):return subprocess.run(['git','-C',str(root),*args],capture_output=True,text=True)
r=git('rev-parse','--show-toplevel')
if r.returncode or Path(r.stdout.strip()).resolve()!=root:sys.exit('Refusing: source must be a separate Git repository root.')
r=git('rev-parse','HEAD')
if r.returncode or r.stdout.strip()!=BASE:sys.exit('Refusing: source HEAD does not match the pinned OpenWrt base.')
r=git('status','--porcelain','--untracked-files=normal')
if r.returncode or r.stdout.strip():sys.exit('Refusing: source checkout must be clean; existing work will not be overwritten.')
patch=R/'patches/openwrt/0001-original-layout-integration.patch'
r=git('apply','--check',str(patch))
if r.returncode:sys.exit('Patch check failed:\n'+r.stderr)
if a.apply:
 r=git('apply',str(patch))
 if r.returncode:sys.exit(r.stderr)
print(json.dumps({'PASS':True,'base_commit':BASE,'source_patch_checked':True,'source_patch_applied':a.apply,'build_run':False,'router_access':False,'warning':'Source review only; historical packager/config drift and clean binary release prerequisites remain in docs/BUILD.md.'},indent=2))
