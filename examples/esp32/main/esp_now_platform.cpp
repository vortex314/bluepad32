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
#include "ps4.h"
// Custom "instance"

EspGtw esp_gtw;
FrameEncoder frame_encoder(200);
FrameDecoder frame_decoder(200);
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

const struct PropDescriptor {
    Ps4 id;
    const char* name;
    const char* description;
    uint8_t ValueType;
    uint8_t ValueMode;
} props[] = {
    {Ps4::DPAD, "dpad_left", "Dpad", ValueType::UINT, ValueMode::READ},

    {Ps4::BUTTON_SQUARE, "button_square", "Square button", ValueType::UINT, ValueMode::READ},
    {Ps4::BUTTON_CROSS, "button_cross", "Cross button", ValueType::UINT, ValueMode::READ},
    {Ps4::BUTTON_CIRCLE, "button_circle", "Circle button", ValueType::UINT, ValueMode::READ},
    {Ps4::BUTTON_TRIANGLE, "button_triangle", "Triangle button", ValueType::UINT, ValueMode::READ},

    {Ps4::BUTTON_LEFT_SHOULDER, "button_left_shoulder", "Left Shoulder button", ValueType::UINT, ValueMode::READ},
    {Ps4::BUTTON_RIGHT_SHOULDER, "button_right_shoulder", "Right Shoulder button", ValueType::UINT, ValueMode::READ},
    {Ps4::BUTTON_LEFT_TRIGGER, "button_left_trigger", "Left Trigger button", ValueType::UINT, ValueMode::READ},
    {Ps4::BUTTON_RIGHT_TRIGGER, "button_right_trigger", "Right Trigger button", ValueType::UINT, ValueMode::READ},
    {Ps4::BUTTON_LEFT_JOYSTICK, "button_left_joystick", "Left Joystick button", ValueType::UINT, ValueMode::READ},
    {Ps4::BUTTON_RIGHT_JOYSTICK, "button_right_joystick", "Right Joystick button", ValueType::UINT, ValueMode::READ},
    {Ps4::BUTTON_SHARE, "button_share", "Share button", ValueType::UINT, ValueMode::READ},

    {Ps4::STICK_LEFT_X, "axis_x", "Left Stick X", ValueType::INT, ValueMode::READ},
    {Ps4::STICK_LEFT_Y, "axis_y", "Left Stick Y", ValueType::INT, ValueMode::READ},
    {Ps4::STICK_RIGHT_X, "axis_rx", "Right Stick X", ValueType::INT, ValueMode::READ},
    {Ps4::STICK_RIGHT_Y, "axis_ry", "Right Stick Y", ValueType::INT, ValueMode::READ},

    {Ps4::GYRO_X, "gyro_x", "Gyro X axis", ValueType::INT, ValueMode::READ},
    {Ps4::GYRO_Y, "gyro_y", "Gyro Y axis", ValueType::INT, ValueMode::READ},
    {Ps4::GYRO_Z, "gyro_z", "Gyro Z axis", ValueType::INT, ValueMode::READ},

    {Ps4::ACCEL_X, "accel_x", "Accelerometer X Axis ", ValueType::INT, ValueMode::READ},
    {Ps4::ACCEL_Y, "accel_y", "Accelerometer Y Axis ", ValueType::INT, ValueMode::READ},
    {Ps4::ACCEL_Z, "accel_z", "Accelerometer Z Axis ", ValueType::INT, ValueMode::READ},

    {Ps4::RUMBLE, "rumble", "Rumble", ValueType::UINT, ValueMode::WRITE},
    {Ps4::LED_GREEN, "led_green", "Green LED", ValueType::UINT, ValueMode::WRITE},
    {Ps4::LED_RED, "led_red", "Red LED", ValueType::UINT, ValueMode::WRITE},
    {Ps4::LED_BLUE, "led_blue", "Blue LED", ValueType::UINT, ValueMode::WRITE},
};

const struct MsgHeader desc_msg_header = {.dst = Option<uint32_t>::None(),
                                          .src = Option<uint32_t>::Some(FNV("ps4")),
                                          .msg_type = MsgType::Info,
                                          .ret_code = Option<uint32_t>::None(),
                                          .msg_id = Option<uint16_t>::None(),
                                          .qos = Option<uint8_t>::None()};

void send_obj_desc() {
    frame_encoder.clear();

    frame_encoder.begin_map();
    desc_msg_header.encode(frame_encoder);
    frame_encoder.end_map();

    frame_encoder.begin_map();
    frame_encoder.add_map(InfoPropertyId::PROP_ID, -1);
    frame_encoder.encode_int32(InfoPropertyId::NAME);
    frame_encoder.encode_str("ps4");  // name
    frame_encoder.encode_int32(InfoPropertyId::DESCRIPTION);
    frame_encoder.encode_str("PS4 Controller");  // description
    frame_encoder.end_map();

    esp_gtw.send(frame_encoder.data(), frame_encoder.size());
}

void send_prop_desc(uint32_t idx) {
    frame_encoder.clear();

    frame_encoder.begin_map();
    desc_msg_header.encode(frame_encoder);
    frame_encoder.end_map();

    frame_encoder.begin_map();
    frame_encoder.encode_int32(InfoPropertyId::PROP_ID);
    frame_encoder.encode_uint32(props[prop_counter].id);
    frame_encoder.encode_int32(InfoPropertyId::NAME);
    frame_encoder.encode_str(props[prop_counter].name);
    frame_encoder.encode_int32(InfoPropertyId::DESCRIPTION);
    frame_encoder.encode_str(props[prop_counter].description);
    frame_encoder.encode_int32(InfoPropertyId::TYPE);  // ValueType
    frame_encoder.encode_uint32(props[prop_counter].ValueType);
    frame_encoder.encode_int32(InfoPropertyId::MODE);  // ValueMode
    frame_encoder.encode_uint32(props[prop_counter].ValueMode);
    frame_encoder.end_map();

    esp_gtw.send(frame_encoder.data(), frame_encoder.size());
}

