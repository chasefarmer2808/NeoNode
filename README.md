# Neonode

This is the firmware code for devices that want to communicate with the Neohub system.  These devices must be Arduino-compatible and WiFi enabled, such as the Adafruit Feather HUZZAH32.

## Hardware Requirements

The Neonode device must communicate with Adafruit Neopixels.  There is no support for DOT Star at this time.  Connect and power your Neopixel circuit following Adafruit's guide.

## Software Requirements

The main program for this application is located in `arduino/neonode/neonode.ino`.  It depends on the following external libraries to compile successfully:

* WiFi
* ESPAsyncWebServer
* AsyncJson
* Adafruit Neopixel
* ArduinoJson

It also reads global vales from a file called `env.h`.  These values are considered secret and should not be committed to source control.  Refactor the `arduino/neonode/env_template.h` file to create `arduino/neonode/env.h`, and populate the values within with your own information.  Finally, update constants in `neonode.ino` to match your setup.

At this point, you should be able to compile and upload the application to your Neonode device.  Verify the server is running by executing an HTTP GET request against `http://<your neonode ip>/neopixel/info`.  It should return a JSON response containing information about your Neonode.