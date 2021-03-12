from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QApplication, QMainWindow, QWidget, QLabel, QTableWidget, QTableWidgetItem, QVBoxLayout, QTabWidget, QLabel, QScrollArea, QLineEdit, QPushButton, QComboBox
from PyQt5 import QtCore
from PyQt5.QtCore import pyqtSlot, Qt, QObject, pyqtSignal
from PyQt5.QtGui import QFont
import sys

import paho.mqtt.client as mqtt
import json
import threading
import time

'''
# References:
PyQt table: https://pythonspot.com/pyqt5-table/    
PyQt Resize table: https://stackoverflow.com/questions/40995778/resize-column-width-to-fit-into-the-qtablewidget-pyqt
PyQt tabwidget: https://pythonbasics.org/pyqt-tabwidget/
PyQt QScrollArea: https://www.learnpyqt.com/tutorials/qscrollarea/
PyQt wake main thread from non-QThread: https://stackoverflow.com/questions/39870577/pyqt-wake-main-thread-from-non-qthread
PyQt Textbox Example: https://pythonspot.com/pyqt5-textbox-example/
PyQt Combobox Example: https://pythonbasics.org/pyqt-combobox/
PyQt change label font: https://www.geeksforgeeks.org/pyqt5-how-to-change-font-and-size-of-label-text/
'''

def calculate_checksum(payload):
    checksum = 0
    for key in payload.keys():
        if key != 'Checksum':
            checksum += sum(bytearray(key, encoding='utf8'))
            checksum += payload[key]
    return checksum

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        connector.emit("Client successfully connected to the server!")
        
    else:
        connector.emit("Connection Failed!")

# status msg for each board
def on_message_board_Terry(client, userdata, message):
    global PubAtmpt_T, PubSucs_T, PubRecv_T, Missing_T
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']: # Valid Checksum
        for key in json_parsed_string.keys():
            if key == 'PubAtmpt':
                PubAtmpt_T = json_parsed_string[key]
            elif key == 'PubSucs':
                PubSucs_T = json_parsed_string[key]
            elif key == 'PubRecv':
                PubRecv_T = json_parsed_string[key]
            elif key == 'Missing':
                Missing_T = json_parsed_string[key]
    connector.emit("TerryBoard Value Needs Update")

def on_message_board_Jason(client, userdata, message):
    global PubAtmpt_J, PubSucs_J, PubRecv_J, Missing_J
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']: # Valid Checksum
        for key in json_parsed_string.keys():
            if key == 'PubAtmpt':
                PubAtmpt_J = json_parsed_string[key]
            elif key == 'PubSucs':
                PubSucs_J = json_parsed_string[key]
            elif key == 'PubRecv':
                PubRecv_J = json_parsed_string[key]
            elif key == 'Missing':
                Missing_J = json_parsed_string[key]
    connector.emit("JasonBoard Value Needs Update")

def on_message_board_Ajay(client, userdata, message):
    global PubAtmpt_A, PubSucs_A, PubRecv_A, Missing_A
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']: # Valid Checksum
        for key in json_parsed_string.keys():
            if key == 'PubAtmpt':
                PubAtmpt_A = json_parsed_string[key]
            elif key == 'PubSucs':
                PubSucs_A = json_parsed_string[key]
            elif key == 'PubRecv':
                PubRecv_A = json_parsed_string[key]
            elif key == 'Missing':
                Missing_A = json_parsed_string[key]
    connector.emit("AjayBoard Value Needs Update")


# Chain
def on_message_chain1(client, userdata, message):
    global chain_value_client
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']:
        chain_value_client = json_parsed_string['Value'] + 1

def on_message_chain2(client, userdata, message):
    global chain_value_client
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']:
        chain_value_client = json_parsed_string['Value'] + 2

def on_message_chain3(client, userdata, message):
    global chain_value_client
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']:
        chain_value_client = json_parsed_string['Value'] + 3

def on_message_chain4(client, userdata, message):
    global chain_value_board, chain_value_client
    msg = str(message.payload.decode("utf-8"))
    json_parsed_string = json.loads(msg)
    checksum = calculate_checksum(json_parsed_string)
    if checksum == json_parsed_string['Checksum']:
        if chain_value_client == json_parsed_string['Value']:
            connector.emit("Correct Chain Value Received. Chain Validated!")
        else:
            connector.emit("Incorrect Chain Value! ")
        chain_value_board = json_parsed_string['Value']
        connector.emit("Chain Value Needs Update")


