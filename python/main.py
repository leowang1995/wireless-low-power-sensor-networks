import serial
import paho.mqtt.client as mqtt

client = mqtt.Client("comp")
client.username_pw_set("admin", "turnonthelights42")
client.connect("192.168.0.84", port=1883)

data = ""
with serial.Serial(port="COM8", baudrate=9600, timeout=1) as ser:
    while True:
        data += (ser.read(16)).decode("utf-8");

        current = data.split(";;", 1)

        if len(current) > 1:
            data = current[1]
            print(current[0])
            
            client.publish("arpa/nodetest", payload=current[0], qos=0, retain=False)

