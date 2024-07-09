#include "M5Unified.h"
#include "M5_Max3421E_Usb.h"
#include <DFRobot_DF1201S.h>

#define DF1201SSerial Serial1

#define D3 13
#define D2 14

// Modifier bit mask
#define HID_LEFT_CONTROL                                                                (1 << 0)
#define HID_LEFT_SHIFT                                                                  (1 << 1)
#define HID_LEFT_ALT                                                                    (1 << 2)
#define HID_LEFT_GUI                                                                    (1 << 3)
#define HID_RIGHT_CONTROL                                                               (1 << 4)
#define HID_RIGHT_SHIFT                                                                 (1 << 5)
#define HID_RIGHT_ALT                                                                   (1 << 6)
#define HID_RIGHT_GUI                                                                   (1 << 7)

#define HID_KEY_A       0x04
#define HID_KEY_SLASH   0x38
#define HID_KEY_ENTER   0x28
// Language ID: English
#define LANGUAGE_ID 0x0409

#define PIN_WS2812B 32  // The ESP32 pin GPIO32 connected to WS2812B
#define NUM_PIXELS 88   // The number of LEDs (pixels) on WS2812B LED strip

#define BLACK    0x0000
#define BLUE     0x001F
#define NAVY     0x1810
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0
#define ORANGE   0xF9C0
#define GREY1    0x6B6D
#define GREY2    0x4A49
#define WHITE    0xFFFF

/*
  The board is equipped with two dip switches to adapt to different M5 series hosts.
  https://static-cdn.m5stack.com/resource/docs/products/module/USB%20v1.2%20Module/pinMap-70b8e2ad-8325-4887-af33-44e3dae91520.png
  If you need to change the spi pin, use these spi configuration settings
  M5_USBH_Host USBHost(&SPI, 18, 23, 19, 5, 35);
*/

//M5_USBH_Host USBHost(&SPI, 18, 23, 19, 5, 35); //Core 1
M5_USBH_Host USBHost(&SPI, 18, 23, 38, 33, 35); //Core 2 SPI Clock, SPI MOSI, SPI MISO, SPI SLAVE, SPI INT

M5Canvas canvas(&M5.Display);

typedef struct {
    tusb_desc_device_t desc_device;
    uint16_t manufacturer[32];
    uint16_t product[48];
    uint16_t serial[16];
    bool mounted;
} dev_info_t;

typedef struct {
    String name;
    uint32_t size;
    bool isDir;
}file_info_t;


/**
 * @brief Scancode to ascii table
 */