def on_message_stop(client, userdata, message):
    msg = str(message.payload.decode("utf-8"))
    if msg == 'STOP':
        mqttc.disconnect()
        connector.emit("STOP command has been called.")

def on_disconnect(client, userdata, message):
    client.loop_stop()
#_____________________________________________________________________________________________________________________________________________________________
# QT Class
class App_Dashboard(QWidget): # inherit from QMainWindow
    dataReceived = QtCore.pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self.initUI() 
                              
    def initUI(self):
        self.setGeometry(300, 300, 1150, 370)
        self.setWindowTitle("Dashboard - Team 7")
        self.tabWidget = QTabWidget(self)
        
        # Scroll Area
        self.scroll = QScrollArea(self)
        self.scroll.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        self.scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        self.scroll.setWidgetResizable(True)
        self.vbox = QVBoxLayout()
        self.widget = QWidget()

        # Value Table
        self.tableWidget = QTableWidget(self)
        self.tableWidget.setRowCount(8)
        self.tableWidget.setColumnCount(4)
        self.tableWidget.setItem(1,0, QTableWidgetItem("# Attempting to publish"))
        self.tableWidget.setItem(2,0, QTableWidgetItem("# Successfully published"))
        self.tableWidget.setItem(3,0, QTableWidgetItem("# Publications received"))
        self.tableWidget.setItem(4,0, QTableWidgetItem("# Missing"))
        self.tableWidget.setItem(0,1, QTableWidgetItem("Jason's Board"))
        self.tableWidget.setItem(0,2, QTableWidgetItem("Terry's Board"))
        self.tableWidget.setItem(0,3, QTableWidgetItem("Ajay's Board"))
        self.tableWidget.setItem(6,1, QTableWidgetItem("Client"))
        self.tableWidget.setItem(6,2, QTableWidgetItem("Board"))
        self.tableWidget.setItem(7,0, QTableWidgetItem("Chain Value"))
        self.tableWidget.setSizeAdjustPolicy(QtWidgets.QAbstractScrollArea.AdjustToContents)
        self.tabWidget.addTab(self.tableWidget, "Results")

        # Chain Value Publish Box (Text box with a button)
        self.label1 = QLabel("Publish to Chain",self)
        self.label1.setFont(QFont('Times font', 12)) 
        self.label1.resize(300,100)
        self.label1.move(575,0)
        self.label2 = QLabel("Input your value:",self)
        self.label2.setFont(QFont('Arial', 8)) 
        self.label2.resize(100,60)
        self.label2.move(575,55)
        self.textbox_chain = QLineEdit(self)
        self.textbox_chain.move(680, 70)
        self.textbox_chain.resize(280,30)
        # Create a button in the window
        self.button1 = QPushButton('Publish', self)
        self.button1.resize(100,30)
        self.button1.move(980,70)
        self.button1.clicked.connect(self.click_publish_chain)


        # Error Checking
        x = 20
        self.board_select = 1  # 0  for Terry's board, 1 for Jason's board, default = 1
        self.label3 = QLabel("Error Checking",self)
        self.label3.setFont(QFont('Times font', 12)) 
        self.label3.resize(300,100)
        self.label3.move(575,110)
        self.label4 = QLabel("Select Board",self)
        self.label4.resize(100,100)
        self.label4.move(575,125+x)
        self.label5 = QLabel("Sequence #",self)
        self.label5.resize(100,60)
        self.label5.move(575,185+x)
        self.textbox_seq = QLineEdit(self)
        self.textbox_seq.move(680, 200+x)
        self.textbox_seq.resize(280,30)
        # Create a button in the window
        self.button2 = QPushButton('Publish', self)
        self.button2.resize(100,30)
        self.button2.move(980,200+x)
        self.button2.clicked.connect(self.click_publish_seq)
        # Selection box - can either be jason board or terry board
        self.selectionbox = QComboBox(self)
        self.selectionbox.addItem("JasonBoard")
        self.selectionbox.addItem("TerryBoard")
        self.selectionbox.resize(120,30)
        self.selectionbox.move(680, 160+x)
        self.selectionbox.currentIndexChanged.connect(self.selectionchange)


        # Start + Stop Buttons
        self.label3 = QLabel("Demo",self)
        self.label3.setFont(QFont('Times font', 12)) 
        self.label3.resize(300,100)
        self.label3.move(575,240)
        self.button_start = QPushButton('START', self)
        self.button_start.resize(100,40)
        self.button_start.move(680,300)
        self.button_start.clicked.connect(self.click_start)
        self.button_stop = QPushButton('STOP', self)
        self.button_stop.resize(100,40)
        self.button_stop.move(860,300)
        self.button_stop.clicked.connect(self.click_stop)

        # Signal
        self.dataReceived.connect(self.receive)

    @pyqtSlot()
    def click_publish_chain(self):
        value = self.textbox_chain.text() # take the value input in the textbox
        # create the chain JSON message:
        payload= {"Value": int(value), "ChainCount": 1, "Checksum": 1}
        checksum = calculate_checksum(payload)
        payload["Checksum"] = checksum
        payload_str = json.dumps(payload)
        mqttc.publish("Chain1", payload_str) 

    @pyqtSlot()
    def click_publish_seq(self):
        value = self.textbox_seq.text() # take the value input in the textbox
        # mqtt.publish(value)
        print("you clicked seq! Value = ", value)
        if self.board_select == 0:
            print("you're publishing to Terry's board")
        if self.board_select == 1:
            print("you're publishing to Jason's board")


    @pyqtSlot()
    def click_start(self):
        mqttc.publish("start","START") 

    @pyqtSlot()
    def click_stop(self):
        mqttc.publish("stop","STOP")


    def selectionchange(self):
        if self.selectionbox.currentText() == "JasonBoard":
            self.board_select = 1
        if self.selectionbox.currentText() == "TerryBoard":
            self.board_select = 0

        print("Right now, you're selecting: ", self.selectionbox.currentText())


    def receive(self, data):
        #print("data = ", data)
        if data == "TerryBoard Value Needs Update":
            self.updateTableForTerry()
        if data == "JasonBoard Value Needs Update":
            self.updateTableForJason()
        if data == "AjayBoard Value Needs Update":
            self.updateTableForAjay()
        if data == "Chain Value Needs Update":
            self.updateTableForChain()
        else:   # display message in the scroll windoww
            self.display_message(data)

    def callback(self, data):
        #print("QT callback")
        self.dataReceived.emit(data)

    def display_message(self, msg):
        # store the msg in a label
        label = QLabel(msg)
        self.vbox.addWidget(label)

        # delete previous widget, create a new one (refresh)
        self.widget.deleteLater()
        self.widget = QWidget()
        self.widget.setLayout(self.vbox)

        # delete previous scroll window, create a new one
        self.scroll.deleteLater()
        self.scroll = QScrollArea(self)
        self.scroll.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        self.scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        self.scroll.setWidgetResizable(True)
        self.scroll.setWidget(self.widget)

        # add to the "messages" tab
        self.tabWidget.addTab(self.scroll, "Messages")

        self.show()

    def updateTableForTerry(self):        
        self.tableWidget.setItem(1,2, QTableWidgetItem(str(PubAtmpt_T)))
        self.tableWidget.setItem(2,2, QTableWidgetItem(str(PubSucs_T)))
        self.tableWidget.setItem(3,2, QTableWidgetItem(str(PubRecv_T)))
        self.tableWidget.setItem(4,2, QTableWidgetItem(str(Missing_T)))
        self.tableWidget.resizeColumnsToContents()
        # add to tab
        self.tabWidget.addTab(self.tableWidget, "Results")
        self.show()

    def updateTableForJason(self):        
        self.tableWidget.setItem(1,1, QTableWidgetItem(str(PubAtmpt_J)))
        self.tableWidget.setItem(2,1, QTableWidgetItem(str(PubSucs_J)))
        self.tableWidget.setItem(3,1, QTableWidgetItem(str(PubRecv_J)))
        self.tableWidget.setItem(4,1, QTableWidgetItem(str(Missing_J)))
        self.tableWidget.resizeColumnsToContents()
        # add to tab
        self.tabWidget.addTab(self.tableWidget, "Results")
        self.show()

    def updateTableForAjay(self):        
        self.tableWidget.setItem(1,3, QTableWidgetItem(str(PubAtmpt_A)))
        self.tableWidget.setItem(2,3, QTableWidgetItem(str(PubSucs_A)))
        self.tableWidget.setItem(3,3, QTableWidgetItem(str(PubRecv_A)))
        self.tableWidget.setItem(4,3, QTableWidgetItem(str(Missing_A)))
        self.tableWidget.resizeColumnsToContents()
        # add to tab
        self.tabWidget.addTab(self.tableWidget, "Results")
        self.show()

    def updateTableForChain(self):
        self.tableWidget.setItem(7,1, QTableWidgetItem(str(chain_value_client)))
        self.tableWidget.setItem(7,2, QTableWidgetItem(str(chain_value_board)))
        self.tableWidget.resizeColumnsToContents()
        # add to tab
        self.tabWidget.addTab(self.tableWidget, "Results")
        self.show()


