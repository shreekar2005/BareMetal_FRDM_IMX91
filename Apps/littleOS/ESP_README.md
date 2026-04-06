### The Essential Power & Data Pins connection for "ESP-01 ESP8266 Serial WIFI Transceiver Module"
You **must** connect these 5 pins for the Wi-Fi bridge to work:

1. **3V3 (Power):** Connect to **P11 Pin 1** (`VRPI_3V3`).
2. **GND (Ground):** Connect to **P11 Pin 6** (`GND`).
3. **EN (Enable):** Connect to **P11 Pin 17** (`VRPI_3V3`).
   * *(Note: This pin is sometimes labeled `CH_PD`. It MUST be pulled to 3.3V for the ESP8266 to turn on. If you leave it disconnected, the module remains dead.)*
4. **TX (ESP Transmit):** Connect to **P11 Pin 10** (`GPIO_IO15`). 
   * *This routes into the i.MX91's `LPUART4_RX` input.*
5. **RX (ESP Receive):** Connect to **P11 Pin 8** (`GPIO_IO14`). 
   * *This receives data from the i.MX91's `LPUART4_TX` output.*

### The "Leave Disconnected" Pins
You should leave these 3 pins **completely unconnected** for normal operation:

* **RST (Reset):** Leave unconnected. It has a built-in pull-up resistor. 
* **GPIO0:** Leave unconnected. *(Warning: If you ground this pin, the ESP8266 enters "Firmware Flash" mode and will not boot your AT firmware).*
* **GPIO2:** Leave unconnected.

### A Quick Sanity Check Before Powering On
Before you plug the USB cable in to power up the i.MX91, double-check that your ESP8266 **3V3** pin is definitely going to a 3.3V source and NOT a 5V source. The ESP8266 has no overvoltage protection.