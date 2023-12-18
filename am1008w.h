// put this file in your esphome folder
// protocol implemented as described in https://en.gassensor.com.cn/Product_files/Specifications/CM1106-C%20Single%20Beam%20NDIR%20CO2%20Sensor%20Module%20Specification.pdf

#include "esphome.h"

class AM1008W : public UARTDevice {
  public:
    int16_t co2;		// 0 - 5000	: 0 - 5000 ppm
    int16_t voc;		// 0 - 3	: 0 - 3 level
    float   humidity;		// 50 - 990	: 5 - 99.0 %
    float   temperature;	// 300 - 1200	: -20 - 70 °C
    int16_t pm1_0_grimm;	// 0 - 1000	: 0 - 1000 ug/m3
    int16_t pm2_5_grimm;	// 0 - 1000	: 0 - 1000 ug/m3
    int16_t pm10_grimm;		// 0 - 1000	: 0 - 1000 ug/m3
    int16_t pm1_0_tsi;		// 0 - 1000	: 0 - 1000 ug/m3
    int16_t pm2_5_tsi;		// 0 - 1000	: 0 - 1000 ug/m3
    int16_t pm10_tsi;		// 0 - 1000	: 0 - 1000 ug/m3
    uint8_t state;		// bit 0: Fan at hHigh revolving speed
				// bit 1: Fan at lLow revolving speed
				// bit 2: Working temperature is high
				// bit 3: Working temperature is low
				// bit 4-7: Reserved
    AM1008W(UARTComponent *parent) : UARTDevice(parent) {}

    void setCo2CalibValue(uint16_t co2 = 400) {
        uint8_t cmd[6];
        memcpy(cmd, AM1008W_CMD_SET_CO2_CALIB, sizeof(cmd));
        cmd[3] = co2 >> 8;
        cmd[4] = co2 & 0xFF;
        uint8_t response[4] = {0};
        bool success = sendUartCommand(cmd, sizeof(cmd), response, sizeof(response));

        if(!success) {
            ESP_LOGW(TAG, "Reading data from AM1008W failed!");
            return;
        }

        // check if correct response received
        if(memcmp(response, AM1008W_CMD_SET_CO2_CALIB_RESPONSE, sizeof(response)) != 0) {
            ESP_LOGW(TAG, "Got wrong UART response: %02X %02X %02X %02X", response[0], response[1], response[2], response[3]);
            return;
        }

        ESP_LOGD(TAG, "AM1008W Successfully calibrated sensor to %uco2", co2);

    }

    uint8_t getResult() {
        uint8_t response[25] = {0}; // response: 0x16, 0x16(22), 0x01, DF1, DF2, DF3, DF4, DF5, DF6, DF7, DF8, DF9, DF10, DF11, DF12, DF13, DF14, DF15, DF16, DF17, DF18, DF19, DF20, DF21, CRC.
        bool success = sendUartCommand(AM1008W_CMD_GET_RESULT, sizeof(AM1008W_CMD_GET_RESULT), response, sizeof(response));
        
        if(!success) {
            ESP_LOGW(TAG, "Reading data from AM1008W failed!");
            return -1;
        }

        if(!(response[0] == 0x16 && response[1] == 0x16 && response[2] == 0x01)) {
            ESP_LOGW(TAG, "Got wrong UART response: %02X %02X %02X %02X...", response[0], response[1], response[2], response[3]);
            return -1;
        }

        uint8_t response_len = sizeof(response);
        uint8_t checksum = calcCRC(response, response_len);
        if(response[(response_len-1)] != checksum) {
            ESP_LOGW(TAG, "Got wrong UART checksum: 0x%02X - Calculated: 0x%02X", response[(response_len-1)], checksum);
            return -1;
        }

        co2		= response[3] << 8 | response[4];			// co2		: [DF1]*256 + [DF2]
        voc		= response[5] << 8 | response[6];			// voc		: [DF3]*256 + [DF4]
        humidity	= (response[7] << 8 | response[8]) / 10.0;		// humidity	: [DF5]*256 + [DF6]
        temperature	= ((response[9] << 8 | response[10]) - 500) / 10.0;	// temperature	: [DF7]*256 + [DF8] - 500
        pm1_0_grimm	= response[11] << 8 | response[12];			// pm1.0(GRIMM)	: [DF9]*256 + [DF10]
        pm2_5_grimm	= response[13] << 8 | response[14];			// pm2.5(GRIMM)	: [DF11]*256 + [DF12]
        pm10_grimm	= response[15] << 8 | response[16];			// pm10(GRIMM)	: [DF13]*256 + [DF14]
        pm1_0_tsi	= response[17] << 8 | response[18];			// pm1.0(TSI)	: [DF15]*256 + [DF16]
        pm2_5_tsi	= response[19] << 8 | response[20];			// pm2.5(TSI)	: [DF17]*256 + [DF18]
        pm10_tsi	= response[21] << 8 | response[22];			// pm10(TSI)	: [DF19]*256 + [DF20]
        state		= response[23];						// state(Alarm)	: [DF21]
										//	- bit 0: Fan at hHigh revolving speed
										//	- bit 1: Fan at lLow revolving speed
										//	- bit 2: Working temperature is high
										//	- bit 3: Working temperature is low
										//	- bit 4-7: Reserved
//        ESP_LOGD(TAG, "AM1008W Received CO₂=%uppm", co2);
//        ESP_LOGD(TAG, "AM1008W Received VOC=%ulevel", voc);
//        ESP_LOGD(TAG, "AM1008W Received Humidity=%u%", humidity);
//        ESP_LOGD(TAG, "AM1008W Received Temperature=%u°C", temperature);
//        ESP_LOGD(TAG, "AM1008W Received PM1.0(GRIMM)=%uug/m3", pm1_0_grimm);
//        ESP_LOGD(TAG, "AM1008W Received PM2.5(GRIMM)=%uug/m3", pm2_5_grimm);
//        ESP_LOGD(TAG, "AM1008W Received PM10(GRIMM)=%uug/m3", pm10_grimm);
//        ESP_LOGD(TAG, "AM1008W Received PM1.0(TSI)=%uug/m3", pm1_0_tsi);
//        ESP_LOGD(TAG, "AM1008W Received PM2.5(TSI)=%uug/m3", pm2_5_tsi);
//        ESP_LOGD(TAG, "AM1008W Received PM10(TSI)=%uug/m3", pm10_tsi);
        return 1;
    }