# call the QT callback function (useful when I need to update msg in a different thread)
class Connector(object):
    def __init__(self, callback):
        self._callback = callback

    def emit(self, msg):
        self._callback(msg)

app = QApplication(sys.argv)
dashboard = App_Dashboard()
connector = Connector(dashboard.callback)
#_____________________________________________________________________________
host = 's1.airmqtt.com'
port = 10021
username = 'h721gyh3'
password = '1tf7h451'

'''
msg_count = 0
sensor_count = 0
sensor_readings = 0
sensor_avg = 0
chain_value = 0

recv_msg_count = 0
recv_sensor_count = 0
recv_avg_counter = 0
recv_sensor_avg = 0
'''
# Terry
PubAtmpt_T = 0
PubSucs_T = 0
PubRecv_T = 0
Missing_T = 0

# Jason
PubAtmpt_J = 0
PubSucs_J = 0
PubRecv_J = 0
Missing_J = 0

# Ajay
PubAtmpt_A = 0
PubSucs_A = 0
PubRecv_A = 0
Missing_A = 0

# Chain
chain_value_board = 0
chain_value_client = 0

# Create Client Object
mqttc = mqtt.Client(client_id="testing_client")

dashboard.display_message('Logging in with username and password...')
mqttc.username_pw_set(username, password=password)

