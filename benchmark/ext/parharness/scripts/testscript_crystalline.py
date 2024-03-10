#!/usr/bin/python


from os.path import dirname, realpath, sep, pardir
import sys
import os

# path loading ----------
#print dirname(realpath(__file__))
os.environ['PATH'] = dirname(realpath(__file__))+\
":" + os.environ['PATH'] # scripts
os.environ['PATH'] = dirname(realpath(__file__))+\
"/..:" + os.environ['PATH'] # bin
os.environ['PATH'] = dirname(realpath(__file__))+\
"/../../../bin:" + os.environ['PATH'] # metacmd

#"/../../cpp_harness:" + os.environ['PATH'] # metacmd

# execution ----------------
for i in range(0,5):
	cmd = "metacmd.py main -i 10 -depochf=110 -demptyf=120 -m 3 -v -r 1"+\
	" --meta t:1:12:24:36:48:60:72:84:96:108:120:132:144:156:168:180:192"+\
	" --meta d:tracker=NIL:tracker=RCU:tracker=Range_new:tracker=HE:tracker=Hazard:tracker=HR:tracker=WFE:tracker=WFR:tracker=HyalineOEL:tracker=HyalineOSEL"+\
	" -o data/final/hashmap_result.csv"
	os.system(cmd)
	cmd = "metacmd.py main -i 10 -depochf=110 -demptyf=120 -m 3 -v -r 2"+\
	" --meta t:1:12:24:36:48:60:72:84:96:108:120:132:144:156:168:180:192"+\
	" --meta d:tracker=NIL:tracker=RCU:tracker=Range_new:tracker=HE:tracker=Hazard:tracker=HR:tracker=WFE:tracker=WFR:tracker=HyalineOEL:tracker=HyalineOSEL"+\
	" -o data/final/list_result.csv"
	os.system(cmd)
	cmd = "metacmd.py main -i 10 -depochf=110 -demptyf=120 -m 3 -v -c -r 1"+\
	" --meta t:1:12:24:36:48:60:72:84:96:108:120:132:144:156:168:180:192"+\
	" --meta d:tracker=NIL:tracker=RCU:tracker=Range_new:tracker=HE:tracker=Hazard:tracker=HR:tracker=WFE:tracker=WFR:tracker=HyalineOEL:tracker=HyalineOSEL"+\
	" -o data/final/hashmap_result_retired.csv"
	os.system(cmd)
	cmd = "metacmd.py main -i 10 -depochf=110 -demptyf=120 -m 3 -v -c -r 2"+\
	" --meta t:1:12:24:36:48:60:72:84:96:108:120:132:144:156:168:180:192"+\
	" --meta d:tracker=NIL:tracker=RCU:tracker=Range_new:tracker=HE:tracker=Hazard:tracker=HR:tracker=WFE:tracker=WFR:tracker=HyalineOEL:tracker=HyalineOSEL"+\
	" -o data/final/list_result_retired.csv"
	os.system(cmd)
	cmd = "metacmd.py main -i 10 -depochf=110 -demptyf=120 -m 2 -v -r 1"+\
	" --meta t:1:12:24:36:48:60:72:84:96:108:120:132:144:156:168:180:192"+\
	" --meta d:tracker=NIL:tracker=RCU:tracker=Range_new:tracker=HE:tracker=Hazard:tracker=HR:tracker=WFE:tracker=WFR:tracker=HyalineOEL:tracker=HyalineOSEL"+\
	" -o data/final/hashmap_result_read.csv"
	os.system(cmd)
	cmd = "metacmd.py main -i 10 -depochf=110 -demptyf=120 -m 2 -v -r 2"+\
	" --meta t:1:12:24:36:48:60:72:84:96:108:120:132:144:156:168:180:192"+\
	" --meta d:tracker=NIL:tracker=RCU:tracker=Range_new:tracker=HE:tracker=Hazard:tracker=HR:tracker=WFE:tracker=WFR:tracker=HyalineOEL:tracker=HyalineOSEL"+\
	" -o data/final/list_result_read.csv"
	os.system(cmd)
	cmd = "metacmd.py main -i 10 -depochf=110 -demptyf=120 -m 2 -v -c -r 1"+\
	" --meta t:1:12:24:36:48:60:72:84:96:108:120:132:144:156:168:180:192"+\
	" --meta d:tracker=NIL:tracker=RCU:tracker=Range_new:tracker=HE:tracker=Hazard:tracker=HR:tracker=WFE:tracker=WFR:tracker=HyalineOEL:tracker=HyalineOSEL"+\
	" -o data/final/hashmap_result_retired_read.csv"
	os.system(cmd)
	cmd = "metacmd.py main -i 10 -depochf=110 -demptyf=120 -m 2 -v -c -r 2"+\
	" --meta t:1:12:24:36:48:60:72:84:96:108:120:132:144:156:168:180:192"+\
	" --meta d:tracker=NIL:tracker=RCU:tracker=Range_new:tracker=HE:tracker=Hazard:tracker=HR:tracker=WFE:tracker=WFR:tracker=HyalineOEL:tracker=HyalineOSEL"+\
	" -o data/final/list_result_retired_read.csv"
	os.system(cmd)
