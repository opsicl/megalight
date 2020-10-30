mosquitto_pub -h mqttserver.example.com -t 99/setconfig -m "`cat lights_config.json`" -r
