
import paho.mqtt.client as mqtt
import time
import random

client = mqtt.Client("test_data_7")
client.username_pw_set("sensor-node", "SensorNode$")
client.connect("sensor-node.hatasaka.com", port=4000)


while True:
    rand_client = random.randint(2, 12);
    # rand_temp = "{0:.2f}".format(random.uniform(21.97, 22.09))
    client.publish(f"arpa/gas/{rand_client}", payload="1", qos=0, retain=False)
    # print(f"Published: {rand_temp}")
    print(f"Published: 1 for client {rand_client}")
    time.sleep(10)

