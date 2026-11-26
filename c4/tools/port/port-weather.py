#!/usr/bin/env python3
# port-weather.py ROOT: CWeather's member functions have no return type
# (old C++'s implicit int) and return nothing. Watcom left eax; under ISO
# C++, falling off the end of a function that returns a value is undefined,
# and gcc -O2 makes RandomizeVectors' loop run past its array. They are
# void now. And deletesprite_replace returns the engine's int, 0.
import os, re, sys
R = sys.argv[1]
names = ['RandomizeVectors', 'SetDefaultBuffer', 'SetParticles', 'SetTranslucency',
         'SetColor', 'SetColorShift', 'Initialize', 'Draw', 'SetWeatherType']
p = os.path.join(R, 'source/blood/weather.h')
s = open(p, encoding='latin-1').read()
n = 0
for name in names:
    s, k = re.subn(r'(\n\s+)(%s\s*\()' % name, r'\1void \2', s)
    n += k
open(p, 'w', encoding='latin-1', newline='').write(s)
p = os.path.join(R, 'source/blood/weather.cpp')
s = open(p, encoding='latin-1').read()
m = 0
for name in names:
    s, k = re.subn(r'\nCWeather::(%s\s*\()' % name, r'\nvoid CWeather::\1', s)
    m += k
open(p, 'w', encoding='latin-1', newline='').write(s)
print('weather.h', n, 'weather.cpp', m)
p = os.path.join(R, 'source/blood/db.cpp')
s = open(p, encoding='latin-1').read()
i = s.index('extern "C" int deletesprite_replace(short a1)\n{')
j = s.index('\n}\n', i)
s = s[:j] + '\n    return 0;  // c4: the engine\'s hook returns int' + s[j:]
open(p, 'w', encoding='latin-1', newline='').write(s)
print('ok')
