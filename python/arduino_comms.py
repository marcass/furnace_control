import serial
import time
#import paho.mqtt.client as mqtt
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish

ser = serial

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("boiler/switch")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.topic+' '+str(msg.payload))
    allowed_passthrough_msg = ['Turn Off Boiler', 'Turn On Boiler']
    if str(msg.payload) in allowed_passthrough_msg:
	port.write('\r\n'+str(msg.payload)+'\r')
        print 'Sent ' + msg.payload + ' to serial port.'



def readlineCR(port):
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
                publish.single(topic, payload, auth=auth, hostname="houseslave")
            return rv


auth = {'username':"esp", 'password':"heating"}

if __name__ == "__main__":
    #keep checking for port with a timeout of 5 seconds
    # need to modify to a threading thingy? https://docs.python.org/3/library/threading.html#event-objects
    #if an event object can run a thread that checks for serial port /dev/arduino, set() internal
    # flag to true such that subscription stuff can start, or clear() when port not available
    # so it can keep checking
    try:
        ser = serial.Serial("/dev/ttyUSB0", baudrate=9600, timeout=5.0)        
        #ser = serial.Serial("/dev/arduino", baudrate=9600, timeout=5.0) 
        #we have a serial port if it can be read
        while ser.read():
            client = mqtt.Client()
            client.username_pw_set(username='esp', password='heating')
        
            #mqtt.userdata_set(username='esp',password='heating')
            client.on_connect = on_connect
            client.on_message = on_message
            client.connect("houseslave", 1883, 60)
        
            # Blocking call that processes network traffic, dispatches callbacks and
            # handles reconnecting.
            # Other loop*() functions are available that give a threaded interface and a
            # manual interface.
            # client.loop_forever()
            client.loop_start()
            while True:
                #for debugging enable printing of serial port data
                rcv = readlineCR(ser)
                #print rcv
        
        #no serial port available
        print 'serial closed'
        ser.close()

    #exception handling
    except serial.serialutil.SerialException:
        print 'exception'
