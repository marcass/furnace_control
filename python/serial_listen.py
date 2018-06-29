import serial
import time
import alerts
import creds
#import paho.mqtt.client as mqtt
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish

# dict of topics for messaging
boiler_topics = {"boiler/state":"State: ", "boiler/temp/water":"Water temp", "boiler/temp/auger": "Auger temp", "boiler/temp/setpoint": "Setpoint"}
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
    allowed_passthrough_msg = ['Turn Off Boiler', 'Turn On Boiler', 'Increase SetPoint', 'Decrease SetPoint']
    if str(msg.payload) in allowed_passthrough_msg:
    	port.write('\r\n'+str(msg.payload)+'\r')
        print 'Sent ' + msg.payload + ' to serial port.'

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
                    alerts.send_alert(payload)
                try:
                    if topic in boiler_topics:
                        boiler_data[boiler_topics[topic]] = payload
                except:
                    # we don't care about this topic
                    pass
            return rv

# Alerts setup:
def chat_messg():
    alerts.on_chat_message(msg, boiler_data)


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
