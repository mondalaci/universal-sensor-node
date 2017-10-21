#include <MCP9808.h>
MCP9808 mcp = MCP9808();
bool hasMcp9808 = false;

#include <BMP180.h>
BMP180 bMP180;
bool hasBmp180 = false;

const int USEC_IN_MIN = 60 * 1000;

String deviceName;

void getDeviceName(const char *topic, const char *data)
{
    deviceName = data;
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

    while (deviceName == "") {
        Particle.subscribe("particle/device/name", getDeviceName);
        Particle.publish("particle/device/name");
        delay(1 * USEC_IN_MIN);
    }
    
    Particle.unsubscribe();
}

void loop()
{
    if (hasMcp9808) {
    	Particle.publish(String(deviceName + "_mcp9808Temperature"), String(mcp.getTemperature()));
    }

    if (hasBmp180) {
        double temperature;
        bMP180.startTemperature();
        bMP180.getTemperature(temperature);
    	Particle.publish(String(deviceName + "_bmp180Temperature"), String(temperature));

        double absolutePressure;
        const int MAXIMUM_OVERSAMPLING = 3;
        const double CURRENT_ALTITUDE = 110; // meters
        bMP180.startPressure(MAXIMUM_OVERSAMPLING);
        bMP180.getPressure(absolutePressure, temperature);
        double sealevelPressure = bMP180.sealevel(absolutePressure, CURRENT_ALTITUDE);
    	Particle.publish(String(deviceName + "_bmp180Pressure"), String(sealevelPressure));
    }

	delay(5 * USEC_IN_MIN);
}
