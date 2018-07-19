import serial
import time
import alerts
import creds
#import paho.mqtt.client as mqtt
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish

# dict of topics for messaging
boiler_topics = {"boiler/state":"State", "boiler/temp/water":"Water temp", "boiler/temp/auger": "Auger temp", "boiler/temp/setpoint": "Setpoint"}
boiler_data = {'State': '', 'Water temp': 0, 'Auger temp': 0, 'Setpoint': 0}


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("boiler/switch")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.topic+' '+str(msg.payload))
    if ('Setpoint' in str(msg.payload)) or ('State'  in str(msg.payload)):
    	port.write('\r\n'+str(msg.payload)+'\r')
        print 'Sent ' + msg.payload + ' to serial port.'
    allowed_passthrough_msg = ['Turn Off Boiler', 'Turn On Boiler', 'Increase SetPoint', 'Decrease SetPoint']
    if str(msg.payload) in allowed_passthrough_msg:
    	port.write('\r\n'+str(msg.payload)+'\r')
        print 'Sent ' + msg.payload + ' to serial port.'

def write_setpoint(setpoint):
	port.write('\r\n'+'Setpoint'+'['+setpoint+']'+'\r')
    print 'Sent Setpoint[' + setpoint + '] to serial port.'

def write_state(state):
    # "Idle","Starting","Heating","Cool down","Error","Off"
	port.write('\r\n'+'State'+'['+state+']'+'\r')
    print 'Sent State[' + state + '] to serial port.'

def readlineCR(port):
    global boiler_data
    rv = ""
    while True:
        ch = port.read()
        rv += ch
        if ch=='\r':# or ch=='':
            if 'MQTT' in rv:
                print rv
                received = rv[6:]
                received_splited = received.split('/')
                topic = '/'.join(received_splited[:-1])
                payload = received_splited[-1]
                print topic, payload
                publish.single(topic, payload, auth=auth, hostname=creds.broker, retain=True)
                if (topic == 'boiler/messages'):
                    # print 'Got an error message'
                    alerts.send_alert('Gobgoyle says: '+payload)
                try:
                    if topic in boiler_topics:
                        boiler_data[boiler_topics[topic]] = payload
                except:
                    # we don't care about this topic
                    # print 'oops'
                    pass
            return rv

# Alerts setup:
def chat_messg(msg):
    if '/turn off' in msg['text']:
        print 'turning off'
        port.write('\r\n'+'Turn Off Boiler'+'\r')
        return
    if '/turn on' in msg['text']:
        print 'turning on'
        port.write('\r\n'+'Turn On Boiler'+'\r')
        return
    else:
        print 'doing other stuff'
        alerts.on_chat_message(msg, boiler_data)
        return


auth = creds.mosq_auth
port = serial.Serial("/dev/arduino", baudrate=9600, timeout=3.0)
#port = serial.Serial("/dev/ttyUSB0", baudrate=9600, timeout=3.0)

if __name__ == "__main__":
    client = mqtt.Client()
    client.username_pw_set(username='esp', password='heating')

    #mqtt.userdata_set(username='esp',password='heating')
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(creds.broker, 1883, 60)
    # Start message bot
    alerts.MessageLoop(alerts.bot, {'chat': chat_messg, 'callback_query': alerts.on_callback_query}).run_as_thread()
    client.loop_start()
    while True:
        #for debugging enable printing of serial port data
        rcv = readlineCR(port)
        #print rcv
