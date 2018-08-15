from influxdb import InfluxDBClient
from datetime import timedelta
import datetime
import time

db_name = 'sensors'
# setup db
client = InfluxDBClient(host='localhost', port=8086)
# setup db if it ins't already:
def setup_db():
    print 'checking for db'
    flag = False
    dbs = client.get_list_database()
    print dbs
    for i in client.get_list_database():
        if db_name in i['name']:
            flag = True
        else:
            print 'doing nothing'
    if not flag:
        print 'making db'
        client.create_database(db_name)

# setup db and use currnet db
setup_db()
client.switch_database(db_name)
# client.drop_database(db_name)

# setup reteniton plicy list must match index of durations
retention_policies = ['24_hours', '7_days','2_months', '1_year', '5_years']
# setup retention policy detail
durations = {'24_hours': {'dur':'1d', 'default':True},
             '7_days': {'dur':'7d', 'default':False},
             '2_months': {'dur':'4w', 'default':False},
             '1_year': {'dur':'52w','default':False},
             '5_years': {'dur':'260w','default':False}}
# orgainse graphing periods
periods = {'hours': ['24_hours'], 'days': ['7_days', '2_months'], 'months': ['1_year'], 'years': ['5_years']}
def setup_RP(meas):
    global retention_policies
    global measurement
    RP_list = []
    # try:
    RP = client.get_list_retention_policies(db_name)
    for i in RP:
        # produce list of existing retention policies
        RP_list.append(i['name'])
    # print RP_list
    # except:
    #     print 'No retention polices here'
    for i in retention_policies:
        if i in RP_list:
            print 'RP already here'
        else:
            print 'making rp for '+i
            client.create_retention_policy(i, durations[i]['dur'], 1, database='sensors', default=durations[i]['default'])
    # https://influxdb-python.readthedocs.io/en/latest/api-documentation.html
    # https://docs.influxdata.com/influxdb/v1.6/guides/downsampling_and_retention/
    try:
        # client.query('CREATE CONTINUOUS QUERY \"%s\" ON %s BEGIN SELECT mean(temp) AS "temp", mean(humidity) AS "humidity", mean(light) AS "light" INTO \"%s\".values_7d FROM \"%s\" GROUP BY time(5m), * END' %(meas+'_cq_7_days', db_name, meas+'_7_days', meas))
        # client.query('CREATE CONTINUOUS QUERY \"%s\" ON %s BEGIN SELECT mean(temp) AS "temp", mean(humidity) AS "humidity", mean(light) AS "light" INTO \"%s\".values_2mo FROM \"%s\" GROUP BY time(10m), * END' %(meas+'_cq_2_months', db_name, meas+'_2_months', meas))
        # client.query('CREATE CONTINUOUS QUERY \"%s\" ON %s BEGIN SELECT mean(temp) AS "temp", mean(humidity) AS "humidity", mean(light) AS "light" INTO \"%s\".values_1y FROM \"%s\" GROUP BY time(20m), * END' %(meas+'_cq_1_year', db_name, meas+'_1_year', meas))
        # client.query('CREATE CONTINUOUS QUERY \"%s\" ON %s BEGIN SELECT mean(temp) AS "temp", mean(humidity) AS "humidity", mean(light) AS "light" INTO \"%s\".values_5y FROM \"%s\" GROUP BY time(30m), * END' %(meas+'_cq_5_years', db_name, meas+'_5_years', meas))
        client.query('CREATE CONTINUOUS QUERY \"%s\" ON %s BEGIN SELECT mean(%s) AS \"%s\" INTO "7_days".%s FROM "24_hours".\"%s\" GROUP BY time(5m), * END' %(meas+'_cq_7_days', db_name, meas, meas, meas, meas))
        client.query('CREATE CONTINUOUS QUERY \"%s\" ON %s BEGIN SELECT mean(%s) AS \"%s\" INTO "2_months".%s FROM "24_hours".\"%s\" GROUP BY time(10m), * END' %(meas+'_cq_2_months', db_name, meas, meas, meas, meas))
        client.query('CREATE CONTINUOUS QUERY \"%s\" ON %s BEGIN SELECT mean(%s) AS \"%s\" INTO "1_year".%s FROM "24_hours".\"%s\" GROUP BY time(20m), * END' %(meas+'_cq_1_year', db_name, meas, meas, meas, meas))
        client.query('CREATE CONTINUOUS QUERY \"%s\" ON %s BEGIN SELECT mean(%s) AS \"%s\" INTO "5_years".%s FROM "24_hours".\"%s\" GROUP BY time(30m), * END' %(meas+'_cq_5_years', db_name, meas, meas, meas, meas))
        print 'making cqs for '+meas
    except:
        # already exist
        print "Failed to create CQs for "+meas+", as it already exists"

measurement = []

def write_data(json):
    # ensure RP's and CQ's in place for new sites
    global measurement
    if json['type'] not in measurement:
        measurement.append(json['type'])
        setup_RP(json['type'])
    try:
        json_data = [
            {
                'measurement': json['type'],
                'tags': {
                    'sensorID': json['sensor'],
                    'site': json['group'],
                    'type': json['type']
                },
                'fields': {
                    json['type']: json['value']
                },
                'time': datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%fZ")
                }
            ]
        client.write_points(json_data)
        return {'Status': 'success', 'Message': 'successfully wrote data points'}
    except:
        return {'Status': 'error', 'Messgage': 'failed to wrote data points'}
