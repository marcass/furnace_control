import serial
import time
# import boiler_alerts as alerts
import creds
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import requests
import creds
import json
from threading import Thread

AUTH_URL = 'https://skibo.duckdns.org/api/auth/login'
DATA_URL = 'https://skibo.duckdns.org/api/data'
headers = ''
jwt = ''

# dict of topics for messaging
boiler_topics = {"boiler/state":"State", "boiler/temp/water":"Water temp", "boiler/temp/auger": "Auger temp", "boiler/temp/setpoint": "Setpoint"}
boiler_data = {'State': '', 'Water temp': 0, 'Auger temp': 0, 'Setpoint': 0}
state_arr = ['Idle', 'Starting', 'Heating', 'Cooling', 'Error', 'Off']

def getToken():
    global jwt
    global headers
    r = requests.post(AUTH_URL, json = {'username': creds.user, 'password': creds.password})
    tokens = r.json()
    #print 'token data is: ' +str(tokens)
    try:
        jwt = tokens['access_token']
        headers = {"Authorization":"Bearer %s" %jwt}
    except:
        print ('oops, no token for you')

def post_data(data):
    global jwt
    global headers
    if (jwt == ''):
        print ('Getting token')
        getToken()
    ret = requests.post(DATA_URL, json = data, headers = headers)
    #print 'JWT = '+str(jwt)
    #print 'First response is: ' +str(ret)
    if '200' not in str(ret):
        print ('Oops, not authenticated')
        try:
            getToken()
            requests.post(DATA_URL, json = data, headers = headersi)
            ret = {'Status': 'Error', 'Message': 'Got token'}
            print ('Post NOT 200 response is: ' +str(r))
        except:
            ret =  {'Status': 'Error', 'Message': 'Failed ot get token, so cannot perform request'}
    #print ret
    #return ret.json()

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("boiler/#")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    message = str(msg.payload, 'UTF-8')
    if 'switch' in msg.topic:
        #print ("port write request received")
        #print(message)
        if ('Setpoint' in message) or ('State'  in message):
            port.write('\r\n'+message+'\r')
            print ('Sent ' + message + ' to serial port.')
    allowed_passthrough_msg = ['Turn Off Boiler', 'Turn On Boiler', 'Increase SetPoint', 'Decrease SetPoint']
    if message in allowed_passthrough_msg:
        #print ("Message allowed through")
        to_send = ('\r\n'+message+'\r\n').encode()
        port.write(to_send)
        #port.write(('\r\n'+message+'\n\r').encode())
        #port.write('\r\n')
        #port.write(message)
        #port.write('\r')
        #print ('Sent ' + message + ' to serial port.')

def parse_packet(payload, sensor):
    #payload = str(payload, 'UTF-8')
    #print(sensor, payload)
    try:
        if 'temp' in sensor:
            temp_type = sensor.split('/')[-1:][0]
            #print ('temp type is: '+str(temp_type)+', value is: '+payload)
            #data.write_data(temp_type, 'temperature', int(payload))
            packet = {'tags': {'state':boiler_data['State'], 'type':'temp', 'sensorID':temp_type, 'site': 'boiler'}, 'value':float(payload), 'measurement': 'things'}
        if 'state' in sensor:
            state = int(payload)
            # Publish string value to state
            readable_state = state_arr[state]
            boiler_data['State'] = readable_state
            client.publish('boiler/state/readable/', readable_state, 0, True)
            packet = {'tags': {'state':boiler_data['State'], 'type':'state', 'sensorID':'state', 'site': 'boiler'}, 'value':state, 'measurement': 'things'}
        if 'pid' in sensor:
            pid_type = sensor.split('/')[-1:][0]
            pids = ['fan', 'pause', 'feed']
            if pid_type in pids:
                # data.write_data(pid_type, 'pid', int(payload))
                packet = {'tags': {'state':boiler_data['State'], 'type':'pid', 'sensorID':pid_type, 'site': 'boiler'}, 'value':int(payload), 'measurement': 'things'}
            else:
                print('Mangled PID typ')
        if 'flame' in sensor:
            # data.write_data('burn', 'flame', int(payload))
            packet = {'tags': {'state':boiler_data['State'], 'type':'light', 'sensorID':'flame', 'site': 'boiler'}, 'value':int(payload), 'measurement': 'things'}
        #print(packet)
        return packet
    except:
        packet = 0
        return packet


def write_setpoint(setpoint):
    port.write('\r\n'+'Setpoint'+'['+setpoint+']'+'\r')
    print ('Sent Setpoint[' + setpoint + '] to serial port.')

def write_state(state):
    # "Idle","Starting","Heating","Cool down","Error","Off"
    port.write('\r\n'+'State'+'['+state+']'+'\r')
    print ('Sent State[' + state + '] to serial port.')

def readlineCR(port):
    global boiler_data
    global client
    rv = ""
    while True:
        ch = port.read()
#        print(ch)
#        rv += ch
#       for python3 need to cast to str
        try:
            rv += str(ch, 'UTF-8')
        except:
            print('Serial character decoding error')
            pass
#        rv += str(ch)
#        print(rv)
        if ch==b'\r':# or ch=='':
            print(rv)
            try:
                if 'boiler' in rv:
                    #received = rv
                    received = rv[:-1]
                    #print("rec = "+str(received))
                    payload = received.split('^')
                    #sensor = payload[0][1:]
                    sensor = payload[0]
                    #print(payload, sensor)
                    #sensor_arr = sensor.split('/')
                    value = payload[1]
                    try:
                        packet = parse_packet(value, sensor)
                        client.publish('hcc/boiler/', str(packet), 0, True)
                    except:
                        print('Could not parse packet, skipping')
                        pass
            except:
                print('Serial garbage. Discarding')
                pass
            return rv
        else:
            #do nothing
            dumb_var = 1
            #print('keep building')


# Alerts setup:
# def chat_messg(msg):
#     if '/turn off' in msg['text']:
#         print ('turning off')
#         port.write('\r\n'+'Turn Off Boiler'+'\r')
#         return
#     if '/turn on' in msg['text']:
#         print ('turning on')
#         port.write('\r\n'+'Turn On Boiler'+'\r')
#         return
#     else:
#         print ('doing other stuff')
#         alerts.on_chat_message(msg, boiler_data)
#         return


def duckpunch():
    print ('Starting mqtt client')
    global mqtt_running
    global client
    # start mqtt client
    client.loop_start()
    mqtt_running = True
    # Start message bot
    # alerts.MessageLoop(alerts.bot, {'chat': chat_messg, 'callback_query': alerts.on_callback_query}).run_as_thread()

def start_listeners():
    global client
    # client.username_pw_set(username='user', password='pass')
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(creds.broker, 1883, 60)
    try:
        duckpunch()
    except:
        mqtt_running = False

auth = creds.mosq_auth
port = serial.Serial("/dev/arduino", baudrate=115200, timeout=3.0)
#port = serial.Serial("/dev/ttyUSB0", baudrate=9600, timeout=3.0)

if __name__ == "__main__":
    global client
    global mqtt_running
    mqtt_running = False
    client = mqtt.Client()
    start_listeners()
    while True:
        if not mqtt_running:
            try:
                # recconnect
                duckpunch()
                print ('Restarting mqtt client')
            except:
                client.loop_stop()
                mqtt_running = False
        #for debugging enable printing of serial port data
        rcv = readlineCR(port)
        #print rcv
