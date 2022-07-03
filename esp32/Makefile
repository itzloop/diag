ROOT=$(shell pwd)

diag:
	pio run -d $(ROOT)/diag -t upload
	pio device monitor -d $(ROOT)/diag

diagmon:
	pio device monitor -d $(ROOT)/diag

ecu:
	pio run -d $(ROOT)/ecu -t upload
	pio device monitor -d $(ROOT)/ecu 

ecumon:
	pio device monitor -d $(ROOT)/ecu 
