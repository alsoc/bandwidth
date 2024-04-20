#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import sys

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
parser.add_argument('filename', type=str, help='CSV file', action='store', nargs='?', default='/dev/stdin')
parser.add_argument('--L1', type=float, default=0., help='L1 cache size')
parser.add_argument('--L2', type=float, default=0., help='L2 cache size')
parser.add_argument('--L3', type=float, default=0., help='L3 cache size')
parser.add_argument('--L4', type=float, default=0., help='L4 cache size')
group = parser.add_mutually_exclusive_group()
group.add_argument('-l', '--linear', dest='linear', default=False, help='linear scale for Y axis', action='store_true')
group.add_argument('-L', '--log', dest='linear', help='log scale for Y axis', action='store_false')
group = parser.add_mutually_exclusive_group()
group.add_argument('--legend', dest='legend', default=True, help='Print the legend', action='store_true')
group.add_argument('--no-legend', dest='legend', help='Do not print the legend', action='store_false')
parser.add_argument('-m', '--machine', type=str, default='', help='Machine name')
parser.add_argument('-t', '--title', type=str, default='', help='Plot title')
parser.add_argument('-T', '--type', type=str, default='f32', help='plot only type')
parser.add_argument('-c', '--columns', type=str, default='', help='columns to plot')
parser.add_argument('--xscale', type=float, default=1., help='Scaling of the values in X')
parser.add_argument('--yscale', type=float, default=1., help='Scaling of the values in Y')
parser.add_argument('--xlabel', type=str, default='', help='')
parser.add_argument('--ylabel', type=str, default='', help='')
parser.add_argument('-o', '--out', type=str, default='', help='output filename')
parser.add_argument('-s', '--speedup', type=str, default='', help='show speedup for this column')

args = parser.parse_args()

import matplotlib
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

#matplotlib.rcParams['axes.autolimit_mode'] = 'round_numbers'

matplotlib.rcParams['patch.force_edgecolor'] = True
matplotlib.rcParams['patch.facecolor'] = 'b' 

matplotlib.rcParams['legend.fancybox'] = False
#matplotlib.rcParams['legend.loc'] = 'upper right'
matplotlib.rcParams['legend.numpoints'] = 1 
matplotlib.rcParams['legend.fontsize'] = 'large'
matplotlib.rcParams['legend.framealpha'] = None
matplotlib.rcParams['legend.scatterpoints'] = 3 
matplotlib.rcParams['legend.edgecolor'] = 'inherit'
#matplotlib.rcParams['axes.autolimit_mode'] = 'round_numbers'
#matplotlib.rcParams['axes.xmargin'] = 0 
#matplotlib.rcParams['axes.ymargin'] = 0 
#matplotlib.rcParams['xtick.direction'] = 'inout'
#matplotlib.rcParams['ytick.direction'] = 'inout'
matplotlib.rcParams['xtick.direction'] = 'in'
matplotlib.rcParams['ytick.direction'] = 'in'
matplotlib.rcParams['xtick.top'] = True
matplotlib.rcParams['ytick.right'] = True
matplotlib.rcParams['axes.grid'] = True
matplotlib.rcParams['grid.linewidth'] = 0.5 
matplotlib.rcParams['grid.color'] = '#000000'
matplotlib.rcParams['grid.alpha'] = 2./15. # 0.133

matplotlib.rcParams['font.family'] = 'Latin Modern Roman'
matplotlib.rcParams['mathtext.fontset'] = 'cm'
matplotlib.rcParams['axes.formatter.useoffset'] = False

raw_caches = [0., args.L1*args.xscale, args.L2*args.xscale, args.L3*args.xscale, args.L4*args.xscale]
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
title = args.title or (args.machine and "{} bandwidth".format(machine_name)) or ""

series = dict()

class Rset(object):
  def __init__(self, l):
    self.__set = set(l)
  def __contains__(self, v):
    return v not in self.__set
  def __repr__(self):
    return 'Rset({!r})'.format(self.__set)
  def __str__(self):
    return repr(self)

csv = CSV(args.filename)
currenttype = args.type or None
ignore = {'type'}
if args.columns:
  ignore = Rset(args.columns.split(","))