  private:
    const char *TAG = "am1008w"; // "cm1106";
    uint8_t AM1008W_CMD_GET_RESULT[5] = {0x11, 0x02, 0x01, 0x01, 0xEB}; // head, len, cmd, [data], crc
    uint8_t AM1008W_CMD_SET_CO2_CALIB[6] = {0x11, 0x03, 0x03, 0x00, 0x00, 0x00};
    uint8_t AM1008W_CMD_SET_CO2_CALIB_RESPONSE[4] = {0x16, 0x01, 0x03, 0xE6};
    
    // Checksum: 256-(HEAD+LEN+CMD+DATA)%256
    uint8_t calcCRC(uint8_t* response, size_t len) {
        uint8_t crc = 0;
        // last byte of response is checksum, don't calculate it
        for(int i = 0; i < len - 1; i++) {
            crc -= response[i];
        }
        return crc;
    }

    bool sendUartCommand(uint8_t *command, size_t commandLen, uint8_t *response = nullptr, size_t responseLen = 0) {
        // Empty RX Buffer
        while (available()) {
            read();
        }

        // calculate CRC
        command[commandLen - 1] = calcCRC(command, commandLen);

        write_array(command, commandLen);
        flush();

        if(response == nullptr) {
            return true;
        }

        return read_array(response, responseLen);
    }
};

class AM1008WSensor : public PollingComponent { //, public Sensor {
  private:
    AM1008W *am1008w; // cm1106;

  public:
    Sensor *co2_sensor = new Sensor();
    Sensor *voc_sensor = new Sensor();
    Sensor *humidity_sensor = new Sensor();
    Sensor *temperature_sensor = new Sensor();
    Sensor *pm1_0_grimm_sensor = new Sensor();
    Sensor *pm2_5_grimm_sensor = new Sensor();
    Sensor *pm10_grimm_sensor = new Sensor();
    Sensor *pm1_0_tsi_sensor = new Sensor();
    Sensor *pm2_5_tsi_sensor = new Sensor();
    Sensor *pm10_tsi_sensor = new Sensor();
    AM1008WSensor(UARTComponent *parent, uint32_t update_interval) : PollingComponent(update_interval) {
        am1008w = new AM1008W(parent);
//        cm1106 = new AM1008W(parent);
    }

    float get_setup_priority() const { return setup_priority::DATA; }

    void setup() override {
    }

    void update() override {
//        am1008w->getResult();
//        int16_t ppm = cm1106->getCo2PPM();
        if(am1008w->getResult() > -1) {
            co2_sensor->publish_state(am1008w->co2);
            voc_sensor->publish_state(am1008w->voc);
            humidity_sensor->publish_state(am1008w->humidity);
            temperature_sensor->publish_state(am1008w->temperature);
            pm1_0_grimm_sensor->publish_state(am1008w->pm1_0_grimm);
            pm2_5_grimm_sensor->publish_state(am1008w->pm2_5_grimm);
            pm10_grimm_sensor->publish_state(am1008w->pm10_grimm);
            pm1_0_tsi_sensor->publish_state(am1008w->pm1_0_tsi);
            pm2_5_tsi_sensor->publish_state(am1008w->pm2_5_tsi);
            pm10_tsi_sensor->publish_state(am1008w->pm10_tsi);
//            state_sensor->publish_state(am1008w->state);
//            publish_state(am1008w->state);
        }
    }
};

/*
class AM1008WCalibrateSwitch : public Component, public Switch {
  private:
    AM1008W *am1008w; // cm1106;

  public:
    AM1008WCalibrateSwitch(UARTComponent *parent) { 
        am1008w = new AM1008W(parent);
//        cm1106 = new AM1008W(parent);
    }

    void write_state(bool state) override {
        if(state) {
            publish_state(state);
            am1008w->setCo2CalibValue();
//            cm1106->setCo2CalibValue();
            turn_off();
        }
        else {
            publish_state(state);
        }
    }
};
*/
