#!/usr/bin/env python3
"""Generate pinned Unicode 17 grapheme tables from official UCD text files.
Input: GraphemeBreakProperty.txt, emoji-data.txt, DerivedCoreProperties.txt.
Unicode data is covered by Unicode-LICENSE.txt; generator is BSD-3-Clause.
"""
import argparse
from pathlib import Path
parser = argparse.ArgumentParser()
parser.add_argument('data_dir', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
properties = {'CR':1,'LF':2,'Control':3,'Extend':4,'ZWJ':5,'Regional_Indicator':6,'Prepend':7,'SpacingMark':8,'L':9,'V':10,'T':11}
def ranges(filename, choose):
    result=[]
    for line in (args.data_dir/filename).read_text(encoding='utf-8').splitlines():
        fields=[part.strip() for part in line.split('#')[0].split(';')]
        if len(fields)<2: continue
        value=choose(fields[1:])
        if not value: continue
        bounds=fields[0].split('..'); first=int(bounds[0],16); last=int(bounds[-1],16)
        result.append((first,last,value))
    merged=[]
    for first,last,value in sorted(result):
        if merged and merged[-1][1]+1==first and merged[-1][2]==value:
            merged[-1]=(merged[-1][0],last,value)
        else: merged.append((first,last,value))
    return merged
sets={
 'GraphemeRanges':ranges('GraphemeBreakProperty.txt',lambda p:properties.get(p[0],0)),
 'ExtendedPictographicRanges':ranges('emoji-data.txt',lambda p:1 if p[0]=='Extended_Pictographic' else 0),
 'IndicConjunctRanges':ranges('DerivedCoreProperties.txt',lambda p:{'Consonant':1,'Extend':2,'Linker':3}.get(p[1],0) if len(p)>1 and p[0]=='InCB' else 0),
}
text='''// Generated Unicode 17.0.0 data. Do not edit by hand.
// Sources: https://www.unicode.org/Public/17.0.0/ucd/
// Copyright Unicode, Inc. See Unicode-LICENSE.txt (Unicode-3.0).
#pragma once
#include <cstddef>
namespace conpty_unicode {
struct Range { unsigned first, last; unsigned char value; };
'''
for name,data in sets.items():
    text+=f'constexpr Range {name}[] = {{\n'
    text+=''.join(f'    {{0x{a:X}, 0x{b:X}, {v}}},\n' for a,b,v in data)
    text+='};\n'
text+='}\n'
args.output.write_bytes(text.encode('utf-8'))
print(', '.join(f'{key}: {len(value)} intervals' for key,value in sets.items()))
