// Example file - Public Domain
// Need help? https://tinyurl.com/bluepad32-help
extern "C" {
#include <string.h>

#include <uni.h>
}
#include <driver/gpio.h>
#include <esp_timer.h>
#include <msg.h>
#include "esp_gtw.h"
#include "limero.h"
// Custom "instance"

#define BUTTON_SQUARE_MASK 0x04
#define BUTTON_CROSS_MASK 0x01
#define BUTTON_CIRCLE_MASK 0x02
#define BUTTON_TRIANGLE_MASK 0x08

#define BUTTON_LEFT_MASK 0x08
#define BUTTON_DOWN_MASK 0x02
#define BUTTON_RIGHT_MASK 0x04
#define BUTTON_UP_MASK 0x01

#define BUTTON_LEFT_SHOULDER_MASK 0x10
#define BUTTON_RIGHT_SHOULDER_MASK 0x20
#define BUTTON_LEFT_TRIGGER_MASK 0x40
#define BUTTON_RIGHT_TRIGGER_MASK 0x80

#define BUTTON_LEFT_JOYSTICK_MASK 0x100
#define BUTTON_RIGHT_JOYSTICK_MASK 0x200
#define BUTTON_SHARE_MASK 0x400

EspGtw esp_gtw;

uni_hid_device_t* hid_device = NULL;
uint8_t lightbar_red, lightbar_green, lightbar_blue;

uint32_t send_counter = 0;

bool gamepad_equal(uni_gamepad_t* gp1, uni_gamepad_t* gp2) {
    if (gp1 == NULL || gp2 == NULL) {
        return false;
    }
    if ((gp1->dpad != gp2->dpad) || (gp1->axis_x != gp2->axis_x) || (gp1->axis_y != gp2->axis_y) ||
        (gp1->axis_rx != gp2->axis_rx) || (gp1->axis_ry != gp2->axis_ry) || (gp1->buttons != gp2->buttons) ||
        (gp1->misc_buttons != gp2->misc_buttons)) {
        return false;
    }
    return true;
}
// TODO

#define GPIO_LED GPIO_NUM_2

void led_toggle() {
    static bool led_state = false;
    static bool led_initialized = false;
    if (!led_initialized) {
        led_initialized = true;
        gpio_reset_pin(GPIO_LED);
        gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);
    };
    led_state = !led_state;
    gpio_set_level(GPIO_LED, led_state);
}

Ps4Info* gamepad_to_output(uni_gamepad_t* gp) {
    Ps4Info* ps4_output = new Ps4Info();
    ps4_output->button_left = gp->dpad & BUTTON_LEFT_MASK;
    ps4_output->button_right = gp->dpad & BUTTON_RIGHT_MASK;
    ps4_output->button_up = gp->dpad & BUTTON_UP_MASK;
    ps4_output->button_down = gp->dpad & BUTTON_DOWN_MASK;

    ps4_output->button_square = gp->buttons & BUTTON_SQUARE_MASK;
    ps4_output->button_cross = gp->buttons & BUTTON_CROSS_MASK;
    ps4_output->button_circle = gp->buttons & BUTTON_CIRCLE_MASK;
    ps4_output->button_triangle = gp->buttons & BUTTON_TRIANGLE_MASK;

    ps4_output->button_left_sholder = gp->buttons & BUTTON_LEFT_SHOULDER_MASK;
    ps4_output->button_right_sholder = gp->buttons & BUTTON_RIGHT_SHOULDER_MASK;
    ps4_output->button_left_trigger = gp->buttons & BUTTON_LEFT_TRIGGER_MASK;
    ps4_output->button_right_trigger = gp->buttons & BUTTON_RIGHT_TRIGGER_MASK;

    ps4_output->button_left_joystick = gp->buttons & BUTTON_LEFT_JOYSTICK_MASK;
    ps4_output->button_right_joystick = gp->buttons & BUTTON_RIGHT_JOYSTICK_MASK;

    ps4_output->button_share = gp->buttons & BUTTON_SHARE_MASK;
    ps4_output->axis_lx = gp->axis_x >> 2;
    ps4_output->axis_ly = gp->axis_y >> 2;
    ps4_output->axis_rx = gp->axis_rx >> 2;
    ps4_output->axis_ry = gp->axis_ry >> 2;
    ps4_output->gyro_x = gp->gyro[0];
    ps4_output->gyro_y = gp->gyro[1];
    ps4_output->gyro_z = gp->gyro[2];
    ps4_output->accel_x = gp->accel[0];
    ps4_output->accel_y = gp->accel[1];
    ps4_output->accel_z = gp->accel[2];

    return ps4_output;
}

