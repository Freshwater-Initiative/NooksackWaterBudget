# Topnet-WM
Surface water model based on Topmodel assumptions, a networked kinematic wave routing, and Water Management component for estimating irrigation, municipal, domestic, commercial, and dairy water use withdrawals from surface and groundwater sources. 
Nooksack River case study data and models to support water resource professionals and their salmon recovery partners working with the WRIA 1 Joint Board on water supply planning and instream flow negotiations.

## To install Topnet-WM 
Clone or Fork and Clone repository.  In the sourcecode folder, run make to compile in UNIX.

> make

Pre and post processing code is available in Python, with earlier R versions archived. 

### Notes
On April 21st a problem with coordinate conversion that affected processes such as evapotranspiration was fixed that resulted in adding a dependency for the Proj4 geospatial library (-lproj). The problem was due to a linear conversion using parameters from an input file, but the parameters for the input file available in the input data set were for the wrong UTM zone. The effect was that Whatom County positons were converted to positions East of the Cascades, and those positions were then used in subsequent calculations by Topnet.
The fix could have been to figure out the correct parameters for the input file, but simply writing code to make the conversion in TopNet itself was more accurate and reliable.

### Acknowledgements:
This repository was orginally developed in https://github.com/Freshwater-Initiative/NooksackWaterBudget 
by Bert Rubash and Christina Bandaragoda

Cite DMIP paper, 2012 LNWB, 2019 Groundwater report, HydrOShare resource for SWGW model 
