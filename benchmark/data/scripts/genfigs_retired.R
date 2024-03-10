# Copyright 2015 University of Rochester
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License. 

###############################################################
### This script generates the 8 plots that were actually    ###
### used in the paper using the data contained in ../final/ ###
###############################################################

library(plyr)
library(ggplot2)

filenames<-c("hashmap","list")
for (f in filenames){
read.csv(paste("../final/",f,"_result_retired.csv",sep=""))->lindata

lindata$environment<-as.factor(gsub("emptyf=120:epochf=110:tracker=RCU","EBR",lindata$environment))
lindata$environment<-as.factor(gsub("emptyf=120:epochf=110:tracker=Hazard","HP",lindata$environment))
lindata$environment<-as.factor(gsub("emptyf=120:epochf=110:tracker=NIL","None",lindata$environment))
lindata$environment<-as.factor(gsub("emptyf=120:epochf=110:tracker=HyalineOSEL","Hyaline-1S",lindata$environment))
lindata$environment<-as.factor(gsub("emptyf=120:epochf=110:tracker=HyalineOEL","Hyaline-1",lindata$environment))
lindata$environment<-as.factor(gsub("emptyf=120:epochf=110:tracker=WFR","Crystalline-W",lindata$environment))
lindata$environment<-as.factor(gsub("emptyf=120:epochf=110:tracker=HR","Crystalline-L",lindata$environment))
lindata$environment<-as.factor(gsub("emptyf=120:epochf=110:tracker=WFE","WFE",lindata$environment))
lindata$environment<-as.factor(gsub("emptyf=120:epochf=110:tracker=HE","HE",lindata$environment))
lindata$environment<-as.factor(gsub("emptyf=120:epochf=110:tracker=Range_new","IBR",lindata$environment))

# Compute average and max retired objects per operation from raw data
ddply(.data=lindata,.(environment,threads),mutate,retired_avg= min(obj_retired)/(mean(ops)))->lindata
ddply(.data=lindata,.(environment,threads),mutate,ops_max= max(ops)/(interval*1000000))->lindata

rcudatalin <- subset(lindata,environment=="EBR")
hazarddatalin <- subset(lindata,environment=="HP")
hrdatalin <- subset(lindata,environment=="Crystalline-L")
wfrdatalin <- subset(lindata,environment=="Crystalline-W")
frdatalin <- subset(lindata,environment=="Hyaline-1")
frrdatalin <- subset(lindata,environment=="Hyaline-1S")
wfedatalin <- subset(lindata,environment=="WFE")
hedatalin <- subset(lindata,environment=="HE")
rangenewdatalin <- subset(lindata,environment=="IBR")

lindata = rbind(rcudatalin, rangenewdatalin, wfrdatalin, hrdatalin, frrdatalin, frdatalin, wfedatalin, hedatalin, hazarddatalin)
lindata$environment <- factor(lindata$environment, levels=c("EBR", "WFE", "HE", "IBR", "Crystalline-W", "Crystalline-L", "Hyaline-1S", "Hyaline-1", "HP"))

# Set up colors and shapes (invariant for all plots)
color_key = c("#0000FF", "#0066FF", "#FF0000", "#FF007F",
              "#1BC40F", "#DA9100",
              "#013220", "#3EB489", "#800080")
names(color_key) <- unique(c(as.character(lindata$environment)))

shape_key = c(5,0,6,4,2,62,1,3,18)
names(shape_key) <- unique(c(as.character(lindata$environment)))

line_key = c(1,4,1,2,1,2,1,2,1)
names(line_key) <- unique(c(as.character(lindata$environment)))


##########################################
#### Begin charts for retired objects ####
##########################################

legend_pos=c(0.5,0.86)
y_range_down = 0
y_range_up = 2000

# Benchmark-specific plot formatting
if(f=="bonsai"){
  y_range_down=0
  legend_pos=c(0.45,0.9)
  y_range_up=1700
}else if(f=="list"){
  y_range_down=0
  y_range_up=1850
}else if(f=="crturn"){
  y_range_down=0
  y_range_up=6250
}else if(f=="hashmap"){
  y_range_up=3950
}else if(f=="natarajan"){
  y_range_up=4900
}

# Generate the plots
linchart<-ggplot(data=lindata,
                  aes(x=threads,y=retired_avg,color=environment, shape=environment, linetype=environment))+
  geom_line()+xlab("Threads")+ylab("Retired Objects per Operation")+geom_point(size=4)+
  scale_shape_manual(values=shape_key[names(shape_key) %in% lindata$environment])+
  scale_linetype_manual(values=line_key[names(line_key) %in% lindata$environment])+
  theme_bw()+ guides(shape=guide_legend(title=NULL,nrow = 4))+ 
  guides(color=guide_legend(title=NULL,nrow = 4))+
  guides(linetype=guide_legend(title=NULL,nrow = 4))+
  scale_color_manual(values=color_key[names(color_key) %in% lindata$environment])+
  scale_x_continuous(breaks=c(1,16,32,48,64,80,96,128,160,192),
                minor_breaks=c(1,16,32,48,64,80,96,112,128,144,160,176,192))+
  theme(plot.margin = unit(c(.2,0,.2,0), "cm"))+
  theme(legend.position=legend_pos,
     legend.direction="horizontal")+
  theme(text = element_text(size = 20))+
  theme(axis.title.y = element_text(margin = margin(t = 0, r = 5, b = 0, l = 5)))+
  theme(axis.title.x = element_text(margin = margin(t = 5, r = 0, b = 5, l = 0)))+
  theme(panel.border = element_rect(size = 1.5))+
  ylim(y_range_down,y_range_up)

# Save all four plots to separate PDFs
ggsave(filename = paste("../final/",f,"_linchart_retired.pdf",sep=""),linchart,width=5, height = 5, units = "in", dpi=300)

}