void send_event(Ps4Event event, uni_gamepad_t* gp) {
    {
        static uni_gamepad_t prev_gamepad;
        static int64_t prev_send = 0;
        // -12 to dropy accel and gyro in comparison
        // send at least every 100 msec
        if ((event == Ps4Event::Data && gamepad_equal(&prev_gamepad, gp)) &&
            (esp_timer_get_time() - prev_send) < 100000) {
            return;
        }
        prev_send = esp_timer_get_time();
        if (gp != NULL)
            memcpy(&prev_gamepad, gp, sizeof(uni_gamepad_t));
        led_toggle();
    }

    Ps4Info* ps4_output = NULL;

    if (event == Ps4Event::Data && gp != NULL) {
        INFO("Sending data event\n");
        ps4_output = gamepad_to_output(gp);
    } else if (event == Ps4Event::Disconnected) {
        Ps4Info* ps4_output = new Ps4Info();
        ps4_output->connected = true;
    } else if (event == Ps4Event::Connected) {
        Ps4Info* ps4_output = new Ps4Info();
        ps4_output->connected = false;
    } else
        return;

    Result<Bytes> res = Ps4Info::json_serialize(*ps4_output);
    delete ps4_output;
    if (res.is_err()) {
        return;
    }
    Bytes encoded = res.unwrap();
    auto v = esp_gtw.send(encoded.data(), encoded.size());
}

typedef struct my_platform_instance_s {
    uni_gamepad_seat_t gamepad_seat;  // which "seat" is being used
} my_platform_instance_t;

// Declarations
extern "C" void trigger_event_on_gamepad(uni_hid_device_t* d);
extern "C" my_platform_instance_t* get_my_platform_instance(uni_hid_device_t* d);

//
// Platform Overrides
//
extern "C" void my_platform_init(int argc, const char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("custom: init()\n");
    auto v = esp_gtw.init();
    auto v1 = esp_gtw.set_callback_receive([](const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
        if (hid_device == NULL) {
            return;
        }
        Result<Ps4Cmd*> res = Ps4Cmd::json_deserialize(Bytes(data, data + len));
        if (res.is_err()) {
            return;
        }
        Ps4Cmd* ps4_cmd = res.unwrap();
        if (ps4_cmd->rumble) {
            uint8_t rumble = *ps4_cmd->rumble;
            if (hid_device->report_parser.play_dual_rumble != NULL)
                hid_device->report_parser.play_dual_rumble(hid_device, 0, 250, rumble, rumble);
        };
        if (ps4_cmd->led_rgb) {
            uint32_t led = *ps4_cmd->led_rgb;
            if (hid_device->report_parser.set_lightbar_color != NULL) {
                uint8_t r = (led & 0xff0000) >> 16;
                uint8_t g = (led & 0x00ff00) >> 8;
                uint8_t b = (led & 0x0000ff);
                hid_device->report_parser.set_lightbar_color(hid_device, r, g, b);
            }
        };
        delete ps4_cmd;
    });

#if 0
    uni_gamepad_mappings_t mappings = GAMEPAD_DEFAULT_MAPPINGS;

    // Inverted axis with inverted Y in RY.
    mappings.axis_x = UNI_GAMEPAD_MAPPINGS_AXIS_RX;
    mappings.axis_y = UNI_GAMEPAD_MAPPINGS_AXIS_RY;
    mappings.axis_ry_inverted = true;
    mappings.axis_rx = UNI_GAMEPAD_MAPPINGS_AXIS_X;
    mappings.axis_ry = UNI_GAMEPAD_MAPPINGS_AXIS_Y;

    // Invert A & B
    mappings.button_a = UNI_GAMEPAD_MAPPINGS_BUTTON_B;
    mappings.button_b = UNI_GAMEPAD_MAPPINGS_BUTTON_A;

    uni_gamepad_set_mappings(&mappings);
#endif
    //    uni_bt_service_set_enabled(true);
}

