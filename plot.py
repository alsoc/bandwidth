#! /usr//bin/env python

import argparse
import sys
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

class CSV(object):
  def __init__(self, name = None, **kwargs):
    self.__name = name
    self.__file = None
    self.__header = kwargs.pop("header", [])
  
  @property
  def name(self):
    return self.__name

  @property
  def header(self):
    return self.__header

  @staticmethod
  def __guess_type(s):
    if not s:
      return None
    try:
      return int(s)
    except ValueError:
      pass
    try:
      return float(s)
    except ValueError:
      pass
    return s

  def __enter__(self):
    if self.__file is not None:
      raise RuntimeError("CSV \'{}\' is already opened".format(self.__name))
    self.__file = open(self.name, 'r')
    line = self.__file.readline()
    if line[-1] == '\n':
      line = line[:-1]
    self.__header = line.split(',')
    return self

  def __exit__(self, type, value, traceback):
    if self.__file is None:
      raise RuntimeError("CSV \'{}\' is not opened and cannot be closed".format(self.__name))
    self.__file.close()
    self.__file = None

  def __iter__(self):
    if self.__file is None:
      raise RuntimeError("CSV must be within a context block to be iterated on")
    def create_dict(row):
      d = dict()
      for h, r in zip(self.header, row):
        d[h] = r
      return d

    for line in self.__file:
      if line[-1] == '\n':
        line = line[:-1]
      row = (CSV.__guess_type(e) for e in line.split(','))
      yield create_dict(row)


  def __repr__(self):
    return "CSV({!r})".format(self.name)


filename = 'bw.csv'

inf = float("inf")
xmin =  inf
xmax = -inf
ymin =  inf
ymax = -inf

ylog = True

parser = argparse.ArgumentParser(description='Plots the bandwidth of a machine from CSV file')
parser.add_argument('filename', type=str, help='CSV file', action='store', default='/dev/stdin')
parser.add_argument('--L1', type=float, default=0., help='L1 cache size')
parser.add_argument('--L2', type=float, default=0., help='L2 cache size')
parser.add_argument('--L3', type=float, default=0., help='L3 cache size')
parser.add_argument('--L4', type=float, default=0., help='L4 cache size')
group = parser.add_mutually_exclusive_group()
group.add_argument('-l', '--linear', help='linear scale for Y axis', action='store_true')
group.add_argument('-L', '--log', help='log scale for Y axis', action='store_true')
parser.add_argument('-m', '--machine', type=str, default='', help='Machine name')
parser.add_argument('-t', '--title', type=str, default='', help='Plot title')

args = parser.parse_args()

raw_caches = [0., args.L1, args.L2, args.L3, args.L4]
caches = [0.]

last_cache = 0

for i in range(1, len(raw_caches)):
  c = raw_caches[i]
  if not c:
    continue
  if last_cache != i-1:
    sys.stderr.write('L{} cannot be set if L{} is not set\n'.format(i, i-1))
    exit(1)
  if c <= raw_caches[last_cache]:
    sys.stderr.write('L{} must be bigger than L{}\n'.format(i, i-1))
    exit(1)
  last_cache = i
  caches.append(c)

ylog = not args.linear
machine_name = args.machine
title = args.title or "{} bandwidth".format(machine_name)

series = dict()


csv = CSV(args.filename)
ignore = {'type'}
Xname = ''
X = []
with csv:
  for n in csv.header:
    series[n] = []
  Xname = next((h for h in csv.header if h not in ignore))
  for row in csv:
    x = row[Xname]
    xmin = min(xmin, x)
    xmax = max(xmax, x)
    X.append(x)
    for n in csv.header:
      if n != Xname and n not in ignore and n in row:
        val = row[n]
        ymin = min(ymin, val)
        ymax = max(ymax, val)
        series[n].append(val)

fig = plt.figure()
ax = plt.subplot(111)

plt.title(title)
plt.xlabel(Xname)
plt.ylabel("bandwidth")

for n in (h for h in csv.header if h not in ignore and h != Xname):
  ax.plot(X, series[n], label=n)

ax.set_xlim(xmin, xmax)
plt.xscale("log")

if ylog:
  ax.set_ylim(ymin/1.2, ymax*1.2)
  plt.yscale("log")


def bytes_fmt(v, *args):
  if v == 0.:
    return "0 B"
  letters = 'fpnum KMGTPE'
  i = letters.index(' ')
  while (i > 0 and v < 0.999):
    v *= 1000.
    i -= 1
  while (i < len(letters)-1 and v > 999.):
    v /= 1000.
    i += 1
  return "{:g} {}B".format(v, letters[i])
formatter = ticker.FuncFormatter

ax.xaxis.set_major_formatter(ticker.FuncFormatter(bytes_fmt))
ax.yaxis.set_minor_formatter(ticker.FuncFormatter(lambda v, pos: bytes_fmt(v, pos) + "/s"))
ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda v, pos: bytes_fmt(v, pos) + "/s"))

if len(caches) > 1:
  lastx = xmin
  if not ylog:
    ymin /= 2

  a = 0.1

  colors = ['black', 'green', 'yellow', 'orange', 'red']
  for i in range(1, len(caches)):
    x = caches[i]
    ax.axvspan(lastx, x, alpha=a, color=colors[i])
    plt.text((lastx*x)**0.5, ymin*0.9, "L{}".format(i))
    lastx = x
  ax.axvspan(lastx, xmax, alpha=a, color=colors[len(caches)])
  plt.text((lastx*xmax)**0.5, ymin*0.9, "RAM".format(i))


ax.grid(b=True, which='major', color="black", alpha=0.5, linestyle=':', linewidth=1.)
ax.grid(b=True, which='minor', color="black", alpha=0.2, linestyle=':', linewidth=1.)

plt.legend()
plt.show()
