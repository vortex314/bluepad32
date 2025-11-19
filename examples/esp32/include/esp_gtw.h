#include "log.h"
#include "errno.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "stdint.h"
#include <result.h>

//const uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

class EspGtw {
   public:
    Result<bool> init();
    Result<bool> set_callback_receive(void (*func)(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len));
    Result<bool> set_pmk();
    Result<bool> add_peer();
    Result<bool> send(const uint8_t* data, int len);
    Result<bool> deinit();
};

typedef enum Ps4Event { Connected = 0, Disconnected, Data, OOB } Ps4Event;