extern "C" void my_platform_on_init_complete(void) {
    logi("custom: on_init_complete()\n");

    // Safe to call "unsafe" functions since they are called from BT thread

    // Start scanning
    uni_bt_enable_new_connections_unsafe(true);

    // Based on runtime condition, you can delete or list the stored BT keys.
    if (1)
        uni_bt_del_keys_unsafe();
    else
        uni_bt_list_keys_unsafe();
}

extern "C" uni_error_t my_platform_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    // You can filter discovered devices here.
    // Just return any value different from UNI_ERROR_SUCCESS;
    // @param addr: the Bluetooth address
    // @param name: could be NULL, could be zero-length, or might contain the name.
    // @param cod: Class of Device. See "uni_bt_defines.h" for possible values.
    // @param rssi: Received Signal Strength Indicator (RSSI) measured in dBms. The higher (255) the better.

    // As an example, if you want to filter out keyboards, do:
    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) == UNI_BT_COD_MINOR_KEYBOARD) {
        logi("Ignoring keyboard\n");
        return UNI_ERROR_IGNORE_DEVICE;
    }
    auto v = esp_gtw.send((uint8_t*)name, strlen(name));

    return UNI_ERROR_SUCCESS;
}

extern "C" void my_platform_on_device_connected(uni_hid_device_t* d) {
    logi("custom: device connected: %p\n", d);
    hid_device = d;
    //  send_event(Ps4Event::Connected, NULL);
}

extern "C" void my_platform_on_device_disconnected(uni_hid_device_t* d) {
    logi("custom: device disconnected: %p\n", d);
    hid_device = NULL;
    send_event(Ps4Event::Disconnected, NULL);
}

extern "C" uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    logi("custom: device ready: %p\n", d);
    my_platform_instance_t* ins = get_my_platform_instance(d);
    ins->gamepad_seat = GAMEPAD_SEAT_A;

    trigger_event_on_gamepad(d);
    auto v = esp_gtw.send((uint8_t*)"Ready", 5);
    return UNI_ERROR_SUCCESS;
}

extern "C" void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    uint8_t leds = 0;
    uint8_t enabled = true;
    uni_gamepad_t* gp;
    std::vector<uint8_t> bytes;
    hid_device = d;

    switch (ctl->klass) {
        case UNI_CONTROLLER_CLASS_GAMEPAD: {
            gp = &ctl->gamepad;
            send_event(Ps4Event::Data, gp);
            // Debugging
            // Axis ry: control rumble
            if ((gp->buttons & BUTTON_A) && d->report_parser.play_dual_rumble != NULL) {
                d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 250 /* duration ms */,
                                                  255 /* weak magnitude */, 0 /* strong magnitude */);
            }
            // Buttons: Control LEDs On/Off
            if ((gp->buttons & BUTTON_B) && d->report_parser.set_player_leds != NULL) {
                d->report_parser.set_player_leds(d, leds++ & 0x0f);
            }
            // Axis: control RGB color
            if ((gp->buttons & BUTTON_X) && d->report_parser.set_lightbar_color != NULL) {
                uint8_t r = (gp->axis_x * 256) / 512;
                uint8_t g = (gp->axis_y * 256) / 512;
                uint8_t b = (gp->axis_rx * 256) / 512;
                d->report_parser.set_lightbar_color(d, r, g, b);
            }

            // Toggle Bluetooth connections
            if ((gp->buttons & BUTTON_SHOULDER_L) && enabled) {
                logi("*** Disabling Bluetooth connections\n");
                uni_bt_enable_new_connections_safe(false);
                enabled = false;
            }
            if ((gp->buttons & BUTTON_SHOULDER_R) && !enabled) {
                logi("*** Enabling Bluetooth connections\n");
                uni_bt_enable_new_connections_safe(true);
                enabled = true;
            }
            break;
        }
        default:
            break;
    }
}

