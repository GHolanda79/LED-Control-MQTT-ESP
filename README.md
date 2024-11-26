| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# ESP-MQTT MQTT over Websocket

This example connects to a remote broker mqtt over web sockets as a demonstration subscribes/unsubscribes and send a message on certain topic.

It uses ESP-MQTT library which implements mqtt client to connect to mqtt broker.

## How to use example

### Hardware Required

This example can be executed on any ESP32 board, the required interface is WiFi and connection to internet, and a GPIO port to set a LED. One LED and a resistor to test.

### Server Required

An mqtt broker (for example mosquitto) is required. To do this, you can use a Linux machine (example arch).

Mosquitto installation in arch linux:

`` 
sudo pacman -Syu mosquitto
``

Enable service:

``
sudo systemctl enable mosquitto 
``

Start Service:

``
sudo systemctl start mosquitto
``

Configure Mosquitto. Edit the file:

``
/etc/mosquitto/mosquitto.conf
``

**Always restart after any changes:

``
sudo systemctl restart mosquitto
``

In the configuration file, uncomment and add the lines:

``
listener 9001
``

Verify listen ports:

``
netstat -tuln | grep 9001
``

Add rules to firewall iptables:

``
sudo iptables -A INPUT -p tcp --dport 9001 -j ACCEPT
``

Enable websockets:

``
protocol websockets
``

Enable anonymous users(only for tests):

``
allow_anonymous true
``

Permit logs:

``
log_type all
``

Log file path:

``
log_dest file /var/log/mosquitto.log
``

Create mosquitto log file:

``
sudo touch /var/log/mosquitto.log
``

Set mosquitto log file owner:

``
sudo chown mosquitto:mosquitto /var/log/mosquitto.log
``

To give mosquitto permission to write:

``
sudo chmod 640 /var/log/mosquitto.log
``

To monitor logs:

``
tail -f /var/log/mosquitto.log
``


For test: server(broker) -> MQTT_message(TOPIC=/led/state; DATA=(ON | OFF)) -> client(ESP32)

``
mosquitto_pub -h serve_ip -t "TOPIC" -m "DATA"
``


### Configure the project

* Open the project configuration menu (`idf.py menuconfig`)
* Configure Wi-Fi or Ethernet under "Example Connection Configuration" menu. 
Set SSID and password to connect to Wi-Fi and Broker URL or 
Set key CONFIG_EXAMPLE_WIFI_SSID="SSID"
Set key CONFIG_EXAMPLE_WIFI_PASSWORD="password"
Set key CONFIG_BROKER_URI="ws://server_ip:9001/"
in sdkconfig and save.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

```
I (5291) esp_netif_handlers: example_netif_sta ip: 192.168.1.105, mask: 255.255.255.0, gw: 192.168.1.1
I (5291) example_connect: Got IPv4 event: Interface "example_netif_sta" address: 192.168.30.105
I (5601) example_connect: Got IPv6 event: Interface "example_netif_sta" address: fe80:0000:0000:0000:e2e2:e6ff:fe62:f1e8, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (5601) example_common: Connected to example_netif_sta
I (5611) example_common: - IPv4 address: 192.168.1.105,
I (5611) example_common: - IPv6 address: fe80:0000:0000:0000:e2e2:e6ff:fe62:f1e8, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (5631) mqtt_event: Other event id:7
I (5631) led_event: Group Event s_led_state_event_group created
I (5641) gpio: GPIO[5]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (5641) led_event: LED configured in port GPIO5
I (5651) main_task: Returned from app_main()
I (6151) mqtt_event: MQTT_EVENT_CONNECTED
I (6161) mqtt_event: sent subscribe successful, msg_id=10209
I (6231) mqtt_event: MQTT_EVENT_SUBSCRIBED, msg_id=10209
I (52831) mqtt_event: MQTT_EVENT_DATA
TOPIC=/led/state
DATA=ON
I (52831) led_event: SET BITS
I (52831) led_event: SET LED_ON_BIT
I (52831) led_event: LED ON
I (55621) mqtt_event: MQTT_EVENT_DATA
TOPIC=/led/state
DATA=OFF
I (55621) led_event: SET BITS
I (55621) led_event: SET LED_OFF_BIT
I (55621) led_event: LED OFF
```

