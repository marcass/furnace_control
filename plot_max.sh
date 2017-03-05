#!/bin/bash
PREFIX=/home/mw/git/furnace_control/status_plot
for INTERVAL in 1 2 5 24 72
  do
	rrdtool graph ${PREFIX}_temp_${INTERVAL}.png -a PNG \
	--end now --start end-${INTERVAL}h --width 800 \
	'DEF:t_water=/tmp/boiler/temp/water.rrd:boiler_temp_water:AVERAGE' \
	'DEF:t_auger=/tmp/boiler/temp/auger.rrd:boiler_temp_auger:AVERAGE' \
	'LINE2:t_water#00ff00:Water Temperature' \
	'LINE3:t_auger#0000ff:Auger Temperature' \

	rrdtool graph ${PREFIX}_flame_${INTERVAL}.png -a PNG \
	--end now --start end-${INTERVAL}h --width 800 \
	'DEF:f=/tmp/boiler/flame.rrd:boiler_flame:AVERAGE' \
	'LINE1:f#ff0000:Flame' 
  done

scp /home/marcus/status_plot.* mw@htpc:~/