Xname = ''
X = []
with csv:
  for n in csv.header:
    series[n] = []
  Xname = next((h for h in csv.header if h not in {'type'}))
  for row in csv:
    if currenttype is None:
      currenttype = row["type"]
    elif currenttype != row["type"]:
      continue
    x = row[Xname] * args.xscale
    xmin = min(xmin, x)
    xmax = max(xmax, x)
    X.append(x)
    for n in csv.header:
      if n != Xname and n not in ignore and n in row:
        val = row[n] * args.yscale
        if val != 0:
          ymin = min(ymin, val)
          ymax = max(ymax, val)
        series[n].append(val)

ram_bw = None
if args.speedup:
  s = series[args.speedup][-10:]
  if len(s) > 0:
    ram_bw = sum(s) / len(s)
  

if args.out:
  fig = plt.figure(figsize=[6.,4.])
else:
  fig = plt.figure()
ax = plt.subplot(111)

if title:
  plt.title(title)
if args.xlabel:
  plt.xlabel(args.xlabel)
if args.ylabel:
  plt.ylabel(args.ylabel)


if ram_bw:
  ax.plot([xmin, xmax], [ram_bw, ram_bw], linewidth=1., linestyle=':', color='black')
for n in (h for h in csv.header if h not in ignore and h != Xname):
  ax.plot(X, series[n], label=n)

ax.set_xlim(xmin, xmax)
plt.xscale("log")

_, top = ax.get_ylim()

if ylog:
  ax.set_ylim(ymin/1.2, ymax*1.4)
  top = ymax*1.4
  plt.yscale("log")
else:
  ax.set_ylim(bottom=0.)


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

def y_formatter(v, *args):
  if "{:g}".format(v)[0] in '689':
    return ''
  return bytes_fmt(v, *args) + "/s"
formatter = ticker.FuncFormatter

ax.xaxis.set_major_formatter(ticker.FuncFormatter(bytes_fmt))
ax.xaxis.set_minor_formatter(ticker.NullFormatter())
ax.yaxis.set_minor_formatter(ticker.FuncFormatter(y_formatter))
ax.yaxis.set_major_formatter(ticker.FuncFormatter(y_formatter))

if ram_bw:
  import bisect

if len(caches) > 1:
  lastx = xmin
  if not ylog:
    ymin /= 2

  a = 0.1

  colors = ['black', 'green', 'blue', 'yellow', 'orange', 'red']
  for i in range(1, len(caches)):
    x = caches[i]
    xmid = (lastx*x)**0.5
    ax.axvspan(lastx, x, alpha=a, color=colors[i], linewidth=0.)
    ax.annotate("L{}".format(i), xy=(xmid, top), xycoords='data', xytext=(0., -5.), textcoords='offset points', horizontalalignment='center', verticalalignment='top')
    if ram_bw:
      i = bisect.bisect_left(X, xmid)
      ys = series[args.speedup][i-5:i+5]
      if len(ys) > 0:
        y = sum(ys) / len(ys)
        if ylog:
          ymid = (ram_bw * y)**0.5
        else:
          ymid = (ram_bw + y)*0.5
        ax.annotate("", (xmid, ram_bw), (xmid, y), arrowprops=dict(arrowstyle='<->', linewidth=0.5))
        plt.annotate(u"×{:0.2g}".format(y / ram_bw), xy=(xmid, ymid), xycoords='data', xytext=(3., 0.), textcoords='offset points', horizontalalignment='left', verticalalignment='center')

      
    lastx = x
  ax.axvspan(lastx, xmax, alpha=a, color=colors[-1], linewidth=0.)
  ax.annotate("RAM", xy=((lastx*xmax)**0.5, top), xycoords='data', xytext=(0., -5.), textcoords='offset points', horizontalalignment='center', verticalalignment='top')


ax.grid(b=True, which='major', color="black", alpha=4./30., linewidth=0.5)
ax.grid(b=True, which='minor', color="black", alpha=1.5/30., linewidth=0.5)

extra_artists = []
if args.legend:
  extra_artists = [plt.legend()]
if args.out:
  plt.savefig(args.out, bbox_extra_artists=extra_artists, bbox_inches='tight', transparent=True)
else:
  plt.show()
