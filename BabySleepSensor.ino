#include <WiFi.h>
#include <HTTPClient.h>
#include <esp8266-google-home-notifier.h>
#include "Ambient.h"

// GPIO
#define LED_OUT GPIO_NUM_14
#define PIR_IN GPIO_NUM_13

// WiFi
const char *ssid = "<YOUR-WIFI-SSID>";
const char *password = "<YOUR-WIFI-PASS>";
WiFiClient client;

// google-home-notifier
GoogleHomeNotifier ghn;
const char *displayName = "ダイニング ルーム"; // GoogleHomeの設定を確認してください。
const char *DEFCON0 = "怪獣が起きました！";
const char *DEFCON1 = "総員、第一戦闘配置";
const char *DEFCON2 = "総員、第二戦闘配置";
// max 15
#define DEFCON0_THRESHOLD 10
#define DEFCON1_THRESHOLD 7
#define DEFCON2_THRESHOLD 5
#define SAY_REPEAT 2
#define SPAN_TIME 1 // n分間

// Ambient
Ambient ambient;
unsigned int channelId = 9999;   // Ambientの設定を確認してください
const char *writeKey = "XXXXX";	 // Ambientの設定を確認してください
unsigned long span = SPAN_TIME * 60 * 1000; // n分
unsigned long start = 0;
int counter = 0;
bool status = false;

void setup()
{
    Serial.begin(115200);
    // initialize GPIO
    pinMode(LED_OUT, OUTPUT);
    pinMode(PIR_IN, INPUT);

    // initialize WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print("Connected to ");
    Serial.println(ssid);

    // initialize google-home-notifier
    Serial.println("connecting to Google Home...");
    if (ghn.device(displayName, "ja") != true)
    {
        Serial.println(ghn.getLastError());
        return;
    }
    Serial.print("found Google Home(");
    Serial.print(ghn.getIPAddress());
    Serial.print(":");
    Serial.print(ghn.getPort());
    Serial.println(")");

    // initialize Ambient
    ambient.begin(channelId, writeKey, &client);

    start = millis();
}

void loop()
{
    if (digitalRead(PIR_IN) == HIGH)
    {
        //Serial.println("ON");
        digitalWrite(LED_OUT, HIGH);

        if (!status)
        {
            // off から on に変わったときのみカウント
            counter++;
            Serial.println(String("CountUp:") + counter);
        }
        status = true;
    }
    else
    {
        status = false;

        //Serial.println("OFF");
        digitalWrite(LED_OUT, LOW);
    }

    // 1分経過したらデータを送信してカウンタを初期化
    unsigned long now = millis();
    if (now - start > span)
    {
        sendAmbient(counter);
        if (counter >= DEFCON0_THRESHOLD)
        {
            say(DEFCON0);
        }
        else if (counter >= DEFCON1_THRESHOLD)
        {
            say(DEFCON1);
        }
        else if (counter >= DEFCON2_THRESHOLD)
        {
            say(DEFCON2);
        }

        counter = 0;
        start = now;
    }
}

void sendAmbient(int counter)
{
    // Ambient に送る
    Serial.println(String("send to Ambient:") + counter);
    ambient.set(1, counter);
    ambient.send();
}

void say(const char *text)
{
    for (int i = 0; i < SAY_REPEAT; i++)
    {
        Serial.println(String("say:") + text);
        if (ghn.notify(text) != true)
        {
            Serial.println(ghn.getLastError());
            return;
        }
        delay(2000);
    }
}