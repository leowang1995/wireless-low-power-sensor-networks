import paho.mqtt.client as mqtt
import time
from datetime import datetime
import os

nodes = {}

for i in range(1, 14):
    nodes[i] = None

def printNodes():
    print("Node\t\t| Last Received")
    print("-----------------------------")
    for key in sorted(nodes.keys()):
        print(f"Node {key}\t\t| {nodes[key]}")

printNodes()

def on_message(client, userdata, message):
    msg = str(message.payload.decode("utf-8"))
    parts = msg.split("=")
    if len(parts) == 2:
        nodeNum = parts[1]
        current_time = datetime.now().strftime("%H:%M:%S")
        nodes[int(nodeNum)] = current_time

        os.system('cls')
        print(f"Last Messaged Received From {nodeNum} at {current_time}\n")
        printNodes()

client = mqtt.Client("comp_watch")
client.username_pw_set("admin", "turnonthelights42")
client.on_message=on_message
client.connect("192.168.0.84", port=1883)

client.subscribe("arpa/nodetest")
client.loop_forever()
