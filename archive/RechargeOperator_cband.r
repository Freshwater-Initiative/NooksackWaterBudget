#08/14/2018
#christina bandaragoda

##Installations (if needed)
#install.packages("xlsx")
## installing/loading the package:
#if(!require(installr)) {
#  install.packages("installr"); 
#  require(installr)
#} #load / install+load installr
## using the package:
#updateR()


# change param options
options(java.parameters = "-Xmx1024m")
library(xlsx)
library(data.table)
library(dplyr)
library(hydroGOF)

# home directories
if(dir.exists('C:/Users/Jimmy/Google Drive/Watershed Dynamics Group/Projects/Bertrand Creek/2016 Project')){ # for jim
  Project2016 <- 'C:/Users/Jimmy/Google Drive/Watershed Dynamics Group/Projects/Bertrand Creek/2016 Project'
} else if (dir.exists('C:/Users/cband/Nooksack/Bertrand Creek/2016 Project')) { # for Christina
  Project2016 <- 'C:/Users/cband/Nooksack/Bertrand Creek/2016 Project'
}
# home directories
if(dir.exists('C:/Users/Jimmy/Google Drive/Watershed Dynamics Group/Projects/Bertrand Creek/2016 Project')){ # for jim
  Project2018 <- 'C:/Users/Jimmy/Google Drive/Watershed Dynamics Group/Projects/Bertrand Creek/2016 Project'
} else if (dir.exists('C:/Users/cband/Nooksack/Bertrand Creek/2018 Project')) { # for Christina
  Project2018 <- 'C:/Users/cband/Nooksack/Bertrand Creek/2018 Project'
}
# subdirectories

WMoff2018_Models <- paste(Project2018, 'modelruns_1952WRIA1_081418/modelruns_1952WRIA1_110912_WMoff', sep='/')
WMon2012_Models <- paste(Project2018, 'modelruns_1952WRIA1_081418/modelruns_1952WRIA1_110912_WMon', sep='/')
#WMon2018_Models <- paste(Project2018, 'modelruns_1952WRIA1_081418', sep='/')
# set working directory

# for (ModelSet_folder in c(Test1, Test2, Test3, Test4, Test5)){
#for (ModelSet_folder in c(Test2, Test3, Test4, Test5)){
#ModelSet_folder=WMoff2018_Models
ModelSet_folder=WMon2012_Models
for (ModelSet_folder in c(WMoff2018_Models,WMon2012_Models)){

  print(paste('start' ModelSet_folder))

  setwd(ModelSet_folder)
  getwd()
  # read in topsbd_v8
  topsbd <- read.table('topsbd_v8.txt', skip = 1, header = T) %>% data.table()
  
  # if statement for colnames that have been removed
  if (!'TimeStep' %in% colnames(topsbd)){
    topsbd <- read.table('topsbd_v8.txt', skip = 0, header = F) %>%
      data.table() %>%
      setnames(., colnames(.), c('Basin', 'TimeStep', 'IrrDrainCat', 'Afrac', 'SWInput_mm', 'Qlat_mm', 'Qtot_mm', 'Qb_mm', 'Recharge_mm', 'SatEx_mm', 'InfEx_mm', 'SurfRo_mm', 'SatAfrac', 'InfAfrac', 'IntStore_mm', 'WTDepth_mm', 'SoilStore_mm', 'Pet_mm', 'Aet_mm', 'Irrig_mm', 'GWTake_mm', 'IrrDem_mm', 'Prec_mm', 'SWE_mm', 'Sublim_mm', 'Tave_C', 'Tdew_C', 'Trange_C', 'ErrClosure_mm'))
  }
  
  # generate TimeStep by subbasin matrices
  Zbar <- dcast(topsbd %>% select(TimeStep, Basin, WTDepth_mm), TimeStep~Basin, value.var = 'WTDepth_mm', fill = 0)
  PET <- dcast(topsbd %>% select(TimeStep, Basin, Pet_mm), TimeStep~Basin, value.var = 'Pet_mm', fill = 0)
  AET <- dcast(topsbd %>% select(TimeStep, Basin, Aet_mm), TimeStep~Basin, value.var = 'Aet_mm', fill = 0)
  Rain <- dcast(topsbd %>% select(TimeStep, Basin, Prec_mm), TimeStep~Basin, value.var = 'Prec_mm', fill = 0)
  SatEx <- dcast(topsbd %>% select(TimeStep, Basin, SatEx_mm), TimeStep~Basin, value.var = 'SatEx_mm', fill = 0)
  InfEx <- dcast(topsbd %>% select(TimeStep, Basin, InfEx_mm), TimeStep~Basin, value.var = 'InfEx_mm', fill = 0)
  SurfRO <- dcast(topsbd %>% select(TimeStep, Basin, SurfRo_mm), TimeStep~Basin, value.var = 'SurfRo_mm', fill = 0)
  SoilStore <- dcast(topsbd %>% select(TimeStep, Basin, SoilStore_mm), TimeStep~Basin, value.var = 'SoilStore_mm', fill = 0)
  
  # load Recharge inputs
  Basins <- read.table("basin.txt", header=T)
  Precipitation_mm <- Rain
  Evaporation_mm = read.table("Evaporation_mm.txt", header=T)
  Depth_to_Water_mm <- Zbar
  # Make if statement here
  #Surface_runoff_cms = read.table("TotalRunoff_cms.txt", header=T)
  Surface_runoff_cms = read.table("TotalRunoff_noWithdrawal_cms.txt", header=T)

  # print these outputs to xlsx using these names
  for (mat in c('InfEx', 'PET', 'AET', 'SatEx', 'Rain', 'Zbar', 'SoilStore', 'Surface_runoff_cms')){
  
    print(paste('starting to print', mat))
    write.table(mat %>% get() %>% data.frame(), file = paste0(mat, '.txt'), col.names = T, row.names = F, append = F)
    print(paste(mat, 'printed to', paste0(mat, '.txt')))
  }
  rm(mat)
  
  # source the RechargeCalculator_kbcb.R script
  # output is the object totalRecharge
  source(paste(WMon2018_Models, "RechargeCalculator_kbcb_jp.R", sep = '/'))
}