mqttc.on_connect = on_connect
mqttc.on_disconnect = on_disconnect
mqttc.will_set("Client", payload="STOPPED", qos=0, retain=False)

# Connect to Server
dashboard.display_message("Connecting to Broker...")
mqttc.connect(host, port=port)

'''
mqttc.message_callback_add('AjayBoard', on_message_ajay)
mqttc.message_callback_add('TerryBoard', on_message_others)
mqttc.message_callback_add('JasonBoard', on_message_others)
'''
mqttc.message_callback_add('AjayStatus', on_message_board_Ajay)
mqttc.message_callback_add('TerryStatus', on_message_board_Terry)
mqttc.message_callback_add('JasonStatus', on_message_board_Jason)

mqttc.message_callback_add('Chain1', on_message_chain1)
mqttc.message_callback_add('Chain2', on_message_chain2)
mqttc.message_callback_add('Chain3', on_message_chain3)
mqttc.message_callback_add('Chain4', on_message_chain4)
mqttc.message_callback_add('stop', on_message_stop)

mqttc.subscribe(('Chain1', 0))
mqttc.subscribe(('Chain2', 0))
mqttc.subscribe(('Chain3', 0))
mqttc.subscribe(('Chain4', 0))
'''
mqttc.subscribe(('AjayBoard', 0))
mqttc.subscribe(('TerryBoard', 0))
mqttc.subscribe(('JasonBoard', 0))
'''
mqttc.subscribe(('AjayStatus', 0))
mqttc.subscribe(('JasonStatus', 0))
mqttc.subscribe(('TerryStatus', 0))

mqttc.subscribe(('stop', 0))

dashboard.display_message("Subsribe to all topics success! ")
mqttc.loop_start()
sys.exit(app.exec_())  
#mqttc.loop_forever()