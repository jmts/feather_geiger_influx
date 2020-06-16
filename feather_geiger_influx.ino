#include <ESP8266WiFi.h>

#include "wifi_secrets.h"
#include "influx_secrets.h"

#define BOOP_PIN 12
#define BAUD_RATE 115200

#define DONT_WAIT_FOR_RESPONSE 0
#define WAIT_FOR_RESPONSE 1

volatile unsigned long isr_pulses;

ICACHE_RAM_ATTR void isr_pulse() {
  isr_pulses += 1;
}

static void isr_init(void) {
  isr_pulses = 0;

  attachInterrupt(digitalPinToInterrupt(BOOP_PIN), isr_pulse, RISING);
}

static unsigned long pulses_get() {
  unsigned long pulses;

  noInterrupts();
  pulses = isr_pulses;
  isr_pulses = 0;
  interrupts();

  return pulses;
}

static void serial_init(void) {
  Serial.begin(BAUD_RATE);
}

static void wifi_init(void) {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SECRET_SSID);

  WiFi.begin(WIFI_SECRET_SSID, WIFI_SECRET_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  serial_init();
  delay(5000);
  wifi_init();
  isr_init();
}

static void send_secure_request(int value, int wait)
{
  unsigned int sent;
  String postData, path, request, line;
  BearSSL::WiFiClientSecure client;

  client.setInsecure();

  if (!client.connect(INFLUX_SECRET_HOST, INFLUX_SECRET_PORT)) {
    Serial.println("Connection failed");
    return;
  }

  postData = String(INFLUX_SECRET_MEASUREMENT) + " value=" + value;

  path = String("/api/v2/write?org=") + INFLUX_SECRET_ORG + "&bucket=" + INFLUX_SECRET_BUCKET;

  request = String("POST ") + path + " HTTP/1.1\r\n" +
    "Host: " + INFLUX_SECRET_HOST + "\r\n" +
    "Authorization: Token " + INFLUX_SECRET_TOKEN + "\r\n" +
    "Content-Type: application/octet-stream\r\n" +
    "Content-Length: " + postData.length() + "\r\n" +
    "Connection: close\r\n" +
    "\r\n" + postData +
    "\r\n";

  Serial.print(request);
  sent = client.print(request);

  Serial.println(String("Sent ") + sent + " of " + request.length() + " bytes");

  if (!wait) {
    return;
  }

  delay(500);

  while (client.available()) {
    line = client.readStringUntil('\r');
    Serial.print(line);
  }
}

void loop() {
  unsigned long then, now;
  unsigned long pulses;

  Serial.println("Sending request...");
  then = millis();

  pulses = pulses_get();
  send_secure_request(pulses, DONT_WAIT_FOR_RESPONSE);

  now = millis();
  Serial.print("Elapsed: ");
  Serial.println(now - then);
}
