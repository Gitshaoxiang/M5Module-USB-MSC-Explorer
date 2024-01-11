#include "M5Unified.h"
#include "Adafruit_TinyUSB.h"
#include "FS.h"
#include "SdFat.h"

// background
#include "mounted.h"
#include "unmount.h"
#include "detail.h"

#include "device_def.h"
#include "device_desc.h"

#include "vector"

#include "./utils/smooth_menu/simple_menu/simple_menu.h"
#include "./utils/smooth_menu/simple_menu/custom_render_callback.h"

//   Adafruit_USBH_Host(SPIClass *spi, int8_t sck, int8_t mosi, int8_t miso,
//                      int8_t cs, int8_t intr);
Adafruit_USBH_Host USBHost(&SPI, 18, 23, 19, 5, 35);
Adafruit_USBH_MSC_BlockDevice msc_host;
FatVolume fatfs;
bool is_mounted    = false;
bool status_change = false;
// CFG_TUH_DEVICE_MAX is defined by tusb_config header
dev_info_t dev_info[CFG_TUH_DEVICE_MAX] = {0};

static std::vector<file_info_t> file_list;

M5Canvas canvas(&M5.Display);
M5Canvas detail_canvas(&M5.Display);

static SMOOTH_MENU::Simple_Menu* menu                    = nullptr;
static SMOOTH_MENU::SimpleMenuCallback_t* menu_render_cb = nullptr;

static LVGL::Anim_Path* menuAnim = nullptr;
static LVGL::Anim_Path* pickAnim = nullptr;

#define MENU_HEIGHT 130
#define MENU_WIDTH  320
// #define BASE_X_POS       MENU_WIDTH / 2
#define BASE_X_POS       10
#define BASE_Y_POS       35
#define BASE_ITEM_HEIGHT 35

int file_pickup = 0;

void createMenu();
void canvasInit();
void list_dir();
void view_file(String path);
void BtnTask(void* pvParameters);

void setup() {
    M5.begin();
    Serial.println("M5Module USB HOST MSC Explorer");
    canvasInit();
    createMenu();

    // Init USB Host on native controller roothub port0
    if (USBHost.begin(0)) {
        Serial.println("OK");
    } else {
        Serial.println("ERROR");
    }
    xTaskCreate(BtnTask, "BtnTask", 1024 * 4, NULL, 2, NULL);
}

void loop() {
    USBHost.task();
}

