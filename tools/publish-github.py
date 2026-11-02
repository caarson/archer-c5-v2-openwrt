#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Default: report a publication plan. --publish creates a public source repo.
Run the publish option only after the owner has approved this exact preview.
Never posts to a forum, publishes firmware binaries, or touches a router.
"""
from pathlib import Path
import argparse,subprocess,json,re,sys
R=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--publish',action='store_true');a=p.parse_args()
repo='archer-c5-v2-openwrt';description='Experimental Archer C5 v2 OpenWrt 25.12.6 / BCM4352 b43 AC-PHY: source prerelease, patches and measured limitations.'
def call(args,check=True):
 r=subprocess.run(args,cwd=R,capture_output=True,text=True)
 if check and r.returncode:sys.exit(r.stderr or r.stdout or 'Command failed')
 return r
v=call([sys.executable,str(R/'tools/verify-preview.py')]);print(v.stdout,end='')
auth=call(['gh','auth','status'],False)
if not a.publish:
 print(json.dumps({'mode':'plan only','repo_name':repo,'visibility':'public','description':description,'github_authenticated':auth.returncode==0,'forum_post':'Separate owner-reviewed forum action after actual repository URL exists.','binary_release':False,'next_command_after_login_and_publication_approval':'python3 tools/publish-github.py --publish'},indent=2));sys.exit(0)
if auth.returncode:sys.exit('GitHub is not authenticated. Run gh auth login first; do not paste tokens into this repository.')
u=json.loads(call(['gh','api','user']).stdout);owner=u['login']
if not re.fullmatch(r'[A-Za-z0-9-]+',owner):sys.exit('Unexpected GitHub account identifier')
full=owner+'/'+repo
if call(['gh','repo','view',full,'--json','nameWithOwner'],False).returncode==0:sys.exit('Refusing: destination repository already exists; review it instead of overwriting.')
if call(['git','remote'],False).stdout.strip():sys.exit('Refusing: this preview already has a remote; review it first.')
call(['git','add','--all'])
if call(['git','rev-parse','--verify','HEAD'],False).returncode!=0:
 call(['git','-c','user.name='+str(u.get('name')or owner),'-c','user.email='+str(u['id'])+'+'+owner+'@users.noreply.github.com','commit','-m','Prepare experimental Archer C5 v2 source prerelease'])
elif call(['git','status','--porcelain']).stdout.strip():sys.exit('Refusing: commit and review changes before publishing an existing history.')
# Explicitly invoked public source publication only. No executable release assets.
call(['gh','repo','create',full,'--public','--description',description,'--source',str(R),'--remote','origin'])
# Use the authenticated CLI for this push only; do not change global Git helpers.
call(['git','-c','credential.helper=','-c','credential.helper=!gh auth git-credential','push','--set-upstream','origin','main'])
url='https://github.com/'+full
post=(R/'FORUM-DRAFT.md').read_text().replace('REPOSITORY_URL',url)
(R.parent/'forum-post-ready.md').write_text(post)
receipt={'repository_url':url,'commit':call(['git','rev-parse','HEAD']).stdout.strip(),'firmware_binary_uploaded':False,'forum_posted':False}
(R.parent/'publication-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
