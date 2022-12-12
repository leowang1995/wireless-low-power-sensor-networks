import time
from datetime import datetime
import os
from random import randint

nodes = {}

for i in range(1, 14):
    nodes[i] = f"03/09/2021, 01:3{randint(0,2)}:{randint(0,59)}\t\t| OK"

def printNodes():
    print("Node\t\t| Last Received (m/d/Y, H:M:S)\t| Message")
    print("--------------------------------------------------")
    for key in sorted(nodes.keys()):
        print(f"Node {key}\t\t| {nodes[key]}")


def on_message(client, userdata, message):
    msg = str(message.payload.decode("utf-8"))
    parts = msg.split("=")
    if len(parts) == 2:
        nodeNum = parts[1]
        current_time = datetime.now().strftime("%m/%d/%Y, %H:%M:%S")
        nodes[int(nodeNum)] = current_time

        os.system('cls')
        print(f"Last Messaged Received From {nodeNum} at {current_time}\n")
        printNodes()



current_time = datetime.now().strftime("%m/%d/%Y, %H:%M:%S")
nodes[4] = f"{current_time}\t\t| Damage detected"
printNodes()