print(paste("Performing the RechargeCalculator within", getwd()))
  
### Load input files ###
if(!"Basins" %in% ls()){
  Basins = read.table("basin.txt", header=T)
}
if(!"TimeSteps" %in% ls()){
  TimeSteps <- read.table('DateTime_yyyymmdd_hhmmss.txt', header = T)
}
if(!"Precipitation_mm" %in% ls()){
  Precipitation_mm = read.table("Precipitation_mm.txt", header=T)
}
if(!"Evaporation_mm" %in% ls()){
  Evaporation_mm = read.table("Evaporation_mm.txt", header=T)
}
if(!"Evaporation_mm" %in% ls()){
  Surface_runoff_cms = read.table("TotalRunoff_cms.txt", header=T)
}
if(!"Soil_storage_mm" %in% ls()){
  Soil_storage_mm = read.table("SoilStore.txt", header=T)
}
if(!"Depth_to_Water_mm" %in% ls()){
  Depth_to_Water_mm = read.table("WaterTableDepth.txt", header=T)
}


# functionalize the timesteploop
timesteploop <- function(TimeStep, totalRecharge_depot){
  for(TimeStep in TimeSteps$TimeStep){
    # CatchID + 1 will be used to adjust for TimeStep as the new column 1
    
    ### Look up P, E, and SR values for this daily timestep and convert to inches
    P <- Precipitation_mm[TimeStep, CatchID + 1]*0.0393701	#Precipitation (in)
    E <- Evaporation_mm[TimeStep, CatchID + 1]*0.0393701	#Evaporation (in)
    SR <- Surface_runoff_cms[TimeStep, CatchID + 1]*61024*60*60*24/Basins[CatchID, "BasinArea_in2"] #Surface runoff/Basin area (in)
    
    if(TimeStep==1){
      CDW <- as.numeric(0) ### Calculate change in depth to water (in)
      CSS <- as.numeric(0) ### Calculate change in soil storage term (in)
    } else {
      # take the difference between the current water depth and the previous water depth at this Drainage
      CDW <- (Depth_to_Water_mm[TimeStep, CatchID + 1] - Depth_to_Water_mm[TimeStep-1, CatchID + 1])*0.0393701
      CSS <- (Soil_storage_mm[TimeStep, CatchID + 1] - Soil_storage_mm[TimeStep-1, CatchID + 1])*0.0393701
    }
    
    ### Calculate daily recharge
    dailyRecharge = P-E-SR+CSS
    
    ### Calculate running total of daily recharge
    totalRecharge = totalRecharge + dailyRecharge
    
    # store the data
    totalRecharge_depot <- rbindlist(list(totalRecharge_depot, data.table(CatchID, TimeStep, dailyRecharge, totalRecharge)), use.names = T, fill = T)
  }
  return(totalRecharge_depot)
}

# daily iteration deposition
totalRecharge_depot <- data.table()
ptm <- Sys.time()

# for each subbasin
for (CatchID in Basins$CatchID) {
  
  ### Calculate average annual recharge (inches) for the basin ###
  totalRecharge=0

  # for each timestep
  totalRecharge_depot <- timesteploop(TimeStep, totalRecharge_depot)
  print(CatchID)
  ptm - Sys.time()
}

totalRecharge_depot <- read.table(paste0(Test1,'/totalRecharge_depot.txt'), sep = '\t', header = T)

# additional calculations
totalRecharge_depot <- totalRecharge_depot %>%
  # merge(., DateTime_water_year.txt, by = 'TimeStep') %>%
  
  # MonthlyRecharge total
  group_by(month, water_year, CatchID) %>%
  mutate(monthlyRecharge = sum(dailyRecharge)) %>%
  
  # WaterYear average AnnualRecharge
  group_by(water_year, CatchID) %>%
  mutate(averageAnnualRecharge = mean(monthlyRecharge)) %>%
  data.table() %>%
  
  group_by(CatchID) %>%
  mutate(averageAnnualRechargeByCatchID = mean(averageAnnualRecharge)) %>%
  data.table()



# print
write.table(totalRecharge_depot, 'totalRecharge_depot.txt', sep = '\t', row.names = F, col.names = T, append = F)
print(paste("Finished printing the RechargeCalculator within", getwd()))
