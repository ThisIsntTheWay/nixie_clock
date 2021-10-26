#include <Arduino.h>
#include <crypto.h>

HTTPClient http;
String APIbase = "https://api.binance.com/api/v3/";

int * queryPrice(String quote, String asset) {
    String fullURL = APIbase + "ticker/price?symbol=" + asset + quote;

    static int price[4];

    http.useHTTP10(true);
    http.begin(fullURL.c_str());

    int httpCode = http.GET();
    if (httpCode > 0) {
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, http.getStream());

        int p = doc["price"].as<int>();
        if (p > 9999) {
            p = p / 10;
        }
        
        price[3] = p % 10;
        price[2] = (p / 10) % 10;
        price[1] = (p / 100) % 10;
        price[0] = (p / 1000) % 10;

        #ifdef DEBUG
            Serial.print("[T] Crypto: Acquired price: "); Serial.println(p);
        #endif
        return price;
    } else {
        #ifdef DEBUG
            Serial.print("Error during API call to: " + fullURL + ", error code: ");
            Serial.println(httpCode);
        #endif

        return 0;
    }

    http.end();
}