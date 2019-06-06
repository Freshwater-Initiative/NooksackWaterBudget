import numpy as np
import matplotlib.pyplot as plt
import datetime as dt
import matplotlib.dates as mdates
import os
from datetime import datetime

TRANSIENT = False

#Model = 'Bertrand'
#steps     = 1000
#start      = 0
#drainages = 46
#path = '../modelruns_BC_2016/'

Model = 'WRIA1_s'
steps      = 16894
start      = 15069     # plot only part of the series
drainages  = 172
path = '../modelruns_1946_2006/'

zbar_ref       = np.zeros((drainages, steps), dtype='d')
zbar_ss        = np.zeros((drainages, steps), dtype='d')
if TRANSIENT:
    zbar_transient = np.zeros((drainages, steps), dtype='d')
else:
    zbar_irr   = np.zeros((drainages, steps), dtype='d')
    zbar_nirr  = np.zeros((drainages, steps), dtype='d')
dates = []

print('Reading date_zbar.dat')
referenceFile   = open(path + 'results/date_zbar.dat','r')
lines = referenceFile.readlines()
referenceFile.close()
for line in lines:
    splitLine = line.split()
    step  = int(splitLine[0])-1
    nsub  = int(splitLine[1])-1
    zbar_ref[nsub,step] = float(splitLine[5])
    year  = int(splitLine[2])
    month = int(splitLine[3])
    day   = int(splitLine[4])
    if nsub == 1:
        dates.append(np.datetime64('{0:04d}-{1:02d}-{2:02d}'.format(year, month, day)).astype(datetime))

print('Reading date_zbar_ss.dat')
steadyStateFile = open(path + 'results/date_zbar_ss.dat','r')
lines = steadyStateFile.readlines()
steadyStateFile.close()
for line in lines:
    splitLine = line.split()
    step  = int(splitLine[0])-1
    nsub  = int(splitLine[1])-1
    zbar_ss[nsub,step] = float(splitLine[5])

if TRANSIENT:
    print('Reading date_zbar_transient.dat')
    transientFile = open(path + 'results/date_zbar_transient.dat','r')
    lines = transientFile.readlines()
    transientFile.close()
    for line in lines:
        splitLine = line.split()
        step  = int(splitLine[0])-1
        nsub  = int(splitLine[1])-1
        zbar_transient[nsub,step] = float(splitLine[5])
else:
    print('Reading date_zbar_irr.dat')
    irrFile = open(path + 'results/date_zbar_irr.dat','r')
    lines = irrFile.readlines()
    irrFile.close()
    for line in lines:
        splitLine = line.split()
        step  = int(splitLine[0])-1
        nsub  = int(splitLine[1])-1
        zbar_irr[nsub,step] = float(splitLine[5])

    print('Reading date_zbar_nirr.dat')
    nirrFile = open(path + 'results/date_zbar_nirr.dat','r')
    lines = nirrFile.readlines()
    nirrFile.close()
    for line in lines:
        splitLine = line.split()
        step  = int(splitLine[0])-1
        nsub  = int(splitLine[1])-1
        zbar_nirr[nsub,step] = float(splitLine[5])

if not os.access('images_'+Model, os.F_OK):
    os.mkdir('images_'+Model)

years = mdates.YearLocator()   # every year
months = mdates.MonthLocator()  # every month
years_fmt = mdates.DateFormatter('%Y')

for n in range(drainages):
    fig, ax = plt.subplots(figsize=(20,9))
    ax.plot(dates[start:], zbar_ref[n,start:],  label='Reference')
    ax.plot(dates[start:], zbar_ss[n,start:],   label='Steady State')
    if TRANSIENT:
        ax.plot(dates[start:], zbar_transient[n,start:],  label='transient')
    else:
        ax.plot(dates[start:], zbar_irr[n,start:],  label='Irrigated')
        ax.plot(dates[start:], zbar_nirr[n,start:], label='Non-Irrigated')
    ax.legend()

    # format the ticks
    ax.xaxis.set_major_locator(years)
    ax.xaxis.set_major_formatter(years_fmt)
    ax.xaxis.set_minor_locator(months)

    # round to nearest years.
    datemin = np.datetime64(dates[start], 'Y')
    datemax = np.datetime64(dates[-1], 'Y') + np.timedelta64(1, 'Y')
    ax.set_xlim(datemin, datemax)

    # format the coords message box
    ax.format_xdata = mdates.DateFormatter('%Y-%m-%d')
    ax.grid(True)
    fig.autofmt_xdate()

    #ax.set_xlabel('Timesteps (days)')
    ax.set_ylabel('Depth to Water Table (m)')
    ax.set_title(Model + ' Drainage {0:2d}'.format(n+1))

    print('Saving images_'+Model+'/D_to_W_sub_{0:03d}.png'.format(n+1))
    plt.savefig('images_'+Model+'/D_to_W_sub_{0:03d}.png'.format(n+1))
    #plt.show()
    plt.close()

# If GM ImageMagick is installed, in shell terminal run:
# gm convert -delay 2000 *.png  depth_to_water.gif

