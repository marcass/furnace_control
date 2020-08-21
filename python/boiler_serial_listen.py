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
    payload = str(msg.payload, 'UTF-8')
    print(msg.topic+' '+payload)
    if 'switch' in msg.topic:
        if ('Setpoint' in payload) or ('State'  in payload):
            port.write('\r\n'+payload+'\r')
            print ('Sent ' + payload + ' to serial port.')
        allowed_passthrough_msg = ['Turn Off Boiler', 'Turn On Boiler', 'Increase SetPoint', 'Decrease SetPoint']
        if payload in allowed_passthrough_msg:
            port.write('\r\n'+payload+'\r')
            print ('Sent ' + payload + ' to serial port.')
    if 'temp' in msg.topic:
        temp_type = msg.topic.split('/')[-1:][0]
        print ('temp type is: '+str(temp_type)+', value is: '+payload)
        #data.write_data(temp_type, 'temperature', int(msg.payload))
        post_data({'tags': {'state':boiler_data['State'], 'type':'temp', 'sensorID':temp_type, 'site': 'boiler'}, 'value':float(msg.payload), 'measurement': 'things'})
    if 'state' in msg.topic:
        try:
            state = msg.payload.decode(encoding='UTF8').replace('\r', '')
            #print("replaced stuff")
        except:
            state = payload
            #print("didn't replace stuff")
        #print ('state is blah '+state)
        # data.write_data('state', 'status', str(msg.payload))
        post_data({'tags': {'state':boiler_data['State'], 'type':'state', 'sensorID':'state', 'site': 'boiler'}, 'value':state, 'measurement': 'things'})
    if 'pid' in msg.topic:
        pid_type = msg.topic.split('/')[-1:][0]
        # data.write_data(pid_type, 'pid', int(msg.payload))
        post_data({'tags': {'state':boiler_data['State'], 'type':'pid', 'sensorID':pid_type, 'site': 'boiler'}, 'value':int(msg.payload), 'measurement': 'things'})
    if 'flame' in msg.topic:
        # data.write_data('burn', 'flame', int(msg.payload))
        post_data({'tags': {'state':boiler_data['State'], 'type':'light', 'sensorID':'flame', 'site': 'boiler'}, 'value':int(msg.payload), 'measurement': 'things'})


def write_setpoint(setpoint):
    port.write('\r\n'+'Setpoint'+'['+setpoint+']'+'\r')
    print ('Sent Setpoint[' + setpoint + '] to serial port.')

def write_state(state):
    # "Idle","Starting","Heating","Cool down","Error","Off"
    port.write('\r\n'+'State'+'['+state+']'+'\r')
    print ('Sent State[' + state + '] to serial port.')

def readlineCR(port):
    global boiler_data
    rv = ""
    while True:
        ch = port.read()
#        print(ch)
#        rv += ch
#       for python3 need to cast to str
        rv += str(ch, 'UTF-8')
#        rv += str(ch)
#        print(rv)
        if ch==b'\r':# or ch=='':
            #print(rv)
            if 'boiler' in rv:
                #received = rv
                received = rv[:-1]
                #print("rec = "+str(received))
                payload = received.split('^')
                topic = payload[0]
                value = payload[1]
                print ("topic, value")
                print (topic, value)
                publish.single(topic, value, retain=True)
                if (topic == 'boiler/messages'):
                    # print 'Got an error message'
                    alerts.send_alert('Gobgoyle says: '+value)
                try:
                    if topic in boiler_topics:
                        #if topic == 'boiler/state':
                        print('need to populate dict')
                        #    boiler_data[boiler_topics[topic]] = payload.replace('\r', '')
                        #else:
                        boiler_data[boiler_topics[topic]] = value
                        print(boiler_data)
                except:
                    # we don't care about this topic
                    print ('oops')
                    pass
            return rv

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
