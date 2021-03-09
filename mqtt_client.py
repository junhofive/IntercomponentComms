import paho.mqtt.client as mqtt
import json

# global msg_count, sensor_count, sensor_readings, sensor_avg, chain_value
# global recv_msg_count, recv_sensor_count, recv_avg_counter, recv_sensor_avg

def calculate_checksum(payload):
    checksum = 0
    for key in payload.keys():
        if key != 'Checksum':
            checksum += sum(bytearray(key, encoding='utf8'))
            checksum += payload[key]
    return checksum


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Client successfully connected to the server!")
    else:
        print("Connection Failed!")


def on_message_ajay(client, userdata, message):
    global recv_msg_count, recv_sensor_count, recv_sensor_avg, recv_avg_counter
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']: # Valid Checksum
        for key in json_parsed_string.keys():
            if key == 'MessagesCount':
                recv_msg_count += json_parsed_string[key]
            elif key == 'ReadingsCount':
                recv_sensor_count += json_parsed_string[key]
            elif key == 'SensorAvg':
                recv_sensor_avg += json_parsed_string[key]
                recv_avg_counter += 1


def on_message_others(client, userdata, message):
    global msg_count, sensor_count, sensor_readings
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']: # Valid Checksum
        msg_count += 1
        for key in json_parsed_string.keys():
            if key == 'SensorReading':
                sensor_readings += json_parsed_string[key]
                sensor_count += 1
            # elif key == 'SensorCount':
            #     sensor_readings = json_parsed_string[key]


def on_message_chain1(client, userdata, message):
    global chain_value
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']:
        chain_value = json_parsed_string['Value'] + 1


def on_message_chain2(client, userdata, message):
    global chain_value
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']:
        chain_value = json_parsed_string['Value'] + 2


def on_message_chain3(client, userdata, message):
    global chain_value
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']:
        chain_value = json_parsed_string['Value'] + 3


def on_message_chain4(client, userdata, message):
    global chain_value
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']:
        if chain_value == json_parsed_string['Value']:
            print('\nCorrect Chain Value Received. Chain Validated!\n')
            print("Chain Value in Client: ", chain_value, " Chain value from Board: ", json_parsed_string['Value'])


def on_message_stop(client, userdata, message):
    global recv_msg_count, recv_sensor_count, recv_sensor_avg, recv_avg_counter
    global msg_count, sensor_count, sensor_readings
    msg = str(message.payload.decode("utf-8"))
    if msg == 'STOP':
        mqttc.disconnect()
        print("STOP command has been called.")
        print("\nTotal Messages Received in Client: ", msg_count, "Total Messages Received in TI Board: ", recv_msg_count)
        print("Total Readings Received in Client: ", sensor_count, "Total Readings Received in TI Board: ", recv_sensor_count)
        print("Average Sensor Data in Clinet: ", (sensor_readings/sensor_count), "Average Sensor Data Received: ", (recv_sensor_avg/recv_avg_counter))


host = 's1.airmqtt.com'
port = 10021
username = 'wg3z4rvo'
password = 'rzxe7rl2'

msg_count = 0
sensor_count = 0
sensor_readings = 0
sensor_avg = 0
chain_value = 0

recv_msg_count = 0
recv_sensor_count = 0
recv_avg_counter = 0
recv_sensor_avg = 0

# recv_msg_count, recv_sensor_count, recv_avg_counter, recv_sensor_avg
# Create Client Object
mqttc = mqtt.Client(client_id="testing_client")

print('Logging in with username and password')
mqttc.username_pw_set(username, password=password)

mqttc.on_connect = on_connect
# mqttc.on_message = on_message
mqttc.will_set("Client", payload="STOPPED", qos=0, retain=False)

# Connect to Server
print('Connecting to Broker')
mqttc.connect(host, port=port)

mqttc.message_callback_add('AjayBoard', on_message_ajay)
mqttc.message_callback_add('TerryBoard', on_message_others)
mqttc.message_callback_add('JasonBoard', on_message_others)
mqttc.message_callback_add('Chain1', on_message_chain1)
mqttc.message_callback_add('Chain2', on_message_chain2)
mqttc.message_callback_add('Chain3', on_message_chain3)
mqttc.message_callback_add('Chain4', on_message_chain4)
mqttc.message_callback_add('stop', on_message_stop)

mqttc.subscribe(('Chain1', 0))
mqttc.subscribe(('Chain2', 0))
mqttc.subscribe(('Chain3', 0))
mqttc.subscribe(('Chain4', 0))
mqttc.subscribe(('AjayBoard', 0))
mqttc.subscribe(('TerryBoard', 0))
mqttc.subscribe(('JasonBoard', 0))
mqttc.subscribe(('stop', 0))

mqttc.loop_forever()