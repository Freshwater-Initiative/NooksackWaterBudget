import numpy as np
import os

MOD_indx = {2:0,   3:1,   4:2,   5:3,   6:4,   7:5,   8:6,   9:7,  10:8,  11:9,
           12:10, 13:11, 14:12, 15:13, 16:14, 17:15, 18:16, 19:17, 20:18, 21:19,
           22:20, 23:21, 24:22, 25:23, 26:24, 27:25, 28:26, 29:27, 30:28, 31:29,
           32:30, 33:31, 34:32, 35:33, 36:34, 38:35}
Top_to_Mod = {103:2,  115:3,  123:4,  124:5,  117:6,   92:7, 125:8,  122:9,   86:10,  91:11,
              107:12, 118:13, 128:14,  84:15, 121:16,  95:17, 81:18, 104:19,  93:20, 130:21,
                3:22,  83:23, 106:24, 108:25, 126:26, 129:27, 89:28,  85:29, 120:30,  94:31,
               80:32,  82:33, 105:34, 116:35, 90:36,   88:38}

MOD_ID = [ 2,  3,  4,  5,  6,  7,  8,  9, 10, 11,
          12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
          22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
          32, 33, 34, 35, 36, 38]
MOD_ID = np.asarray(MOD_ID)

Top_ID = [103, 115, 123, 124, 117,  92, 125, 122,  86,  91,
          107, 118, 128,  84, 121,  95,  81, 104,  93, 130,
            3,  83, 106, 108, 126, 129,  89,  85, 120,  94,
           80,  82, 105, 116,  90,  88]
Top_ID = np.asarray(Top_ID)

names = { 2:"Blaine",                     3:"Breckenridge",          4:"California",            5:"Cherry Point",
          6:"Dale",                       7:"Deer",                  8:"Fazon",                 9:"Fingalson",
         10:"Fishtrap",                   11:"Fourmile",            12:"Haynie",               13:"Johnson",
         14:"Jordan",                     15:"Kamm",                16:"Lake Terrell",         17:"Little Campbell",
         18:"Lower Anderson",             19:"Lower Dakota",        20:"Lummi Peninsula West", 21:"Lummi River Delta",
         22:"Nooksack Deming to Everson", 23:"North Fork Anderson", 24:"North Fork Dakota",    25:"Saar",
         26:"Sandy Point",                27:"Schell",              28:"Schneider",            29:"Scott",
         30:"Semiahmoo",                  31:"Silver",              32:"Smith",                33:"South Fork Anderson",
         34:"South Fork Dakota",          35:"Swift",               36:"Ten Mile",             38:"Wiser Lake/Cougar Creek"}

# ----------------------------------
topinpFile = open('topinpBertrand.dat','r')
lines = topinpFile.readlines()
topinpFile.close()
nstepsBertrand = int(lines[3].split()[0])

topinpFile = open('modelspcBertrand.dat','r')
lines = topinpFile.readlines()
topinpFile.close()
nSubBertrand = int(lines[2].split()[1])
# ----------------------------------
topinpFile = open('topinpWRIA1.dat','r')
lines = topinpFile.readlines()
topinpFile.close()
nstepsLENS = int(lines[3].split()[0])   # WRIA1 and LENS are are the same

topinpFile = open('modelspcWRIA1.dat','r')
lines = topinpFile.readlines()
topinpFile.close()
nSubWRIA1 = int(lines[2].split()[1])
nSubLENS  = 36
# ----------------------------------
print('Reading MODFLOW output.')
modflowFile = open('Drainage_DTW_censored.dat','r')
DTW_SS_M   = np.zeros(nSubLENS+nSubBertrand, dtype='d')
DTW_IRR_M  = np.zeros(nSubLENS+nSubBertrand, dtype='d')
DTW_NIRR_M = np.zeros(nSubLENS+nSubBertrand, dtype='d')
Flags      = np.full(nSubWRIA1, -1, dtype='i')
line = modflowFile.readline()   # discard the header
for jsub in range(nSubLENS):
    line = modflowFile.readline()
    splitLine = line.split()
    modID = int(splitLine[0])
    if modID == 37: # skip ahead
        line = modflowFile.readline()
        splitLine = line.split()
        modID = int(splitLine[0])
    DTW_SS_M[jsub]   = float(splitLine[-3])*0.3048
    DTW_IRR_M[jsub]  = float(splitLine[-2])*0.3048
    DTW_NIRR_M[jsub] = float(splitLine[-1])*0.3048
    Flags[Top_ID[jsub]-1]      = Top_ID[jsub]
    #print('jsub {0:3d} topID {1:3d} modID  {2:3d} {3:27s} DTW_SS{4:8.4f}  DTW_IRR{5:8.4f}  DTW_NONIRR{6:8.4f}'.format(
                        #jsub, topID, modID, names[modID], DTW_SS_M[jsub], DTW_IRR_M[jsub], DTW_NIRR_M[jsub]))