const MsgHeader pub_header = {
    .dst = Option<uint32_t>::None(),
    .src = Option<uint32_t>::Some(FNV("ps4")),
    .msg_type = MsgType::Pub,
    .ret_code = Option<uint32_t>::None(),
    .msg_id = Option<uint16_t>::None(),
    .qos = Option<uint8_t>::None(),
};

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
    frame_encoder.clear();
    frame_encoder.begin_map();
    pub_header.encode(frame_encoder);
    frame_encoder.end_map();

    if (event == Ps4Event::Data && gp != NULL) {
        frame_encoder.begin_map();
        frame_encoder.add_map(Ps4::DPAD, gp->dpad);

        frame_encoder.add_map(Ps4::BUTTON_SQUARE, gp->buttons & 0x4 ? 1 : 0);
        frame_encoder.add_map(Ps4::BUTTON_CROSS, gp->buttons & 0x1 ? 1 : 0);
        frame_encoder.add_map(Ps4::BUTTON_CIRCLE, gp->buttons & 0x2 ? 1 : 0);
        frame_encoder.add_map(Ps4::BUTTON_TRIANGLE, gp->buttons & 0x8 ? 1 : 0);

        frame_encoder.add_map(Ps4::BUTTON_LEFT_SHOULDER, gp->misc_buttons & 0x1);
        frame_encoder.add_map(Ps4::BUTTON_RIGHT_SHOULDER, gp->misc_buttons & 0x2 ? 1 : 0);
        frame_encoder.add_map(Ps4::BUTTON_LEFT_TRIGGER, gp->misc_buttons & 0x4 ? 1 : 0);
        frame_encoder.add_map(Ps4::BUTTON_RIGHT_TRIGGER, gp->misc_buttons & 0x8 ? 1 : 0);

        frame_encoder.add_map(Ps4::BUTTON_LEFT_JOYSTICK, gp->buttons & 0x100 ? 1 : 0);
        frame_encoder.add_map(Ps4::BUTTON_RIGHT_JOYSTICK, gp->buttons & 0x200 ? 1 : 0);
        frame_encoder.add_map(Ps4::BUTTON_SHARE, gp->buttons & 0x400 ? 1 : 0);

        frame_encoder.add_map(Ps4::STICK_LEFT_X, gp->axis_x >> 2);
        frame_encoder.add_map(Ps4::STICK_LEFT_Y, gp->axis_y >> 2);
        frame_encoder.add_map(Ps4::STICK_RIGHT_X, gp->axis_rx >> 2);
        frame_encoder.add_map(Ps4::STICK_RIGHT_Y, gp->axis_ry >> 2);

        frame_encoder.add_map(Ps4::GYRO_X, gp->gyro[0]);
        frame_encoder.add_map(Ps4::GYRO_Y, gp->gyro[1]);
        frame_encoder.add_map(Ps4::GYRO_Z, gp->gyro[2]);

        frame_encoder.add_map(Ps4::ACCEL_X, gp->accel[0]);
        frame_encoder.add_map(Ps4::ACCEL_Y, gp->accel[1]);
        frame_encoder.add_map(Ps4::ACCEL_Z, gp->accel[2]);
        frame_encoder.end_map();
    } else if (event == Ps4Event::Disconnected) {
        frame_encoder.begin_map();
        frame_encoder.encode_int32(Ps4::CONNECTED);
        frame_encoder.encode_bool(false);
        frame_encoder.end_map();
    } else if (event == Ps4Event::Connected) {
        frame_encoder.begin_map();
        frame_encoder.encode_int32(Ps4::CONNECTED);
        frame_encoder.encode_bool(true);
        frame_encoder.end_map();
    } else
        return;
    // send props
    esp_gtw.send(frame_encoder.data(), frame_encoder.size());

    if (send_counter++ % 10 == 0) {
        // send object and prop description
        send_obj_desc();
        static int prop_counter = 0;
        send_prop_desc(prop_counter++);
        if (prop_counter == sizeof(props) / sizeof(PropDescriptor)) {
            prop_counter = 0;
        }
    }
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
    esp_gtw.init();
    esp_gtw.set_callback_receive([](const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
        if (hid_device == NULL) {
            return;
        }
        MsgHeader hdr;
        frame_decoder.clear();
        frame_decoder.fill_buffer(data, len);
        hdr.decode(frame_decoder);
        if (hdr.dst.is_some() && hdr.dst.unwrap() != FNV("ps4")) {
            return;
        }
        Ps4Map ps4_msg;
        if (ps4_msg.decode(frame_decoder).is_err()) {
            return;
        }
        ps4_msg.rumble.inspect([&](rumble) {
            if (hid->device->report_parser.play_dual_rumble != NULL)
                hid_device->report_parser.play_dual_rumble(hid_device, 0, 250, rumble, rumble);
        });
        ps4_msg.lightbar_rgb.inspect([&](led) {
            if (led_update && hid->device->report_parser.set_lightbar_color != NULL){
                uint8_t r = (led & 0xff0000) >> 16;
                uint8_t g = (led & 0x00ff00) >> 8;
                uint8_t b = (led & 0x0000ff);
               hid_device->report_parser.set_lightbar_color(hid_device, r,g,b);
            }
        });
        
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
    esp_gtw.send((uint8_t*)name, strlen(name));

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
    esp_gtw.send((uint8_t*)"Ready", 5);
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