void BtnTask(void* pvParameters) {
    while (1) {
        if (is_mounted && file_list.size() > 0) {
            M5.update();
            if (M5.BtnA.wasPressed()) {
                M5.Speaker.tone(600, 10);
                menu->goLast();
                menuAnim->resetTime(millis());
                pickAnim->resetTime(millis());
            }
            if (M5.BtnB.wasPressed()) {
                M5.Speaker.tone(600, 10);
                Serial.println("Pick up!");
                SMOOTH_MENU::RenderAttribute_t file_pickup =
                    menu->getSelector()->getRenderAttribute();

                Serial.printf("current item id: %d\n", file_pickup.targetItem);
                view_file(file_list[file_pickup.targetItem].name);
            }
            if (M5.BtnC.wasPressed()) {
                M5.Speaker.tone(600, 10);
                menu->goNext();
                menuAnim->resetTime(millis());
                pickAnim->resetTime(millis());
            }
            menu->update(millis());
        }
        if (status_change) {
            status_change = false;
            if (is_mounted) {
                M5.Display.pushImage(0, 0, 320, 240, image_data_mounted);

            } else {
                M5.Display.pushImage(0, 0, 320, 240, image_data_unmount);
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted (configured)
void tuh_mount_cb(uint8_t daddr) {
    msc_host.begin(daddr);
    is_mounted = fatfs.begin(&msc_host);
    if (is_mounted) {
        status_change = true;
        Serial.println("mount!");
        list_dir();
    } else {
        Serial.println(
            "Failed to mount mass storage device. Make sure it is formatted as "
            "FAT");
    }
    Serial.printf("Device attached, address = %d\r\n", daddr);

    dev_info_t* dev = &dev_info[daddr - 1];
    dev->mounted    = true;

    // Get Device Descriptor
    tuh_descriptor_get_device(daddr, &dev->desc_device, 18,
                              print_device_descriptor, 0);
}

/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr) {
    if (is_mounted) {
        is_mounted    = false;
        status_change = true;
        fatfs.end();
        msc_host.end();
        file_list.clear();
        menu->getMenu()->clearAllItem();
    }

    Serial.printf("Device removed, address = %d\r\n", daddr);
    dev_info_t* dev = &dev_info[daddr - 1];
    dev->mounted    = false;

    // print device summary
    print_lsusb();
}

// UI
void canvasInit() {
    M5.begin();
    M5.Display.begin();
    M5.Display.pushImage(0, 0, 320, 240, image_data_unmount);
    M5.Display.setTextColor(WHITE);
    canvas.createSprite(MENU_WIDTH, MENU_HEIGHT);
    canvas.setFont(&fonts::FreeSerifBold9pt7b);
    canvas.setTextColor(WHITE);
    canvas.setTextDatum(top_left);

    detail_canvas.setColorDepth(1);  // mono color
    detail_canvas.createSprite(MENU_WIDTH, MENU_HEIGHT);
    detail_canvas.setTextSize(1);
    detail_canvas.setTextScroll(true);
}

void createMenu() {
    menu           = new SMOOTH_MENU::Simple_Menu(MENU_WIDTH, MENU_HEIGHT);
    menu_render_cb = new CustomMenuCallback(&canvas);

    menu->setRenderCallback(menu_render_cb);

    // Set selector anim, in this launcher case, is the icon's moving anim
    // (fixed selector)
    auto cfg_selector       = menu->getSelector()->config();
    cfg_selector.animPath_x = LVGL::ease_out;
    cfg_selector.animTime_x = 300;
    cfg_selector.animTime_y = 300;
    menu->getSelector()->config(cfg_selector);

    // Set menu open anim
    auto cfg_menu          = menu->getMenu()->config();
    cfg_menu.animPath_open = LVGL::ease_out;
    cfg_menu.animTime_open = 300;
    menu->getMenu()->config(cfg_menu);

    menu->setMenuLoopMode(true);

    menu->setFirstItem(0);

    menuAnim = &((CustomMenuCallback*)menu_render_cb)->menuAnim;
    pickAnim = &((CustomMenuCallback*)menu_render_cb)->pickAnim;

    menuAnim->setAnim(LVGL::ease_out, 40, 0, 300);
    pickAnim->setAnim(LVGL::ease_out, 0, 320, 300);
}

void view_file(String path) {
    detail_canvas.clear();
    detail_canvas.setCursor(0, 0);
    M5.Display.pushImage(0, 0, 320, 240, image_data_detail);
    M5.Display.drawString(path, 60, 15, &fonts::FreeSerifBold12pt7b);
    File32 file = fatfs.open("/" + path);
    while (file.available()) {
        detail_canvas.printf("%c", file.read());
        detail_canvas.pushSprite(0, 55);
        M5.update();
        if (M5.BtnB.wasPressed()) {
            file.close();
            M5.Display.pushImage(0, 0, 320, 240, image_data_mounted);
            return;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    file.close();
    while (1) {
        M5.update();
        if (M5.BtnB.wasPressed()) {
            M5.Display.pushImage(0, 0, 320, 240, image_data_mounted);
            return;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void list_dir() {
    File32 root = fatfs.open("/");
    File32 file;
    file_list.clear();
    while (file.openNext(&root, O_RDONLY)) {
        file_info_t f;
        char name_buf[256] = {0};
        if (file.getName(name_buf, 256)) {
            f.isDir = false;
            f.size  = file.fileSize();
            f.name  = String(name_buf);
        } else {
            continue;
        }
        file.printName(&Serial);
        if (file.isDir()) {
            f.isDir = true;
            // Indicate a directory.
            f.name += "/";
            Serial.write('/');
        }
        Serial.println();
        file.close();
        file_list.push_back(f);
    }

    std::vector<SMOOTH_MENU::Item_t*> List = menu->getMenu()->getItemList();
    menu->getMenu()->clearAllItem();
    for (int i = 0; i < file_list.size(); i++) {
        menu->getMenu()->addItem(file_list[i].name.c_str(), BASE_X_POS,
                                 BASE_Y_POS * i, MENU_WIDTH, BASE_ITEM_HEIGHT,
                                 &file_list[i]);
    }

    menuAnim->resetTime(millis());
    pickAnim->resetTime(millis());
}