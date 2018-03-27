# A Mongoose OS app

## Overview

This app uses a NodeMCU board (ESP8266) and uses its flash button (GPIO0) to
send messages to AWS IOT through MQTT.

## How to install this app

- Install [mos tool](https://mongoose-os.com/software.html)
- Generate a new device on AWS IoT
- Download AWS IoT certificates to `fs` folder
- Copy `fs/conf2.json` to `fs/conf3.json` and change your settings accordingly
- Run `bin/build && bin/deploy`
- If needed, change `.env`

**Important**: Your secrets will be stored on `fs/conf3.json`. Beware not to
leak them.

## License

BSD 3-Clause.