print('Reading WRIA1 depths to water, {0:d} drainages'.format(nSubWRIA1))
zbar = np.zeros((nstepsLENS+1, nSubWRIA1), dtype='d')
zFile = open('zbar.dat','r')
lines = zFile.readlines()
for line in lines:
    splitLine = line.split()
    nt   = int(splitLine[0])
    jsub = int(splitLine[1])-1
    zbar[nt,jsub] = float(splitLine[2]) # The rest are the same.

# A relevant TopNet comment about the six regions in each subbasin:
#   The one line below is a substantive change DGT 10/13/12.  It amounts to an assumption of
#   lumped depth to water table, rather than separate depth to water table for each drainage and
#   irrigation component.  It was made to fix the issue of groundwater levels declining indefinitely
#   due to there being no recharge from artificially drained areas

# --------------------- starting with LENS ---------------------------------
print('Writing LENS depth-to-water files, {0:d} drainages'.format(nSubWRIA1))
if not os.access('LENS', os.F_OK):
    os.mkdir('LENS')
os.chdir('LENS')
zbar_ssFile   = open('zbar_ss.dat','w')
zbar_irrFile  = open('zbar_irr.dat','w')
zbar_nirrFile = open('zbar_nirr.dat','w')
istep = 365 # a placeholder
for jsub in range(nSubWRIA1):
    if Flags[jsub] > -1:
        mod_ID = Top_to_Mod[Flags[jsub]]
        mod_indx = MOD_indx[mod_ID]
        #print('{0:4d} {1:26s} {2:5.1f} {3:5.1f} {4:5.1f}'.format(mod_indx, names[mod_ID],
            #DTW_SS_M[mod_indx]/0.3048, DTW_IRR_M[mod_indx]/0.3048, DTW_NIRR_M[mod_indx]/0.3048))
        zbar_ssFile.write('{0:4d} {1:17.9f}\n'.format(jsub, DTW_SS_M[mod_indx]))
        zbar_irrFile.write('{0:4d} {1:17.9f}\n'.format(jsub, DTW_IRR_M[mod_indx]))
        zbar_nirrFile.write('{0:4d} {1:17.9f}\n'.format(jsub, DTW_NIRR_M[mod_indx]))
    else:
        zbar_ssFile.write('{0:4d} {1:17.9f}\n'.format(jsub, zbar[istep,jsub]))
        zbar_irrFile.write('{0:4d} {1:17.9f}\n'.format(jsub, zbar[istep,jsub]))
        zbar_nirrFile.write('{0:4d} {1:17.9f}\n'.format(jsub, zbar[istep,jsub]))
zbar_ssFile.close()
zbar_irrFile.close()
zbar_nirrFile.close()

# ----------------- now Bertrand ---------------------------------------------
print('Reading Bertrand depth-to-water files, {0:d} drainages'.format(nSubBertrand))
os.chdir('..')
if not os.access('Bertrand', os.F_OK):
    os.mkdir('Bertrand')
os.chdir('Bertrand')
for jsub in range(nSubLENS,nSubLENS+nSubBertrand):
    line = modflowFile.readline()
    splitLine = line.split()
    modID = int(splitLine[0])
    topID = int(splitLine[0][2:])
    DTW_SS_M[jsub]     = float(splitLine[-3])*0.3048
    DTW_IRR_M[jsub]    = float(splitLine[-2])*0.3048
    DTW_NIRR_M[jsub] = float(splitLine[-1])*0.3048
    #print('{0:3d} {1:4d} BertrandSub-{2:d}  DTW_SS{3:8.4f}  DTW_IRR{4:8.4f}  DTW_NONIRR{5:8.4f}'.format(topID,
                                        #modID, topID, DTW_SS_M[jsub], DTW_IRR_M[jsub], DTW_NIRR_M[jsub]))
print('Writing Bertrand depth-to-water files.')
zbar_ssFile   = open('zbar_ss.dat','w')
zbar_irrFile  = open('zbar_irr.dat','w')
zbar_nirrFile = open('zbar_nirr.dat','w')
for j in range(nSubLENS,nSubLENS+nSubBertrand):
    jsub = j - nSubLENS
    print('{0:4d} {1:5.1f} {2:5.1f} {3:5.1f}'.format(jsub,
            DTW_SS_M[j]/0.3048, DTW_IRR_M[j]/0.3048, DTW_NIRR_M[j]/0.3048))
    zbar_ssFile.write('{0:4d} {1:17.9f}\n'.format(jsub, DTW_SS_M[j]))
    zbar_irrFile.write('{0:4d} {1:17.9f}\n'.format(jsub, DTW_IRR_M[j]))
    zbar_nirrFile.write('{0:4d} {1:17.9f}\n'.format(jsub, DTW_NIRR_M[j]))
zbar_ssFile.close()
zbar_irrFile.close()
zbar_nirrFile.close()