const uint8_t keycode2ascii[57][2] = {
    {0, 0},     /* HID_KEY_NO_PRESS        */
    {0, 0},     /* HID_KEY_ROLLOVER        */
    {0, 0},     /* HID_KEY_POST_FAIL       */
    {0, 0},     /* HID_KEY_ERROR_UNDEFINED */
    {'a', 'A'}, /* HID_KEY_A               */
    {'b', 'B'}, /* HID_KEY_B               */
    {'c', 'C'}, /* HID_KEY_C               */
    {'d', 'D'}, /* HID_KEY_D               */
    {'e', 'E'}, /* HID_KEY_E               */
    {'f', 'F'}, /* HID_KEY_F               */
    {'g', 'G'}, /* HID_KEY_G               */
    {'h', 'H'}, /* HID_KEY_H               */
    {'i', 'I'}, /* HID_KEY_I               */
    {'j', 'J'}, /* HID_KEY_J               */
    {'k', 'K'}, /* HID_KEY_K               */
    {'l', 'L'}, /* HID_KEY_L               */
    {'m', 'M'}, /* HID_KEY_M               */
    {'n', 'N'}, /* HID_KEY_N               */
    {'o', 'O'}, /* HID_KEY_O               */
    {'p', 'P'}, /* HID_KEY_P               */
    {'q', 'Q'}, /* HID_KEY_Q               */
    {'r', 'R'}, /* HID_KEY_R               */
    {'s', 'S'}, /* HID_KEY_S               */
    {'t', 'T'}, /* HID_KEY_T               */
    {'u', 'U'}, /* HID_KEY_U               */
    {'v', 'V'}, /* HID_KEY_V               */
    {'w', 'W'}, /* HID_KEY_W               */
    {'x', 'X'}, /* HID_KEY_X               */
    {'y', 'Y'}, /* HID_KEY_Y               */
    {'z', 'Z'}, /* HID_KEY_Z               */
    {'1', '!'}, /* HID_KEY_1               */
    {'2', '@'}, /* HID_KEY_2               */
    {'3', '#'}, /* HID_KEY_3               */
    {'4', '$'}, /* HID_KEY_4               */
    {'5', '%'}, /* HID_KEY_5               */
    {'6', '^'}, /* HID_KEY_6               */
    {'7', '&'}, /* HID_KEY_7               */
    {'8', '*'}, /* HID_KEY_8               */
    {'9', '('}, /* HID_KEY_9               */
    {'0', ')'}, /* HID_KEY_0               */
    {'E', 'E'}, /* HID_KEY_ENTER */
    {0, 0},      /* HID_KEY_ESC             */
    {'\b', 0},   /* HID_KEY_DEL             */
    {0, 0},      /* HID_KEY_TAB             */
    {' ', ' '},  /* HID_KEY_SPACE           */
    {'-', '_'},  /* HID_KEY_MINUS           */
    {'=', '+'},  /* HID_KEY_EQUAL           */
    {'[', '{'},  /* HID_KEY_OPEN_BRACKET    */
    {']', '}'},  /* HID_KEY_CLOSE_BRACKET   */
    {'\\', '|'}, /* HID_KEY_BACK_SLASH      */
    {'\\', '|'},
    /* HID_KEY_SHARP           */ // HOTFIX: for NonUS Keyboards repeat
                                  // HID_KEY_BACK_SLASH
    {';', ':'},                   /* HID_KEY_COLON           */
    {'\'', '"'},                  /* HID_KEY_QUOTE           */
    {'`', '~'},                   /* HID_KEY_TILDE           */
    {',', '<'},                   /* HID_KEY_LESS            */
    {'.', '>'},                   /* HID_KEY_GREATER         */
    {'/', '?'}                    /* HID_KEY_SLASH           */
};

char qrcode1[20] = "Centrum";
char qrcode2[20] = "Vitamin C";
char qrcode3[20] = "Decolgen";
char qrcode4[20] = "Paracetamol";
char qrcode5[20] = "Claritin";
char qrcode6[20] = "Imodium";
char qrcode7[20] = "Hemostan";

char barcode_input[25] = {0};
uint8_t i = 0;

dev_info_t dev_info[CFG_TUH_DEVICE_MAX] = {0};

extern SemaphoreHandle_t max3421_intr_sem;

DFRobot_DF1201S DF1201S;

struct Button {
	const uint8_t PIN;
	uint32_t numberKeyPresses;
	bool pressed;
};

Button button1 = {26, 0, false};

void IRAM_ATTR isr() {	  
  //button1.pressed = true;
  ESP.restart();
}

void print_lsusb(void) {
    bool no_device = true;
    for (uint8_t daddr = 1; daddr < CFG_TUH_DEVICE_MAX + 1; daddr++) {
        // TODO can use tuh_mounted(daddr), but tinyusb has an bug
        // use local connected flag instead
        dev_info_t *dev = &dev_info[daddr - 1];
        if (dev->mounted) {
            Serial.printf("Device %u: ID %04x:%04x %s %s\r\n", daddr,
                          dev->desc_device.idVendor, dev->desc_device.idProduct,
                          (char *)dev->manufacturer, (char *)dev->product);

            no_device = false;
        }
    }

    if (no_device) {
        Serial.println("No device connected (except hub)");
    }
}

