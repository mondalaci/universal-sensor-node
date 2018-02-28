#define INFLUXDB_HOST   "myhostname"
#define INFLUXDB_PORT   8086
#define INFLUXDB_DB     "sensors"

#include "HttpClient/HttpClient.h"
HttpClient http;
http_header_t headers[] = {
    { NULL, NULL }
};

#include <ArduinoJson.h>
#define JSON_BUFFER_LENGTH 500
StaticJsonBuffer<JSON_BUFFER_LENGTH> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

#include <MCP9808.h>
MCP9808 mcp = MCP9808();
bool hasMcp9808 = false;

#include <BMP180.h>
BMP180 bMP180;
bool hasBmp180 = false;

#include <HTU21D.h>
HTU21D htu21d = HTU21D();
bool hasHtu21d = false;

const int USEC_IN_SEC = 1000;
const int USEC_IN_MIN = 60 * USEC_IN_SEC;

bool sendInflux(String payload) {
    http_request_t     request;
    http_response_t    response;

    request.hostname = INFLUXDB_HOST;
    request.port     = INFLUXDB_PORT;
    request.path     = "/write?db=" + String(INFLUXDB_DB);
    request.body     = payload;

    http.post(request, response, headers);

    return response.status == 204;
}

String zone;
String deviceName;
bool gotDeviceName = false;

void getDeviceName(const char *topic, const char *data)
{
    deviceName = data;
    int nodeSeparatorPos = deviceName.indexOf('_');
    if (nodeSeparatorPos != -1) {
        zone = deviceName.substring(nodeSeparatorPos + 1);
    }
    gotDeviceName = true;
}

void setup()
{
    RGB.brightness(1);

	if (mcp.begin()) {
    	mcp.setResolution(MCP9808_SLOWEST);
    	hasMcp9808 = true;
	}

    if (bMP180.begin()) {
        hasBmp180 = true;
    }

    if (htu21d.begin()) {
        hasHtu21d = true;
    }

    Particle.subscribe("particle/device/name", getDeviceName);
    Particle.publish("particle/device/name");

    while (deviceName == "") {
        delay(1 * USEC_IN_SEC);
    }

    Particle.unsubscribe();
}

void loop()
{
    bool hasProperty;
    String influxPayload = "temperature,zone=" + String(zone) + ",node=" + deviceName + " ";

    if (hasMcp9808) {
    	float temperature = mcp.getTemperature();
    	root["mcp9808Temperature"] = temperature;
    	influxPayload.concat(String(hasProperty ? "," : "") + "mcp9808Temperature=" + String(temperature) + "");
    	hasProperty = true;
    }

    if (hasBmp180) {
        bMP180.startTemperature();
        double temperature = bMP180.getTemperature(temperature);
    	root["bmp180Temperature"] = temperature;
    	influxPayload.concat(String(hasProperty ? "," : "") + "bmp180Temperature=\"" + String(temperature) + "\"");

        double absolutePressure;
        const int MAXIMUM_OVERSAMPLING = 3;
        const double CURRENT_ALTITUDE = 110; // meters
        bMP180.startPressure(MAXIMUM_OVERSAMPLING);
        bMP180.getPressure(absolutePressure, temperature);
        double sealevelPressure = bMP180.sealevel(absolutePressure, CURRENT_ALTITUDE);
    	root["bmp180Pressure"] = sealevelPressure;
    	influxPayload.concat(String(hasProperty ? "," : "") + "bmp180Pressure=\"" + String(sealevelPressure) + "\"");

    	hasProperty = true;
    }

    if (hasHtu21d) {
    	float temperature = htu21d.readTemperature();
    	root["htu21dTemperature"] = temperature;
    	influxPayload.concat(String(hasProperty ? "," : "") + "htu21dTemperature=\"" + String(temperature) + "\"");

    	float humidity = htu21d.readHumidity();
    	root["htu21dHumidity"] = humidity;
    	influxPayload.concat(String(hasProperty ? "," : "") + "htu21dHumidity=\"" + String(humidity) + "\"");

    	hasProperty = true;
    }

    char output[JSON_BUFFER_LENGTH];
    root.printTo(output, JSON_BUFFER_LENGTH);
    Particle.publish(deviceName, influxPayload);
    bool sendState = sendInflux(influxPayload);
    Particle.variable("influxPayloa", influxPayload);

	delay(5 * USEC_IN_MIN);
}
