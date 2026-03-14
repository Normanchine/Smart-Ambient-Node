#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- 1. 网络与 ThingsBoard 配置 ---
const char* ssid = "510_2.4G";         /* <--- 修改这里 */
const char* password = "510510510";     /* <--- 修改这里 */
const char* mqtt_server = "192.168.2.106";  // 你的电脑 IP
const char* token = "R5Qkm5qVDpvIO5sZhWCj";       /* <--- 修改这里 (从 TB 网页复制) */

// --- 2. 硬件引脚定义 (基于你之前的描述) ---
// OLED SPI: SCL=D5(14), SDA=D7(13), CS=D4(2), DC=D3(0), RES=D2(4)
U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, 14, 13, 2, 0, 4);

// DS18B20: 接 D6 (GPIO12)
OneWire oneWire(12); 
DallasTemperature sensors(&oneWire);

WiFiClient espClient;
PubSubClient client(espClient);

// --- 3. 函数实现 ---
void setup_wifi() {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Connecting to ThingsBoard...");
        // ThingsBoard 认证: 用户名=Token, 密码=NULL
        if (client.connect("ESP8266_Node", token, NULL)) {
            Serial.println("Success!");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Try again in 5s");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    u8g2.begin();
    sensors.begin();
    setup_wifi();
    client.setServer(mqtt_server, 1883);
}

void loop() {
    if (!client.connected()) reconnect();
    client.loop();

    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 5000) { // 每 5 秒更新一次
        lastUpdate = millis();

        // A. 读取数据
        sensors.requestTemperatures();
        float temp = sensors.getTempCByIndex(0);
        Serial.printf("Temp: %.2f\n", temp);

        // B. 本地 OLED 显示
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(0, 12, "TB Node: Online");
        u8g2.setFont(u8g2_font_logisoso24_tf); 
        u8g2.setCursor(0, 50);
        u8g2.print(temp, 1); u8g2.print(" C");
        u8g2.sendBuffer();

        // C. 上传到 ThingsBoard (JSON 格式)
        StaticJsonDocument<128> doc;
        doc["temperature"] = temp;
        char buffer[128];
        serializeJson(doc, buffer);
        client.publish("v1/devices/me/telemetry", buffer);
    }
}