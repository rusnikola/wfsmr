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
	cmd = "metacmd.py main -i 10 -depochf=110 -demptyf=120 -m 3 -v -r 11"+\
	" --meta t:1:16:32:48:64:80:96"+\
	" --meta d:tracker=NIL:tracker=RCU:tracker=Range_new:tracker=HE:tracker=Hazard:tracker=HR:tracker=WFE:tracker=WFR:tracker=HyalineOEL:tracker=HyalineOSEL"+\
	" -o data/final/crturn_result.csv"
	os.system(cmd)
	cmd = "metacmd.py main -i 10 -depochf=110 -demptyf=120 -m 3 -v -c -r 11"+\
	" --meta t:1:16:32:48:64:80:96"+\
	" --meta d:tracker=NIL:tracker=RCU:tracker=Range_new:tracker=HE:tracker=Hazard:tracker=HR:tracker=WFE:tracker=WFR:tracker=HyalineOEL:tracker=HyalineOSEL"+\
	" -o data/final/crturn_result_retired.csv"
	os.system(cmd)
