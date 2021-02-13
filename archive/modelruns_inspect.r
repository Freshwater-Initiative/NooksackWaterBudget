library(data.table)
library(dplyr)
library(hydroGOF)

# change param options
options(java.parameters = "-Xmx1024m")
library(xlsx)

# home directories
if(dir.exists('C:/Users/Jimmy/Google Drive/Watershed Dynamics Group/Projects/Bertrand Creek/2016 Project')){ # for jim
  Project2016 <- 'C:/Users/Jimmy/Google Drive/Watershed Dynamics Group/Projects/Bertrand Creek/2016 Project'
} else if (dir.exists('C:/Users/cband/Google Drive/UW Projects/Watershed Dynamics Group/Projects/Bertrand Creek/2016 Project')) { # for Christina
  Project2016 <- 'C:/Users/cband/Google Drive/UW Projects/Watershed Dynamics Group/Projects/Bertrand Creek/2016 Project'
}

# subdirectories
WMoff2018_Models <- paste(Project2018, 'modelruns_1952WRIA1_081418/modelruns_1952WRIA1_110912_WMoff', sep='/')
WMon2012_Models <- paste(Project2018, 'modelruns_1952WRIA1_081418/modelruns_1952WRIA1_110912_WMon', sep='/')

# set working directory

# for (ModelSet_folder in c(Test1, Test2, Test3, Test4, Test5)){
#for (ModelSet_folder in c(Test2, Test3, Test4, Test5)){
for (ModelSet_folder in c(WMoff2018_Models)){

  print(paste('start' ModelSet_folder))
  setwd(ModelSet_folder)
  
  # set start date and end date of rain year
  Rain_start <- as.numeric(19000101) #as.numeric(20061001)
  Rain_end <- as.numeric(20150930)
  calib_start <- as.numeric(20030613)
  calib_end <- as.numeric(20051231)
  valid_start <- as.numeric(20060101)
  valid_end <- as.numeric(20160930)
  
  # set target files
  readus <- c('FlowAtStreamNodes_cms.txt',
              'UserDemand_cms.txt',
              'UserWithdrawal_cms.txt',
              'TotalRunoff_noWithdrawal_cms.txt',
			  #'TotalRunoff_cms.txt',
              'UserDeficit_cms.txt',
              
              'Precipitation_mm.txt',
              'Evaporation_mm.txt',
              
              'DateTime_yyyymmdd_hhmmss.txt',
              'Artificial_Drainage.txt',
              'basinpars.txt',
              'StreamFlowLinks.txt',
              'DrainageID.txt',
              'DrainageInfo.txt',
              
              'user.txt')
  
  readus_skip1 <- c('user.txt', 'MonthlyDemandFraction.txt')
  
  # readus for 2009calib_ArtDrain_WMon
  for (i in readus){
    if(i %in% readus_skip1){
      assign(i, i %>% read.table(skip = 1, header=T) %>% data.table())
      print(paste('reading', i))
    } else{
      assign(i, i %>% read.table(header=T) %>% data.table())
      print(paste('reading', i))
    }
  }
  
  
  # function to calculate the water year
  wtr_yr <- function(dates, start_month=10) {
    # Convert dates into POSIXlt
    dates.posix = as.POSIXlt(as.character(dates), format = '%Y%m%d')
    
    # Calculate offset based on the month of the year
    offset = ifelse(dates.posix$mon >= start_month, 1, 0)
    
    # adjust the current year to the appropriate Water year
    adj.year = dates.posix$year + 1900 + offset # year + adjustment for date origin + offset
    return(adj.year)
  }
  
  # Abstract the Year, Month, and range for the Water Year period
  DateTime_water_year.txt <- DateTime_yyyymmdd_hhmmss.txt %>%
    mutate(year = substring(yyyymmdd, 1,4) %>% as.numeric(),
           month = substring(yyyymmdd, 5, 6) %>% as.numeric(),
           day = substring(yyyymmdd, 7, 8) %>% as.numeric(),
           water_year = wtr_yr(yyyymmdd, start_month = 9)) %>%
    filter(yyyymmdd >= Rain_start,
           yyyymmdd <= Rain_end) %>% 
    arrange(TimeStep) %>%
    data.table()
  
  # calib
  DateTime_calib.txt <- DateTime_yyyymmdd_hhmmss.txt %>%
    mutate(year = substring(yyyymmdd, 1,4) %>% as.numeric(),
           month = substring(yyyymmdd, 5, 6) %>% as.numeric(),
           day = substring(yyyymmdd, 7, 8) %>% as.numeric(),
           water_year = wtr_yr(yyyymmdd, start_month = 9)) %>%
    filter(yyyymmdd >= calib_start,
           yyyymmdd <= calib_end) %>% 
    arrange(TimeStep) %>%
    data.table()
  
  # validation
  DateTime_valid.txt <- DateTime_yyyymmdd_hhmmss.txt %>%
    mutate(year = substring(yyyymmdd, 1,4) %>% as.numeric(),
           month = substring(yyyymmdd, 5, 6) %>% as.numeric(),
           day = substring(yyyymmdd, 7, 8) %>% as.numeric(),
           water_year = wtr_yr(yyyymmdd, start_month = 9)) %>%
    filter(yyyymmdd >= valid_start,
           yyyymmdd <= valid_end) %>% 
    arrange(TimeStep) %>%
    data.table()
  
  # convert mm to inches
  Precipitation_in.txt <- Precipitation_mm.txt %>%
    mutate_each(funs(./25.4), -TimeStep) %>%
    merge(., DateTime_water_year.txt, by = 'TimeStep') %>% 
    arrange(TimeStep) %>%
    select(TimeStep, yyyymmdd, year, month, day, water_year, contains('Drainage')) %>%
    data.table()
  
  Evaporation_in.txt <- Evaporation_mm.txt %>%
    mutate_each(funs(./25.4), -TimeStep) %>%
    merge(., DateTime_water_year.txt, by = 'TimeStep') %>% 
    arrange(TimeStep) %>%
    select(TimeStep, yyyymmdd, year, month, day, water_year, contains('Drainage')) %>%
    data.table()
  
  
  # convert cms to cfs: 1cms = 35.3147 cfs
  FlowAtStreamNodes_cfs.txt <- FlowAtStreamNodes_cms.txt %>%
    mutate_each(funs(.*35.3147), -TimeStep) %>%
    merge(., DateTime_water_year.txt, by = 'TimeStep') %>% 
    mutate(FlowAtOutlet = Node1) %>% # 
    arrange(TimeStep) %>% 
    select(TimeStep, yyyymmdd, year, month, day, water_year, contains('Node')) %>%
    data.table()
  
  Artificial_Drainage_cfs.txt <- Artificial_Drainage.txt %>%
    mutate_each(funs(.*35.3147), -TimeStep) %>%
    merge(., DateTime_water_year.txt, by = 'TimeStep') %>% 
    arrange(TimeStep) %>% 
    select(TimeStep, yyyymmdd, year, month, day, water_year, contains('Drainage')) %>%
    data.table()
  
  
  # convert cms to gal/day: 1cms * 264.172 gal/m^3 * 86400 s/day = 23688461 gal/day
  UserDemand_gpd.txt <- UserDemand_cms.txt %>%
    mutate_each(funs(.*264.172*86400), -TimeStep) %>%
    merge(., DateTime_water_year.txt, by = 'TimeStep') %>% 
    arrange(TimeStep) %>% 
    select(TimeStep, yyyymmdd, year, month, day, water_year, contains('User')) %>%
    data.table()
  
  UserWithdrawal_gpd.txt <- UserWithdrawal_cms.txt %>%
    mutate_each(funs(.*264.172*86400), -TimeStep) %>%
    merge(., DateTime_water_year.txt, by = 'TimeStep') %>% 
    arrange(TimeStep) %>% 
    select(TimeStep, yyyymmdd, year, month, day, water_year, contains('User')) %>%
    data.table()
  
  
  # monthly average for the 9 water years
  monthlySummary_Precipitation_in.txt <- Precipitation_in.txt %>%
    # group by TimeStep and Month
    select(-c(TimeStep, yyyymmdd, year, day)) %>%
    melt(., id.vars = c('month', 'water_year'), variable.name = 'Drainages', value.name = 'Precipitation') %>%
    
    # calculate total by month, year, and drainage
    group_by(month, water_year, Drainages) %>%
    summarise(sumByMonthyByWateryearByDrainage = sum(Precipitation)) %>%
    
    # calculate mean by month, year, and drainage
    group_by(month, Drainages) %>%
    summarise(meanByMonthByDrainage = mean(sumByMonthyByWateryearByDrainage)) %>%
    
    # calculate mean and sd by month
    group_by(month) %>%
    mutate(meanByMonth = mean(meanByMonthByDrainage),
           sdByMonth = sd(meanByMonthByDrainage)) %>%
    
    # reformat the table to wide for each DrainageID
    dcast(., month + meanByMonth + sdByMonth ~ Drainages, value.var = 'meanByMonthByDrainage') %>%
    data.table()
  
  
  # annual average for the 9 water years
  yearlySummary_Precipitation_in.txt <- Precipitation_in.txt %>%
    # group by TimeStep and Month
    select(-c(TimeStep, yyyymmdd, year, day)) %>%
    melt(., id.vars = c('month', 'water_year'), variable.name = 'Drainages', value.name = 'Precipitation') %>%
    
    # calculate total by month, wateryear, and drainage
    group_by(month, water_year, Drainages) %>%
    summarise(sumByMonthByWateryearByDrainage = sum(Precipitation)) %>%
    
    # calculate mean by wateryear, and drainage
    group_by(water_year, Drainages) %>%
    summarise(meanByWateryearByDrainage = sum(sumByMonthByWateryearByDrainage)) %>%
    
    # calculate mean and sd by month
    group_by(water_year) %>%
    mutate(meanByWateryear = mean(meanByWateryearByDrainage),
           sdByWateryear = sd(meanByWateryearByDrainage)) %>%
    
    # reformat the table to wide for each DrainageID
    dcast(., water_year + meanByWateryear + sdByWateryear ~ Drainages, value.var = 'meanByWateryearByDrainage') %>%
    data.table()
  
  
  # monthly average for the 9 water years
  monthlySummary_Evaporation_in.txt <- Evaporation_in.txt %>%
    # group by TimeStep and Month
    select(-c(TimeStep, yyyymmdd, year, day)) %>%
    melt(., id.vars = c('month', 'water_year'), variable.name = 'Drainages', value.name = 'Evaporation') %>%
    
    # calculate total by month, year, and drainage
    group_by(month, water_year, Drainages) %>%
    summarise(sumByMonthyByWateryearByDrainage = sum(Evaporation)) %>%
    
    # calculate mean by month, year, and drainage
    group_by(month, Drainages) %>%
    summarise(meanByMonthByDrainage = mean(sumByMonthyByWateryearByDrainage)) %>%
    
    # calculate mean and sd by month
    group_by(month) %>%
    mutate(meanByMonth = mean(meanByMonthByDrainage),
           sdByMonth = sd(meanByMonthByDrainage)) %>%
    
    # reformat the table to wide for each DrainageID
    dcast(., month + meanByMonth + sdByMonth ~ Drainages, value.var = 'meanByMonthByDrainage') %>%
    data.table()
  
  
  # annual average for the 9 water years
  yearlySummary_Evaporation_in.txt <- Evaporation_in.txt %>%
    # group by TimeStep and Month
    select(-c(TimeStep, yyyymmdd, year, day)) %>%
    melt(., id.vars = c('month', 'water_year'), variable.name = 'Drainages', value.name = 'Evaporation') %>%
    
    # calculate total by month, wateryear, and drainage
    group_by(month, water_year, Drainages) %>%
    summarise(sumByMonthByWateryearByDrainage = sum(Evaporation)) %>%
    
    # calculate mean by wateryear, and drainage
    group_by(water_year, Drainages) %>%
    summarise(meanByWateryearByDrainage = sum(sumByMonthByWateryearByDrainage)) %>%
    
    # calculate mean and sd by month
    group_by(water_year) %>%
    mutate(meanByWateryear = mean(meanByWateryearByDrainage),
           sdByWateryear = sd(meanByWateryearByDrainage)) %>%
    
    # reformat the table to wide for each DrainageID
    dcast(., water_year + meanByWateryear + sdByWateryear ~ Drainages, value.var = 'meanByWateryearByDrainage') %>%
    data.table()
  
  # the average and sd of the average Drainages
  monthlySummary_Artificial_Drainage_cfs.txt <- Artificial_Drainage_cfs.txt %>%
    # group by TimeStep and Month
    select(-c(TimeStep, yyyymmdd, year, day)) %>%
    melt(., id.vars = c('month','water_year'), variable.name = 'Drainages', value.name = 'Artificial_Drainages') %>%
    
    # calculate the sum of artificial drainage for each month and wateryear
    group_by(month, water_year) %>%
    mutate(sumByMonthByWateryear = sum(Artificial_Drainages)) %>%
    
    # calculate mean by month and drainage
    group_by(month, Drainages) %>%
    mutate(meanByMonthByDrainage = mean(Artificial_Drainages)) %>%
    
    # calculate mean and sd by month
    group_by(month) %>%
    mutate(meanByMonth = mean(meanByMonthByDrainage),
           sdByMonth = sd(meanByMonthByDrainage),
           meanOfSumByMonth = mean(sumByMonthByWateryear)) %>%
    
    # reformat the table to wide for each DrainageID
    select(month, meanOfSumByMonth, meanByMonth, sdByMonth, Drainages, meanByMonthByDrainage) %>%
    unique() %>% # meanByMonthByDrainage should be the most stringent - get rid of duplicate rows
    dcast(., month + meanOfSumByMonth + meanByMonth + sdByMonth ~ Drainages, value.var = 'meanByMonthByDrainage') %>%
    data.table()
  
  
  # the average and sd of the average Node
  monthlySummary_FlowAtStreamNodes_cfs.txt <- FlowAtStreamNodes_cfs.txt %>%
    # group by TimeStep and Month
    select(-c(TimeStep, yyyymmdd, year, day, water_year)) %>%
    melt(., id.vars = c('month'), variable.name = 'Node', value.name = 'FlowAtStreamNodes') %>%
    
    # calculate mean by month and node
    group_by(month, Node) %>%
    summarise(meanByMonthByNode = mean(FlowAtStreamNodes)) %>%
    
    # calculate mean and sd by month
    group_by(month) %>%
    mutate(meanByMonth = mean(meanByMonthByNode),
           sdByMonth = sd(meanByMonthByNode)) %>%
    
    # reformat the table to wide for each NodeID
    dcast(., month+meanByMonth+sdByMonth~Node, value.var = 'meanByMonthByNode') %>%
    select(month, meanByMonth, sdByMonth, Node1, Node4, Node28) %>%
    data.table()
  
  
  # Calculate the Ratios
  AD_FASN_Summary <- Artificial_Drainage_cfs.txt %>%
    # group by TimeStep and Month
    select(-c(yyyymmdd, year, month, day, water_year)) %>%
    melt(., id.vars = c('TimeStep'), variable.name = 'Drainages', value.name = 'Artificial_Drainages') %>%
    
    # calculate the sum of artificial drainage for each month and wateryear
    group_by(TimeStep) %>%
    summarise(sumArtDrainByTimeStep = sum(Artificial_Drainages)) %>%
    
    # merge with FlowAtStreamNodes
    merge(., FlowAtStreamNodes_cfs.txt %>% 
            select(TimeStep, water_year, month, Node1) %>% 
            group_by(water_year, month) %>% 
            mutate(monthlyMeanNode1 = mean(Node1)),
          all=F, fill=T) %>%
    mutate(ratio_ArtDrain_Node1 = sumArtDrainByTimeStep/Node1,
           ratio_ArtDrain_monthlyMeanNode1 = sumArtDrainByTimeStep/monthlyMeanNode1) %>%
    data.table()
  
  
  # UserTypes (Currently numbered like this as of 9/14/2016)
  UserType_table <- rbindlist(list(
    data.table(user = paste0('User', 1:19), UserType = 'SelfSupply_US'),
    data.table(user = paste0('User', 20:31), UserType = 'SelfSupply_CAN'),
    data.table(user = paste0('User', 32:80), UserType = 'PWS_US'),
    data.table(user = paste0('User', 81:89), UserType = 'Commercial_US'),
    data.table(user = paste0('User', 90:100), UserType = 'Commercial_CAN'),
    data.table(user = paste0('User', 101:115), UserType = 'Dairy_US'),
    data.table(user = paste0('User', c(116:126,130:140,142,145:161)), UserType = 'Irrigation_US'),
    data.table(user = paste0('User', c(127, 128, 129, 141, 143, 144)), UserType = 'Irrigation_CAN'),
    data.table(user = paste0('User', 162:163), UserType = 'Reservoir_US')), use.names = T, fill = T)
  
  
  # the average and sd of the average User
  monthlySummary_UserDemand_gpd.txt <- UserDemand_gpd.txt %>%
    # group by TimeStep and Month
    select(-c(TimeStep, yyyymmdd, year, day, water_year)) %>%
    melt(., id.vars = c('month'), variable.name = 'User', value.name = 'UserDemand_gpd') %>%
    mutate(UserType = UserType_table[match(User, user),UserType]) %>%
    
    # calculate mean by month, and UserType
    group_by(month, UserType) %>%
    mutate(meanByMonthByUserType = mean(UserDemand_gpd)) %>%
    
    # calculate mean and sd by month
    group_by(month) %>%
    mutate(meanByMonth = mean(UserDemand_gpd),
           sdByMonth = sd(UserDemand_gpd)) %>%
    
    # reformat the table to wide for each UserType
    select(-c(User, UserDemand_gpd)) %>%
    dcast(., month + meanByMonth + sdByMonth ~ UserType, fun = mean, value.var = c('meanByMonthByUserType')) %>%
    data.table()
  
  
  # the average monthly sum for each UserType
  monthlySums_UserDemand_gpd.txt <- UserDemand_gpd.txt %>%
    # group by month and water_year
    select(-c(TimeStep, yyyymmdd, year, day)) %>%
    melt(., id.vars = c('month','water_year'), variable.name = 'User', value.name = 'UserDemand_gpd') %>%
    mutate(UserType = UserType_table[match(User, user),UserType]) %>%
    
    # calculate sum by month, water_year, and UserType
    group_by(month, water_year, UserType) %>%
    mutate(UserDemand_gpm = sum(UserDemand_gpd)) %>%
    
    # calculate mean of the sums by month, and UserType
    group_by(month, UserType) %>%
    mutate(mean_UserDemand_gpm = mean(UserDemand_gpm)) %>%
    
    # calculate 
    group_by(month) %>%
    mutate(sumByMonth = sum(UserDemand_gpd),
           meanByMonth = mean(UserDemand_gpd),
           sdByMonth = sd(UserDemand_gpd),
           varByMonth = var(UserDemand_gpd)) %>%
    
    # reformat the table to wide for each UserType
    select(-c(User, water_year, UserDemand_gpd)) %>%
    dcast(., month + sumByMonth + meanByMonth + sdByMonth + varByMonth ~ UserType, fun = mean, value.var = c('mean_UserDemand_gpm')) %>%
    data.table()
  
  
  # the average annual sum for each UserType
  yearlySums_UserDemand_gpd.txt <- UserDemand_gpd.txt %>%
    # group by month and water_year
    select(-c(TimeStep, yyyymmdd, year, month, day)) %>%
    melt(., id.vars = c('water_year'), variable.name = 'User', value.name = 'UserDemand_gpd') %>%
    mutate(UserType = UserType_table[match(User, user),UserType]) %>%
    
    # calculate sum by water_year, and UserType
    group_by(water_year, UserType) %>%
    mutate(UserDemand_gpy = sum(UserDemand_gpd)) %>%
    
    # calculate sum by water_year
    group_by(water_year) %>%
    mutate(sumByWateryear = sum(UserDemand_gpd)) %>%
    
    # reformat the table to wide for each UserType
    select(-c(UserDemand_gpd, User)) %>%
    dcast(., water_year + sumByWateryear ~ UserType, fun = mean, value.var = c('UserDemand_gpy')) %>%
    data.table()
  
  
  # the average and sd of the average User
  monthlySummary_UserWithdrawal_gpd.txt <- UserWithdrawal_gpd.txt %>%
    # group by TimeStep and Month
    select(-c(TimeStep, yyyymmdd, year, day, water_year)) %>%
    melt(., id.vars = c('month'), variable.name = 'User', value.name = 'UserWithdrawal_gpd') %>%
    mutate(UserType = UserType_table[match(User, user),UserType]) %>%
    
    # calculate mean by month, and UserType
    group_by(month, UserType) %>%
    mutate(meanByMonthByUserType = mean(UserWithdrawal_gpd)) %>%
    
    # calculate mean and sd by month
    group_by(month) %>%
    mutate(meanByMonth = mean(UserWithdrawal_gpd),
           sdByMonth = sd(UserWithdrawal_gpd)) %>%
    
    # reformat the table to wide for each UserType
    select(-c(User, UserWithdrawal_gpd)) %>%
    dcast(., month + meanByMonth + sdByMonth ~ UserType, fun = mean, value.var = c('meanByMonthByUserType')) %>%
    data.table()
  
  
  # the average monthly sum for each UserType
  monthlySums_UserWithdrawal_gpd.txt <- UserWithdrawal_gpd.txt %>%
    # group by TimeStep and Month
    select(-c(TimeStep, yyyymmdd, year, day)) %>%
    melt(., id.vars = c('month','water_year'), variable.name = 'User', value.name = 'UserWithdrawal_gpd') %>%
    mutate(UserType = UserType_table[match(User, user),UserType]) %>%
    
    # calculate mean by month, water_year, and UserType
    group_by(month, water_year, UserType) %>%
    mutate(UserWithdrawal_gpm = sum(UserWithdrawal_gpd)) %>%
    
    # calculate mean of the sums by month, and UserType
    group_by(month, UserType) %>%
    mutate(mean_UserWithdrawal_gpm = mean(UserWithdrawal_gpm)) %>%
    
    # calculate 
    group_by(month) %>%
    mutate(sumByMonth = sum(UserWithdrawal_gpd),
           meanByMonth = mean(UserWithdrawal_gpd),
           sdByMonth = sd(UserWithdrawal_gpd),
           varByMonth = var(UserWithdrawal_gpd)) %>%
    
    # reformat the table to wide for each UserType
    select(-c(User, water_year, UserWithdrawal_gpd)) %>%
    dcast(., month + sumByMonth + meanByMonth + sdByMonth + varByMonth ~ UserType, fun = mean, value.var = c('mean_UserWithdrawal_gpm')) %>%
    data.table()
  
  
  # the average annual sum for each UserType
  yearlySums_UserWithdrawal_gpd.txt <- UserWithdrawal_gpd.txt %>%
    # group by month and water_year
    select(-c(TimeStep, yyyymmdd, year, month, day)) %>%
    melt(., id.vars = c('water_year'), variable.name = 'User', value.name = 'UserWithdrawal_gpd') %>%
    mutate(UserType = UserType_table[match(User, user),UserType]) %>%
    
    # calculate sum by water_year, and UserType
    group_by(water_year, UserType) %>%
    mutate(UserWithdrawal_gpy = sum(UserWithdrawal_gpd)) %>%
    
    # calculate sum by water_year
    group_by(water_year) %>%
    mutate(sumByWateryear = sum(UserWithdrawal_gpd)) %>%
    
    # reformat the table to wide for each UserType
    select(-c(UserWithdrawal_gpd, User)) %>%
    dcast(., water_year + sumByWateryear ~ UserType, fun = mean, value.var = c('UserWithdrawal_gpy')) %>%
    data.table()
  
  
  # function to read in the time series files 
  findmeta <- function(file){
    # index last row of metadata, depends on use of "Ver#" start to header line
    meta_end <- grep('^Ver', readLines(file))
    
    # metadata
    meta <- readLines(file)[1:meta_end] %>% 
      gsub("^\\t+|\\t+$|^\\s+|\\s+$","",.) %>% # removing leading and trailing spaces and tabs
      gsub("\\t+"," ",.) # separate tab-separated metadata
    
    print(paste("metadata headers for", file))
    return(meta)
  }
  
  # function to read in .dat format output files
  read_dat <- function(file, meta){
    # column names based on tab separation
    col.Names <- meta %>% gsub("\\t+$","",.) %>%
      strsplit(' |\t') %>% 
      tail(1) %>%
      unlist() %>%
      .[-c(1,2)] %>%
      .[.!=""]
    
    # arrange the data
    dat <- read.csv(file, sep='\t', skip = length(meta), header = F)
    nonNA <- colnames(dat)[which(colSums(is.na(dat)) != nrow(dat))]
    dat <- dat[colnames(dat) %in% nonNA] %>% data.table()
    setnames(dat, colnames(dat), col.Names)
    
    return(dat)
  }
  
  # find meta lines for rain.dat
  rain.dat_meta <- findmeta('rain.dat')
  
  # read in rain.dat
  rain.dat <- read_dat('rain.dat', rain.dat_meta)
  
  # apply conversion from mm to inches. Code-base taken from Precipitation.
  # convert mm to inches
  rain_in_obs.dat <- rain.dat %>%
    mutate_each(funs(./25.4), -c(Date, Hour)) %>%
    merge(., DateTime_water_year.txt, by.x = 'Date', by.y = 'yyyymmdd') %>% 
    arrange(TimeStep) %>%
    data.table() %>%
    select(-c(Date, year, day, Hour, hhmmss)) 
  
  
  # monthly average for the 9 water years
  monthlySummary_rain_in.dat <- rain_in_obs.dat %>%
    # group by TimeStep and Month
    melt(., id.vars = c('TimeStep', 'water_year', 'month'), variable.name = 'zone_code', value.name = 'rain_in_obs') %>%
    mutate(zone_code = paste0('zone_',zone_code)) %>%
    
    # calculate total by month, water_year, and zone_code
    group_by(month, water_year, zone_code) %>%
    mutate(sumByMonthByWateryearByZonecode = sum(rain_in_obs)) %>%
    
    # calculate mean by month, and drainage
    group_by(month, zone_code) %>%
    mutate(mean_rain_in_obs = mean(sumByMonthByWateryearByZonecode)) %>%
    
    # calculate mean and sd by month
    group_by(month) %>%
    mutate(meanSumByMonth = mean(sumByMonthByWateryearByZonecode),
           meanByMonth = mean(rain_in_obs),
           sdByMonth = sd(rain_in_obs)) %>%
    
    # reformat the table to wide for each DrainageID
    select(-c(TimeStep, water_year, rain_in_obs, sumByMonthByWateryearByZonecode)) %>%
    dcast(., month + meanSumByMonth + meanByMonth + sdByMonth ~ zone_code, fun = mean, value.var = 'mean_rain_in_obs') %>%
    data.table()
  
  
  # annual average for the 9 water years
  yearlySummary_rain_in_obs.dat <- rain_in_obs.dat %>%
    # group by TimeStep and Month
    select(-c(TimeStep)) %>%
    melt(., id.vars = c('month', 'water_year'), variable.name = 'zone_code', value.name = 'rain_in_obs') %>%
    mutate(zone_code = paste0('zone_',zone_code)) %>%
    
    # calculate total by month, wateryear, and drainage
    group_by(month, water_year, zone_code) %>%
    mutate(sumByMonthByWateryearByZonecode = sum(rain_in_obs)) %>%
    
    # calculate mean by wateryear, and drainage
    group_by(water_year, zone_code) %>%
    mutate(meanByWateryearByZonecode = mean(sumByMonthByWateryearByZonecode)) %>%
    
    # calculate mean and sd by month
    group_by(water_year) %>%
    mutate(sumByWateryear = sum(rain_in_obs),
           meanByWateryear = mean(sumByMonthByWateryearByZonecode),
           sdByWateryear = sd(sumByMonthByWateryearByZonecode)) %>%
    
    # reformat the table to wide for each DrainageID
    select(-c(month, rain_in_obs, sumByMonthByWateryearByZonecode)) %>%
    dcast(., water_year + sumByWateryear + meanByWateryear + sdByWateryear ~ zone_code, fun = mean, value.var = 'meanByWateryearByZonecode') %>%
    data.table()
  
  
  # find meta lines for streamflow_calibration.dat
  streamflow_calibration.dat_meta <- findmeta('streamflow_calibration.dat')
  col.Names <- streamflow_calibration.dat_meta %>% gsub("\\t+$","",.) %>% strsplit(' |\t') %>% tail(1) %>% unlist() %>% .[-c(1,2)] %>% .[.!=""]
  
  # read in streamflow_calibration.dat using readlines approach. read_dat was not working due to extra space characters.
  # streamflow_calibration.dat <- read_dat('streamflow_calibration.dat', streamflow_calibration.dat_meta)
  dat <- readLines('streamflow_calibration.dat', skip = length(streamflow_calibration.dat_meta)) %>% 
    .[-c(1:4)] %>% 
    gsub('^[[:space:]]+|[[:space:]]+$','', .) %>% 
    gsub('[[:space:]]+','\t', .) %>% 
    strsplit(., '\t') %>% 
    do.call(rbind, .) %>% 
    data.table
  nonNA <- colnames(dat)[which(colSums(is.na(dat)) != nrow(dat))]
  dat <- dat[colnames(dat) %in% nonNA] %>% data.table()
  setnames(dat, colnames(dat), paste0('SF',col.Names))
  streamflow_calibration.dat <- dat %>% mutate_each(funs(as.numeric(.))) %>% data.table()
  
  
  # convert cms to cfs: 1cms = 35.3147 cfs
  FlowAtStreamNodes_calib_cfs.txt <- FlowAtStreamNodes_cms.txt %>%
    mutate_each(funs(.*35.3147), -TimeStep) %>%
    merge(., DateTime_calib.txt, by = 'TimeStep') %>% 
    mutate(FlowAtOutlet = Node1) %>% # 
    arrange(TimeStep) %>% 
    select(TimeStep, yyyymmdd, year, month, day, water_year, contains('Node')) %>%
    data.table()
  
  
  # convert cms to cfs: 1cms = 35.3147 cfs
  FlowAtStreamNodes_valid_cfs.txt <- FlowAtStreamNodes_cms.txt %>%
    mutate_each(funs(.*35.3147), -TimeStep) %>%
    merge(., DateTime_valid.txt, by = 'TimeStep', all.y = T) %>% 
    mutate(FlowAtOutlet = Node1) %>% # 
    arrange(TimeStep) %>% 
    select(TimeStep, yyyymmdd, year, month, day, water_year, contains('Node')) %>%
    data.table()
  
  
  # multiple flowatstreamnodes
  FlowAtStreamNodes_calib_cfs.txt
  FlowAtStreamNodes_valid_cfs.txt
  FlowAtStreamNodes_cfs.txt
  
  # compare node4 (streamflow_calibration1) and node28 (streamflow_calibration2)
  # the comparison between FlowAtStreamNodes (model output) and streamflow_calibration (obs)
  
  # rename the metadata file
  streamflow_calibration_calib_cfs.dat_meta <- streamflow_calibration.dat_meta
  
  # adjust streamflow_calibration to cfs and filter for the calibration date range
  streamflow_calibration_calib_cfs.dat <- streamflow_calibration.dat %>% 
    # convert cms to cfs
    mutate_each(funs(.*35.3147)) %>%
    mutate(SF152 = ifelse(SF152 == -999*35.3147, NA, SF152),
           SF236 = ifelse(SF236 == -999*35.3147, NA, SF236)) %>%
    
    # add date sequence and convert to the %Y%m%d format
    mutate(date = seq(from = as.Date('20030613', format = '%Y%m%d'), by = 'day', length.out = nrow(streamflow_calibration.dat)),
           date = format(date, '%Y%m%d')) %>%
    # filter for the respective model run dates
    filter(date %in% FlowAtStreamNodes_calib_cfs.txt[,yyyymmdd]) %>%
    data.table()
  
  NSEc1 <- NSE(sim=FlowAtStreamNodes_calib_cfs.txt[,Node4], obs = streamflow_calibration_calib_cfs.dat[,SF152], na.rm=T)
  NSEc2 <- NSE(sim = FlowAtStreamNodes_calib_cfs.txt[,Node28], obs = streamflow_calibration_calib_cfs.dat[,SF236], na.rm=T)
  
  
  # rename the metadata file
  streamflow_calibration_valid_cfs.dat_meta <- streamflow_calibration.dat_meta
  
  # adjust streamflow_calibration to cfs and filter for the validation date range
  streamflow_calibration_valid_cfs.dat <- streamflow_calibration.dat %>% 
    # convert cms to cfs
    mutate_each(funs(.*35.3147)) %>%
    mutate(SF152 = ifelse(SF152 == -999*35.3147, NA, SF152),
           SF236 = ifelse(SF236 == -999*35.3147, NA, SF236)) %>%
    
    # add date sequence and convert to the %Y%m%d format
    mutate(date = seq(from = as.Date('20030613', format = '%Y%m%d'), by = 'day', length.out = nrow(streamflow_calibration.dat)),
           date = format(date, '%Y%m%d')) %>%
    # filter for the respective model run dates
    filter(date %in% FlowAtStreamNodes_valid_cfs.txt[,yyyymmdd]) %>%
    data.table()
  
  # obs data update for column 1
  BertrandStreamflow <- read.table(paste(ObsStreamflowData,'BertrandStreamflow.txt', sep='/'), sep = '\t', header = T) %>% 
    mutate(Date = format(as.Date(Date, format = '%M/%d/%Y'), '%Y%m%d')) %>%
    setnames(., colnames(.), c('Date', 'Flow_cfs')) %>%
    data.table()
  
  CanadaStreamflow <- read.table(paste(ObsStreamflowData,'Daily__Sep-15-2016_10_14_39PM__ddf.txt', sep='/'), sep = '\t', header = T) %>% 
    mutate(Date = format(as.Date(Date, format = '%M/%d/%Y'), '%Y%m%d')) %>%
    data.table()
  
  # add the missing values
  streamflow_calibration_valid_cfs.dat <- streamflow_calibration_valid_cfs.dat %>%
    mutate(SF152 = ifelse(is.na(SF152), BertrandStreamflow[match(date, Date), Flow_cfs], SF152),
           SF236 = ifelse(is.na(SF236), CanadaStreamflow[match(date, Date), Value]*35.3147, SF236)) %>%
    mutate(date = as.integer(date)) %>%
    data.table()
  
  FlowAtStreamNodes_valid_cfs.txt <- FlowAtStreamNodes_valid_cfs.txt[yyyymmdd %in% streamflow_calibration_valid_cfs.dat[,date],]
  
  NSEv1 <- NSE(sim = FlowAtStreamNodes_valid_cfs.txt[,Node4], obs = streamflow_calibration_valid_cfs.dat[,SF152], na.rm=T)
  NSEv2 <- NSE(sim = FlowAtStreamNodes_valid_cfs.txt[,Node28], obs = streamflow_calibration_valid_cfs.dat[,SF236], na.rm=T)
  
  
  # generate the NSE summary table
  NSE_Summary.txt <- rbind(
    c('calibration', paste0(calib_start, '-', calib_end), NSEc1, NSEc2),
    c('validation', paste0(valid_start, '-', valid_end), NSEv1, NSEv2)) %>%
    data.frame() %>%
    setnames(., colnames(.), c('NSE_mode','DateRange','Node4_SF152', 'Node28_SF236')) %>%
    data.table()
  
  
  # print to txt files
  for (j in c('Precipitation_in.txt', 
              'Evaporation_in.txt',
              'FlowAtStreamNodes_cfs.txt',
              'Artificial_Drainage_cfs.txt',
              'UserDemand_gpd.txt',
              'UserWithdrawal_gpd.txt',
              'DateTime_water_year.txt',
              'rain_in_obs.dat',
              "monthlySummary_Artificial_Drainage_cfs.txt",
              "monthlySummary_Evaporation_in.txt",
              "monthlySummary_Precipitation_in.txt",
              "monthlySummary_FlowAtStreamNodes_cfs.txt",
              "monthlySummary_UserDemand_gpd.txt",
              "monthlySummary_UserWithdrawal_gpd.txt",
              "yearlySummary_Evaporation_in.txt",
              "yearlySummary_Precipitation_in.txt",
              "AD_FASN_Summary",
              "monthlySums_UserDemand_gpd.txt",
              "monthlySums_UserWithdrawal_gpd.txt",
              "streamflow_calibration_calib_cfs.dat",
              "streamflow_calibration_valid_cfs.dat")){
    
    # start
    print(paste('start', j))
    
    # separate protocol for .dat files
    if(grepl('.dat',j)){
      write.table(j %>% gsub('_in_obs','',.) %>% paste0(., '_meta') %>% get(), file = j, append = F, sep = '\t', row.names = F, col.names = F, quote = F)
      write.table(j, file = j, append = T, sep = '\t', row.names = F, col.names = F)
    } else {
      write.table(j %>% get(), file = j, col.names = T, row.names = F, append = F)
    }
    
    # finished
    print(paste('finished', j))
  }


  # deposit the monthly and yearly summaries into an xlsx workbook
  # print to xlsx workbook
  # if the storage file already exists, print a null
  
  # joanie's tab name preferences
  tab_names <- rbindlist(list(
    data.table(iname = 'monthlySummary_Artificial_Drainage_cfs.txt', jname = 'MonAve_ArtDrain_cfs'),
    data.table(iname = 'monthlySummary_Evaporation_in.txt', jname = 'MonSum__Evap_in'),
    data.table(iname = 'monthlySummary_FlowAtStreamNodes_cfs.txt', jname = 'MonAve_FlowAtStreamNodes_cfs'),
    data.table(iname = 'monthlySummary_Precipitation_in.txt', jname = 'MonAve_Precip_in'),
    data.table(iname = 'monthlySummary_rain_in.dat', jname = 'ObsMonAve_Precip_in'),
    data.table(iname = 'monthlySummary_UserDemand_gpd.txt', jname = 'AveDaily_UserDemand_gpd'),
    data.table(iname = 'monthlySums_UserDemand_gpd.txt', jname = 'MonSum_UserDemand_Gallons'),
    data.table(iname = 'yearlySummary_Evaporation_in.txt', jname = 'AnnSumSubbasin_Evap_in'),
    data.table(iname = 'yearlySummary_Precipitation_in.txt', jname = 'AnnSumSubbasin_Precip_in')), use.names = T, fill = T)
  
  if("modelruns_summary.xlsx" %in% list.files()){
    write.xlsx2(ls()[grepl('table|monthly|yearly|Summary',ls())], file = "modelruns_summary.xlsx", sheetName = 'tableofcontents', col.names = F, row.names = F, append = F)
  }
  
  for (j in ls()){
    if(grepl('table|monthly|yearly|Summary',j)){
      
      # skip these summaries from inclusion into the summary xlsx
      if(j %in% c('monthlySums_UserWithdrawal_gpd.txt', 'monthlySummary_UserWithdrawal_gpd.txt')){
        next
      }
      
      print(paste('start', j))
      
      write.xlsx2(j %>% get(), file = "modelruns_summary.xlsx", 
                  sheetName = ifelse(j %in% tab_names[,iname], tab_names[iname == j,jname], j) %>% gsub('.txt|.dat','', .),
                  col.names = T, row.names = F, append = T)
      
      ifelse(j %in% tab_names[,iname], print(paste(j, 'transformed and finished as', tab_names[iname == j,jname], j)), print(paste('finished', j)))
    }
  }
}