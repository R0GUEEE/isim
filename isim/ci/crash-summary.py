#!/usr/bin/env python3
"""Prints the useful part of a Darwin-style .ips crash report: the exception, the first thread's frames, the images.

    crash-summary.py FILE.ips [FRAMES]
"""
import json
import sys

raw = open(sys.argv[1]).read()
body = raw[raw.index('\n') + 1:]                       # the first line is the {"app_name":…,"timestamp":…} header
d = json.loads(body)
n = int(sys.argv[2]) if len(sys.argv) > 2 else 24
print('exception:', d.get('exception'), d.get('termination', {}).get('indicator'), d.get('asi'))
for t in d.get('threads', [])[:1]:
    print('thread:', t.get('name') or t.get('queue') or 'main')
    for fr in t.get('frames', [])[:n]:
        print('   %2s %s %s' % (fr.get('imageIndex'), hex(fr.get('imageOffset', 0)), fr.get('symbol', '')))
print('images:', [i.get('name') for i in d.get('usedImages', [])])
