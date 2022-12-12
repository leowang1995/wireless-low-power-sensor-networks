import serial
import paho.mqtt.client as mqtt
import time
import random

client = mqtt.Client("test_data")
client.username_pw_set("admin", "turnonthelights42")
client.connect("192.168.0.84", port=1883)


while True:
    rand_temp = "{0:.2f}".format(random.uniform(22.02, 22.1))
    client.publish("arpa/6/temperature", payload=rand_temp, qos=0, retain=False)
    print(f"Published: {rand_temp}")
    time.sleep(30)

