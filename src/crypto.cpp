#include <Arduino.h>
#include <crypto.h>

HTTPClient http;
String APIbase = "https://api.binance.com/api/v3/";

int * queryPrice(String quote, String asset) {
    String fullURL = APIbase + "price?symbol=" + quote + asset;

    static int price[4];

    http.useHTTP10(true);
    http.begin(fullURL.c_str());

    int httpCode = http.GET();
    if (httpCode > 0) {
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, http.getStream());

        int p = doc["price"].as<int>();
        price[3] = p % 10;
        price[2] = (p / 10) % 10;
        price[1] = (p / 100) % 10;
        price[0] = (p / 1000) % 10;

        Serial.printf("Acquired response: %d\n", p);
        return price;
    } else {
        Serial.println("Error during API call to: " + fullURL);
        Serial.printf("Error code: %d\n", httpCode);

        return 0;
    }

    http.end();
}