static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len,
                                     uint8_t *utf8, size_t utf8_len) {
    // TODO: Check for runover.
    (void)utf8_len;
    // Get the UTF-16 length out of the data itself.

    for (size_t i = 0; i < utf16_len; i++) {
        uint16_t chr = utf16[i];
        if (chr < 0x80) {
            *utf8++ = chr & 0xff;
        } else if (chr < 0x800) {
            *utf8++ = (uint8_t)(0xC0 | (chr >> 6 & 0x1F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
        } else {
            // TODO: Verify surrogate.
            *utf8++ = (uint8_t)(0xE0 | (chr >> 12 & 0x0F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 6 & 0x3F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
        }
        // TODO: Handle UTF-16 code points that take two entries.
    }
}

// Count how many bytes a utf-16-le encoded string will take in utf-8.
static int _count_utf8_bytes(const uint16_t *buf, size_t len) {
    size_t total_bytes = 0;
    for (size_t i = 0; i < len; i++) {
        uint16_t chr = buf[i];
        if (chr < 0x80) {
            total_bytes += 1;
        } else if (chr < 0x800) {
            total_bytes += 2;
        } else {
            total_bytes += 3;
        }
        // TODO: Handle UTF-16 code points that take two entries.
    }
    return total_bytes;
}

void utf16_to_utf8(uint16_t *temp_buf, size_t buf_len) {
    size_t utf16_len = ((temp_buf[0] & 0xff) - 2) / sizeof(uint16_t);
    size_t utf8_len  = _count_utf8_bytes(temp_buf + 1, utf16_len);

    _convert_utf16le_to_utf8(temp_buf + 1, utf16_len, (uint8_t *)temp_buf,
                             buf_len);
    ((uint8_t *)temp_buf)[utf8_len] = '\0';
}

//--------------------------------------------------------------------+
// Print Device Descriptor 
//--------------------------------------------------------------------+

void print_device_descriptor(tuh_xfer_t *xfer) {
    if (XFER_RESULT_SUCCESS != xfer->result) {
        Serial.printf("Failed to get device descriptor\r\n");
        return;
    }

    uint8_t const daddr      = xfer->daddr;
    dev_info_t *dev          = &dev_info[daddr - 1];
    tusb_desc_device_t *desc = &dev->desc_device;

    Serial.printf("Device %u: ID %04x:%04x\r\n", daddr, desc->idVendor,
                  desc->idProduct);
    Serial.printf("Device Descriptor:\r\n");
    Serial.printf("  bLength             %u\r\n", desc->bLength);
    Serial.printf("  bDescriptorType     %u\r\n", desc->bDescriptorType);
    Serial.printf("  bcdUSB              %04x\r\n", desc->bcdUSB);
    Serial.printf("  bDeviceClass        %u\r\n", desc->bDeviceClass);
    Serial.printf("  bDeviceSubClass     %u\r\n", desc->bDeviceSubClass);
    Serial.printf("  bDeviceProtocol     %u\r\n", desc->bDeviceProtocol);
    Serial.printf("  bMaxPacketSize0     %u\r\n", desc->bMaxPacketSize0);
    Serial.printf("  idVendor            0x%04x\r\n", desc->idVendor);
    Serial.printf("  idProduct           0x%04x\r\n", desc->idProduct);
    Serial.printf("  bcdDevice           %04x\r\n", desc->bcdDevice);

    // Get String descriptor using Sync API
    Serial.printf("  iManufacturer       %u     ", desc->iManufacturer);
    if (XFER_RESULT_SUCCESS ==
        tuh_descriptor_get_manufacturer_string_sync(
            daddr, LANGUAGE_ID, dev->manufacturer, sizeof(dev->manufacturer))) {
        utf16_to_utf8(dev->manufacturer, sizeof(dev->manufacturer));
        Serial.printf((char *)dev->manufacturer);
    }
    Serial.printf("\r\n");

    Serial.printf("  iProduct            %u     ", desc->iProduct);
    if (XFER_RESULT_SUCCESS ==
        tuh_descriptor_get_product_string_sync(daddr, LANGUAGE_ID, dev->product,
                                               sizeof(dev->product))) {
        utf16_to_utf8(dev->product, sizeof(dev->product));
        Serial.printf((char *)dev->product);
    }
    Serial.printf("\r\n");

    Serial.printf("  iSerialNumber       %u     ", desc->iSerialNumber);
    if (XFER_RESULT_SUCCESS ==
        tuh_descriptor_get_serial_string_sync(daddr, LANGUAGE_ID, dev->serial,
                                              sizeof(dev->serial))) {
        utf16_to_utf8(dev->serial, sizeof(dev->serial));
        Serial.printf((char *)dev->serial);
    }
    Serial.printf("\r\n");

    Serial.printf("  bNumConfigurations  %u\r\n", desc->bNumConfigurations);

    // print device summary
    print_lsusb();
}

/**
 * @brief HID Keyboard modifier verification for capitalization application
 * (right or left shift)
 *
 * @param[in] modifier
 * @return true  Modifier was pressed (left or right shift)
 * @return false Modifier was not pressed (left or right shift)
 *
 */
static inline bool hid_keyboard_is_modifier_shift(uint8_t modifier) {
  if (((modifier & HID_LEFT_SHIFT) == HID_LEFT_SHIFT) ||
      ((modifier & HID_RIGHT_SHIFT) == HID_RIGHT_SHIFT)) {
    return true;
  }
  return false;
}

/**
 * @brief HID Keyboard get char symbol from key code
 *
 * @param[in] modifier  Keyboard modifier data
 * @param[in] key_code  Keyboard key code
 * @param[in] key_char  Pointer to key char data
 *
 * @return true  Key scancode converted successfully
 * @return false Key scancode unknown
 */
static inline bool hid_keyboard_get_char(uint8_t modifier, uint8_t key_code,
                                         unsigned char *key_char) {
  uint8_t mod = (hid_keyboard_is_modifier_shift(modifier)) ? 1 : 0;

  if ((key_code >= HID_KEY_A) && (key_code <= HID_KEY_SLASH)) {
    *key_char = keycode2ascii[key_code][mod];  
  }

  return true;
}

void report_parse(uint8_t const* report) {
    if (xSemaphoreTake(max3421_intr_sem, 0)) {
        xSemaphoreGive(max3421_intr_sem);
    }    
}

static uint8_t report_data[4] = {0};

void setup() {
    M5.begin();
    M5.Power.begin();   
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(55, 1);  
    M5.Lcd.setTextSize(3);    // Set the font size to 4    
    M5.Lcd.print("MEDICINE ID");
    Serial.println("MEDICINE ID START");

    pinMode(button1.PIN, INPUT_PULLUP);
	  attachInterrupt(button1.PIN, isr, FALLING);   

    DF1201SSerial.begin(115200, SERIAL_8N1, /*rx =*/D3, /*tx =*/D2);

    while (!DF1201S.begin(DF1201SSerial)) {
      Serial.println("Init failed, please check the wire connection!");
      delay(1000);
    }

    /*Set volume to 20*/
    DF1201S.setVol(/*VOL = */15);
    Serial.print("VOL:");
    /*Get volume*/
    Serial.println(DF1201S.getVol());
    DF1201S.switchFunction(DF1201S.MUSIC);
    /*Wait for the end of the prompt tone */
    delay(2000);
    /*Set playback mode to "repeat all"*/
    DF1201S.setPlayMode(DF1201S.SINGLE);
    Serial.print("PlayMode:");
    /*Get playback mode*/
    Serial.println(DF1201S.getPlayMode());

    DF1201S.playSpecFile("/Intro.mp3");

    // Init USB Host on native controller roothub port0
    if (USBHost.begin(0)) {
        Serial.println("MODULE INIT OK");
    } else {
        Serial.println("MODULE INIT ERROR");
    }          
}

void loop() { 
    USBHost.task(); 
}

void tuh_hid_report_sent_cb(uint8_t dev_addr, uint8_t idx,
                            uint8_t const* report, uint16_t len) {
}

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use.
// tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
// descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
// it will be skipped therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const* desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    Serial.printf("HID device address = %d, instance = %d is mounted\r\n",
                  dev_addr, instance);
    Serial.printf("VID = %04x, PID = %04x\r\n", vid, pid);
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        Serial.printf("Error: cannot request to receive report\r\n");
    }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    Serial.printf("HID device address = %d, instance = %d is unmounted\r\n",
                  dev_addr, instance);
}



// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const* report, uint16_t len) {
    

    unsigned char key_char;    
    /*
    Serial.printf("HIDreport : ");
    for (uint16_t i = 0; i < len; i++) {
        Serial.printf("0x%02X ", report[i]);
    }
    
    Serial.println();
    */
    if(report[2] != 0)
    {
      if (hid_keyboard_get_char(report[0], report[2], &key_char)) 
      {
        Serial.printf("%c", key_char);
        barcode_input[i++] = key_char;
        if(report[2] == HID_KEY_ENTER) // barcode scan done
        {
          i = i-1;
          barcode_input[i] = 0;
          Serial.printf(" Enter Key Detected");
          Serial.println();
          if(strcmp(barcode_input, qrcode1) == 0)
          { 
            M5.Lcd.setCursor(0, 100);  
            M5.Lcd.setTextSize(3);    // Set the font size to 4 
            M5.Lcd.print("           ");
            M5.Lcd.setCursor(0, 100);           
            M5.Lcd.print(barcode_input);
            M5.Lcd.setCursor(0, 140);
            M5.Lcd.print("                  "); 
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("Centrum 500 mg");
            Serial.printf("Centrum 500 mg");
            Serial.println(); 
            DF1201S.playSpecFile("/Centrum.mp3");                                  
          }
          else if(strcmp(barcode_input, qrcode2) == 0)
          {
            M5.Lcd.setCursor(0, 100);  
            M5.Lcd.setTextSize(3);    // Set the font size to 4     
            M5.Lcd.print("           ");
            M5.Lcd.setCursor(0, 100);         
            M5.Lcd.print(barcode_input);
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("                  ");
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("Vitamin C 500 mg");
            Serial.printf("Vitamin C 500 mg");
            Serial.println();           
            DF1201S.playSpecFile("/Vitaminc.mp3");         
          }
          else if(strcmp(barcode_input, qrcode3) == 0)
          {
            M5.Lcd.setCursor(0, 100);  
            M5.Lcd.setTextSize(3);    // Set the font size to 4    
            M5.Lcd.print("           ");
            M5.Lcd.setCursor(0, 100);          
            M5.Lcd.print(barcode_input);
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("                  ");
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("Decolgen 500 mg");
            Serial.printf("Decolgen 500 mg");
            Serial.println();            
            DF1201S.playSpecFile("/Decolgen.mp3");          
          }
          else if(strcmp(barcode_input, qrcode4) == 0)
          {
            M5.Lcd.setCursor(0, 100);  
            M5.Lcd.setTextSize(3);    // Set the font size to 4    
            M5.Lcd.print("           ");
            M5.Lcd.setCursor(0, 100);         
            M5.Lcd.print(barcode_input);
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("                  ");
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("Paracetamol 500 mg");
            Serial.printf("Paracetamol 500 mg");
            Serial.println();              
            DF1201S.playSpecFile("/Para.mp3");            
          }
          else if(strcmp(barcode_input, qrcode5) == 0)
          {
            M5.Lcd.setCursor(0, 100);  
            M5.Lcd.setTextSize(3);    // Set the font size to 4    
            M5.Lcd.print("           ");
            M5.Lcd.setCursor(0, 100);          
            M5.Lcd.print(barcode_input);
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("                  ");
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("Claritin 10 mg");
            Serial.printf("Claritin 10 mg");
            Serial.println();              
            DF1201S.playSpecFile("/Claritin.mp3");  
          }
          else if(strcmp(barcode_input, qrcode6) == 0)
          {
            M5.Lcd.setCursor(0, 100);  
            M5.Lcd.setTextSize(3);    // Set the font size to 4    
            M5.Lcd.print("               ");
            M5.Lcd.setCursor(0, 100);          
            M5.Lcd.print(barcode_input);
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("                  ");
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("Imodium 2 mg");
            Serial.printf("Imodium 2 mg");
            Serial.println();              
            DF1201S.playSpecFile("/Imodium.mp3");     
          }
          else if(strcmp(barcode_input, qrcode7) == 0)
          { 
            M5.Lcd.setCursor(0, 100);  
            M5.Lcd.setTextSize(3);    // Set the font size to 4 
            M5.Lcd.print("           ");  
            M5.Lcd.setCursor(0, 100);          
            M5.Lcd.print(barcode_input);          
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("                  ");
            M5.Lcd.setCursor(0, 140); 
            M5.Lcd.print("Hemostan 500 mg");
            Serial.printf("Hemostan 500 mg");  
            Serial.println();             
            DF1201S.playSpecFile("/Hemostan.mp3");                    
          }               
          else
          {
            Serial.printf("Strcmp failed ! ! !");
            Serial.println();
          }

          i = 0;
          memset(barcode_input, '\0', 20);
        }
      }
    }
    
    //Serial.println();
    //report_parse(report);
   
    // continue to request to receive report
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        Serial.printf("Error: cannot request to receive report\r\n");
    }
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted (configured)
void tuh_mount_cb(uint8_t daddr) {
    Serial.printf("Device attached, address = %d\r\n", daddr);

    dev_info_t* dev = &dev_info[daddr - 1];
    dev->mounted    = true;
    // Get Device Descriptor
    tuh_descriptor_get_device(daddr, &dev->desc_device, 18,
                              print_device_descriptor, 0);
}

/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr) {
    Serial.printf("Device removed, address = %d\r\n", daddr);
    dev_info_t* dev = &dev_info[daddr - 1];
    dev->mounted    = false;

    // print device summary
    print_lsusb();
}