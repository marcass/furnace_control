rrdtool graph /home/marcus/status_plot.png -a PNG \
'DEF:f=/tmp/boiler/flame.rrd:boiler_flame:AVERAGE' \
'DEF:t_water=/tmp/boiler/temp/water.rrd:boiler_temp_water:AVERAGE' \
'DEF:t_auger=/tmp/boiler/temp/auger.rrd:boiler_temp_auger:AVERAGE' \
'LINE1:f#ff0000:Flame' \
'LINE2:t_water#00ff00:Water Temperature' \
'LINE3:t_auger#0000ff:Auger Temperature' \

scp /home/marcus/status_plot.png mw@htpc:~/
