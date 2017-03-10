// Include Bluetooth

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <bluefruit.h>

// Neopixel
#define PIN            23   /* Pin used to drive the NeoPixels */

#define MAXCOMPONENTS  4
uint8_t *pixelBuffer = NULL;
uint8_t width = 0;
uint8_t height = 0;
uint8_t components = 3;     // only 3 and 4 are valid values
uint8_t stride;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel();

BLEUart bleuart;

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit Neopixel Test"));
  Serial.println(F("------------------------------------"));

  // Neopixels
  pixels.begin();

  Bluefruit.begin();
  Bluefruit.setName("Bluefruit52");

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Set up Advertising Packet
  setupAdv();

  // Start Advertising
  Bluefruit.Advertising.start();
  
}

void setupAdv(void)
{  
  //Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // There is no room for Name in Advertising packet
  // Use Scan response for Name
  Bluefruit.ScanResponse.addName();
}


void loop()
{
  // Echo received data
  if ( Bluefruit.connected() && bleuart.notifyEnabled() )
  {
    int command = bleuart.read();

    switch (command) {
      case 'V': {   // Get Version
          commandVersion();
          break;
        }
  
      case 'S': {   // Setup dimensions, components, stride...
          commandSetup();
          break;
       }

      case 'C': {   // Clear with color
          commandClearColor();
          break;
      }

      case 'B': {   // Set Brightness
          commandSetBrightness();
          break;
      }
            
      case 'P': {   // Set Pixel
          commandSetPixel();
          break;
      }
  
      case 'I': {   // Receive new image
          commandImage();
          break;
       }

    }
  }
}

void swapBuffers()
{
  uint8_t *base_addr = pixelBuffer;
  int pixelIndex = 0;
  for (int j = 0; j < height; j++)
  {
    for (int i = 0; i < width; i++) {
      if (components == 3) {
        pixels.setPixelColor(pixelIndex, pixels.Color(*base_addr, *(base_addr+1), *(base_addr+2)));
      }
      else {
        Serial.println(F("TODO: implement me"));
      }
      base_addr+=components;
      pixelIndex++;
    }
    pixelIndex += stride - width;   // move pixelIndex to the next row (take into account the stride)
  }
  pixels.show();

}

void commandVersion() {
  Serial.println(F("Command: Version check"));
  sendResponse("Neopixel v1.0");
}

void commandSetup() {
  Serial.println(F("Command: Setup"));

  width = bleuart.read();
  height = bleuart.read();
  components = bleuart.read();
  stride = bleuart.read();
  neoPixelType pixelType;
  pixelType = bleuart.read();
  pixelType += bleuart.read()<<8;
  
  Serial.printf("\tsize: %dx%d\n", width, height);
  Serial.printf("\tcomponents: %d\n", components);
  Serial.printf("\tstride: %d\n", stride);
  Serial.printf("\tpixelType %d\n", pixelType );


  if (pixelBuffer != NULL) {
      delete[] pixelBuffer;
  }

  uint32_t size = width*height;
  pixelBuffer = new uint8_t[size*components];
  pixels.updateLength(size);
  pixels.updateType(pixelType);
  pixels.setPin(PIN);

  // Done
  sendResponse("OK");
}

void commandSetBrightness() {
  Serial.println(F("Command: SetBrightness"));

   // Read value
  uint8_t brightness = bleuart.read();

  // Set brightness
  pixels.setBrightness(brightness);

  // Refresh pixels
  swapBuffers();

  // Done
  sendResponse("OK");
}

void commandClearColor() {
  Serial.println(F("Command: ClearColor"));

  // Read color
  uint8_t color[MAXCOMPONENTS];
  for (int j = 0; j < components;) {
    if (bleuart.available()) {
      color[j] = bleuart.read();
      j++;
    }
  }

  // Set all leds to color
  int size = width * height;
  uint8_t *base_addr = pixelBuffer;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < components; j++) {
      *base_addr = color[j];
      base_addr++;
    }
  }

  // Swap buffers
  Serial.println(F("ClearColor completed"));
  swapBuffers();


  if (components == 3) {
    Serial.printf("\tcolor (%d, %d, %d)\n", color[0], color[1], color[2] );
  }
  
  // Done
  sendResponse("OK");
}

void commandSetPixel() {
  Serial.println(F("Command: SetPixel"));

  // Read position
  uint8_t x = bleuart.read();
  uint8_t y = bleuart.read();

  // Read colors
  uint32_t pixelIndex = y*width+x;
  uint32_t pixelComponentOffset = pixelIndex*components;
  uint8_t *base_addr = pixelBuffer+pixelComponentOffset;
  for (int j = 0; j < components;) {
    if (bleuart.available()) {
      *base_addr = bleuart.read();
      base_addr++;
      j++;
    }
  }

  // Set colors
  if (components == 3) {
    uint32_t pixelIndex = y*stride+x;
    pixels.setPixelColor(pixelIndex, pixels.Color(pixelBuffer[pixelComponentOffset], pixelBuffer[pixelComponentOffset+1], pixelBuffer[pixelComponentOffset+2]));

    Serial.printf("\tcolor (%d, %d, %d)\n", pixelBuffer[pixelComponentOffset], pixelBuffer[pixelComponentOffset+1], pixelBuffer[pixelComponentOffset+2] );
  }
  else {
    Serial.println(F("TODO: implement me"));
  }
  pixels.show();

  // Done
  sendResponse("OK");
}

void commandImage() {
  Serial.printf("Command: Image %dx%d, %d, %d\n", width, height, components, stride);
  
  // Receive new pixel buffer
  int size = width * height;
  uint8_t *base_addr = pixelBuffer;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < components;) {
      if (bleuart.available()) {
        *base_addr = bleuart.read();
        base_addr++;
        j++;
      }
    }

/*
    if (components == 3) {
      uint32_t index = i*components;
      Serial.printf("\tp%d (%d, %d, %d)\n", i, pixelBuffer[index], pixelBuffer[index+1], pixelBuffer[index+2] );
    }
    */
  }

  // Swap buffers
  Serial.println(F("Image received"));
  swapBuffers();

  // Done
  sendResponse("OK");
}

void sendResponse(char const *response) {
    Serial.printf("Send Response: %s\n", response);
    bleuart.write(response, strlen(response)*sizeof(char));
}