extern "C" const uni_property_t* my_platform_get_property(uni_property_idx_t idx) {
    ARG_UNUSED(idx);
    return NULL;
}

extern "C" void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    switch (event) {
        case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON: {
            uni_hid_device_t* d = (uni_hid_device_t*)data;

            if (d == NULL) {
                loge("ERROR: my_platform_on_oob_event: Invalid NULL device\n");
                return;
            }
            logi("custom: on_device_oob_event(): %d\n", event);

            my_platform_instance_t* ins = get_my_platform_instance(d);
            ins->gamepad_seat = ins->gamepad_seat == GAMEPAD_SEAT_A ? GAMEPAD_SEAT_B : GAMEPAD_SEAT_A;

            trigger_event_on_gamepad(d);
            break;
        }

        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            logi("custom: Bluetooth enabled: %d\n", (bool)(data));
            break;

        default:
            logi("my_platform_on_oob_event: unsupported event: 0x%04x\n", event);
            break;
    }
}

//
// Helpers
//
extern "C" my_platform_instance_t* get_my_platform_instance(uni_hid_device_t* d) {
    return (my_platform_instance_t*)&d->platform_data[0];
}

extern "C" void trigger_event_on_gamepad(uni_hid_device_t* d) {
    my_platform_instance_t* ins = get_my_platform_instance(d);

    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 150 /* duration ms */, 128 /* weak magnitude */,
                                          40 /* strong magnitude */);
    }

    if (d->report_parser.set_player_leds != NULL) {
        d->report_parser.set_player_leds(d, ins->gamepad_seat);
    }

    if (d->report_parser.set_lightbar_color != NULL) {
        uint8_t red = (ins->gamepad_seat & 0x01) ? 0xff : 0;
        uint8_t green = (ins->gamepad_seat & 0x02) ? 0xff : 0;
        uint8_t blue = (ins->gamepad_seat & 0x04) ? 0xff : 0;
        d->report_parser.set_lightbar_color(d, red, green, blue);
    }
}

extern "C" void my_platform_on_gamepad_data(uni_hid_device_t* d, uni_gamepad_t* gp) {
    /*my_platform_instance_t* ins = get_my_platform_instance(d);
    ins->gamepad_seat = GAMEPAD_SEAT_A;

    trigger_event_on_gamepad(d);*/
}

//
// Entry Point
//
extern "C" struct uni_platform* get_my_platform(void) {
    static struct uni_platform plat = {/*.name =*/"custom",
                                       /*.init =  */ my_platform_init,
                                       /* .on_init_complete =  */ my_platform_on_init_complete,
                                       /* .on_device_discovered = */ my_platform_on_device_discovered,
                                       /* .on_device_connected = */ my_platform_on_device_connected,
                                       /* .on_device_disconnected = */ my_platform_on_device_disconnected,
                                       /* .on_device_ready = */ my_platform_on_device_ready,
                                       /* .on_gamepad_data = */ my_platform_on_gamepad_data,
                                       /* .on_controller_data = */ my_platform_on_controller_data,
                                       /*.get_property = */ my_platform_get_property,
                                       /* .on_oob_event = */ my_platform_on_oob_event,
                                       /* device_dump */ 0,
                                       /* register_console_cmds */ 0

    };

    return &plat;
}
