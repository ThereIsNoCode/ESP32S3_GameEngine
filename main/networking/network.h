#pragma once
#include <stdint.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_mac.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


void init_nvs();


void init_host();
void init_client();
void Network_Apply_Movement();