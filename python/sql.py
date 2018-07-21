import pytz
import sys
import time
import sqlite3
from dateutil import parser
from passlib.hash import pbkdf2_sha256
import datetime
from datetime import timedelta

users_db = '/home/marcus/git/arduino_access_control/python/door_database.db'
tz = 'Pacific/Auckland'

def localtime_from_response(resp):
    ts = datetime.datetime.strptime(resp, "%Y-%m-%d %H:%M:%S.%f")
    ts = ts.replace(tzinfo=pytz.UTC)
    return ts.astimezone(pytz.timezone(tz))

def utc_from_string(payload):
    local = pytz.timezone(tz)
    try:
        naive = datetime.datetime.strptime(payload, "%Y-%m-%dT%H:%M:%S.%fZ")
    except:
        # print 'not first format'
        pass
        try:
            naive = datetime.datetime.strptime(payload, "%a, %b %d %Y, %H:%M")
            # print 'successful convertion'
        except:
            # print 'problem with time string format'
            pass
            try:
                naive = datetime.datetime.strptime(payload, "%Y-%m-%d %H:%M:%S.%f")
            except:
                return 'failed'
    local_dt = local.localize(naive, is_dst=None)
    utc_dt = local_dt.astimezone(pytz.utc)
    return utc_dt


def get_db():
    conn = sqlite3.connect(users_db)
    c = conn.cursor()
    return conn, c

def setup_db():
    # Create table
    conn, c = get_db()
    #type is of 'mqtt', 'keycode' or 'rfid'
    c.execute('''CREATE TABLE IF NOT EXISTS boiler
                    (timestamp TIMESTAMP, state TEXT, water INTEGER, auger INTEGER, fan INTEGER, pause INTEGER, feed INTEGER, setpoint INTEGER)''')
    conn.commit() # Save (commit) the changes

def setup_admin_user(user, passw):
    conn, c = get_db()
    c.execute("SELECT * FROM userAuth")
    if len(c.fetchall()) > 0:
        return
    else:
        pw_hash = pbkdf2_sha256.hash(passw)
        c.execute("INSERT INTO userAuth VALUES (?,?,?)", (user, pw_hash, 'admin'))
        conn.commit()

#######  Get data #############################
def get_doorlog(door, resp):
    conn, c = get_db()
    # conn1, c1 = get_db()
    start = resp['timeStart']
    end = resp['timeEnd']
    if (start is not None) and (len(start) > 0):
        timeStart = utc_from_string(start)
    else:
        timeStart = datetime.datetime.utcnow() - timedelta(days=1)
    if (end is not None) and (len(end) > 0):
        timeEnd = utc_from_string(end)
    else:
        timeEnd = datetime.datetime.utcnow()
    c.execute("SELECT * FROM actionLog WHERE door=? AND timestamp BETWEEN datetime(?) AND datetime(?) ORDER BY timestamp DESC", (door, timeStart, timeEnd))
    ret = c.fetchall()
    message = [i[1] for i in ret]
    actionType = [i[2] for i in ret]
    actionTime = [localtime_from_response(i[0]) for i in ret]
    # door = [i[3] for i in ret]
    # dump = []
    # for a in ret:
    #     dump = dump+[localtime_from_response(a[0]),a[1],a[2]]
    c.execute("SELECT * FROM doorStates WHERE door=? AND timestamp BETWEEN datetime(?) AND datetime(?) ORDER BY timestamp DESC",  (door, timeStart, timeEnd))
    got = c.fetchall()
    state = [i[2] for i in got]
    stateTime = [localtime_from_response(i[0]) for i in got]
    ret_dict = {"actions":{'mesage':message, 'action':actionType, 'time':actionTime}, "states":{'state':state, 'time':stateTime}}
    return ret_dict

############  Write data ########################
def write(col, payload):
    utcnow = datetime.datetime.utcnow()
    conn, c = get_db()
    c.execute("INSERT INTO boiler (timestamp, %s,) VALUES (?,?)" %(col), (utcnow, payload)
    conn.commit()
    return {'Status':'Success', 'Message': col + ' successfully updated with '+str(payload)}


setup_db()
