#include "network.h"
#include "packets.h"
#include "../entities/player.h"
#include "../entities/bomb.h"
#include "../drivers/esp_family/ESP32-S3.h"
#include <fcntl.h>   // for fcntl / O_NONBLOCK — add to your inclu
#define WIFI_SSID  "AOWKESP"
#define WIFI_PASS  "1@#$%ALMOPROBALY1@#$%Z"
#define WIFI_CHANNEL  1
#define WIFI_MAX_STATION_CONN  4


#if CONFIG_ESP_GTK_REKEYING_ENABLE
#define EXAMPLE_GTK_REKEY_INTERVAL CONFIG_ESP_GTK_REKEY_INTERVAL
#else
#define EXAMPLE_GTK_REKEY_INTERVAL 0
#endif

#define HOST_IP_ADDR "192.168.4.1"

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(DISPLAY_TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(DISPLAY_TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}
#define MAX_RETRY 10
static int s_retry_num = 0;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static EventGroupHandle_t s_wifi_event_group;
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(DISPLAY_TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(DISPLAY_TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(DISPLAY_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void init_nvs(){
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(DISPLAY_TAG, "ESP_WIFI_MODE_AP");
}



typedef struct input_packet_t {
    uint32_t seq;
    uint16_t axis_x;
    uint16_t axis_y;
    uint8_t buttons;
} input_packet_t;


#define NETWORK_UDP_PORT 3333


packet_clientInput_t localInputPacket;
//you can make this into an array of # of players, where index = player ID and value is packet for input
packet_clientInput_t cachedInputPacket;
packet_clientPosition_t cachedPositionPacket;
//Once we want more players, loop through all cached packets and compute them
void Network_Apply_Movement(){

    localInputPacket.playerId = player_Current->id;
    localInputPacket.axis_X = joystick_X;
    localInputPacket.axis_Y = joystick_Y;
    uint8_t buttons =
        ((INPUT_JUMP     ? 1 : 0) << 0) |
        ((INPUT_INTERACT ? 1 : 0) << 1);   // add more bits as needed
    localInputPacket.buttons = buttons;


    Player_Apply_Movement(&localInputPacket);

    Player_Apply_Movement(&cachedInputPacket);
    //Player_InterpolateRemote(&cachedPositionPacket);
}

//Occurs only in client
void apply_snapshot(packet_serverSnapshot_t *snap){
    
    for(int i = 0; i < 2; i++){
        playerArr[i].position_x = snap->players[i].position_x;
        playerArr[i].position_y = snap->players[i].position_y;
    }
    for(int i = 0; i < 5; i++){
        bombArr[i].position_x = snap->bombs[i].position_x;
        bombArr[i].position_y = snap->bombs[i].position_y;
    }
}
#define tick_speed 33 //originally 33
// ---------------------------------------------------------------------------
// SERVER
// ---------------------------------------------------------------------------
static void udp_server_task(void *pvParameters)
{
    uint8_t rx_buffer[128];
    struct sockaddr_in6 dest_addr;

    // Per-client tracking (single client for now; index by player later)
    struct sockaddr_storage client_addr;
    socklen_t              client_addr_len = 0;
    bool                   have_client = false;
    uint32_t               last_seq_from_client = 0;

    while (1) {   // OUTER: own the socket's lifecycle
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family      = AF_INET;
        dest_addr_ip4->sin_port        = htons(NETWORK_UDP_PORT);

        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE(DISPLAY_TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        if (bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            ESP_LOGE(DISPLAY_TAG, "Socket unable to bind: errno %d", errno);
            close(sock);
            continue;
        }
        ESP_LOGI(DISPLAY_TAG, "Socket bound, port %d", NETWORK_UDP_PORT);

        // Non-blocking: recvfrom returns immediately (EAGAIN) when nothing waiting
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        TickType_t last_wake = xTaskGetTickCount();
        const TickType_t tick = pdMS_TO_TICKS(tick_speed);   // 30 Hz server tick

        while (1) {   // INNER: drain inputs, simulate is elsewhere, send snapshots


            // 1. DRAIN all pending packets this tick (never blocks)
            for (;;) {
                struct sockaddr_storage source_addr;
                socklen_t socklen = sizeof(source_addr);

                int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0,
                                   (struct sockaddr *)&source_addr, &socklen);
                if (len < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // nothing left
                    ESP_LOGE(DISPLAY_TAG, "recvfrom failed: errno %d", errno);
                    goto rebuild;   // real error -> rebuild socket
                }
                if (len < 1) continue;

                uint8_t requestType = rx_buffer[0];
                switch (requestType) {
                    case MSG_SERVER_JOIN: {
                        // Remember who this client is so we can push snapshots to them
                        memcpy(&client_addr, &source_addr, socklen);
                        client_addr_len = socklen;
                        have_client = true;

                        packet_assignRequest_t assign = {
                            .requestType = MSG_CLIENT_ASSIGN,
                            .player_id   = 1,       // TODO: real slot assignment
                        };
                        sendto(sock, &assign, sizeof(assign), 0,
                               (struct sockaddr *)&source_addr, socklen);
                        break;
                    }
                    case MSG_SERVER_INPUT: {
                        //if (len != sizeof(packet_clientInput_t)) break;
                        if (len != sizeof(packet_clientInput_t)) break;

                        //If I want to continue server authorative, uncomment below memcpy
                        //memcpy(&cachedInputPacket, rx_buffer, sizeof(cachedInputPacket));
            
                        memcpy(&cachedInputPacket, rx_buffer, sizeof(cachedInputPacket));

                        //cachedPositionPacket
                        //Player_DirectSetPosition(&inputPacket);

                        // (optional) remember client for snapshots if not via join
                        memcpy(&client_addr, &source_addr, socklen);
                        client_addr_len = socklen;
                        have_client = true;
            
                        break;
                    }
                }
            }

            // 2. SEND a snapshot to the known client, once per tick
            if (have_client) {
                
                // Build your snapshot from current authoritative game state.
                // Placeholder shape — replace with your real snapshot_packet_t:
                //
                // snapshot_packet_t snap = { .requestType = MSG_CLIENT_SNAPSHOT, ... };
                // fill_snapshot(&snap);
                // sendto(sock, &snap, sizeof(snap), 0,
                //        (struct sockaddr *)&client_addr, client_addr_len);

                packet_serverSnapshot_t snapshotPacket = {
                    .requestType = MSG_CLIENT_SNAPSHOT,
                };

                for(int i = 0; i < 2; i++){
                    snapshotPacket.players[i].position_x = playerArr[i].position_x;
                    snapshotPacket.players[i].position_y = playerArr[i].position_y;
                }
                for(int i = 0; i < 5; i++){
                    snapshotPacket.bombs[i].position_x = bombArr[i].position_x;
                    snapshotPacket.bombs[i].position_y = bombArr[i].position_y;
                }
                sendto(sock, &snapshotPacket, sizeof(snapshotPacket), 0,
                    (struct sockaddr *)&client_addr, client_addr_len);   // A;waus remember fromt host to client, use client_addr


                localInputPacket.requestType = MSG_CLIENT_INPUT;
                sendto(sock, &localInputPacket, sizeof(localInputPacket), 0,
                    (struct sockaddr *)&client_addr, client_addr_len);   // <-- the real client

            }

            // 3. Steady tick, independent of network timing
            xTaskDelayUntil(&last_wake, tick);
        }

    rebuild:
        shutdown(sock, 0);
        close(sock);
    }

    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// CLIENT
// ---------------------------------------------------------------------------


static void udp_client_task(void *pvParameters)
{
    uint8_t rx_buffer[128];

    while (1) {   // OUTER: build socket, rebuild if it breaks
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        dest_addr.sin_family      = AF_INET;
        dest_addr.sin_port        = htons(NETWORK_UDP_PORT);

        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) { ESP_LOGE(DISPLAY_TAG, "socket: errno %d", errno); break; }

        // Non-blocking so the receive never gates the send rate
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        bool     joined       = false;
        uint8_t  my_player_id = 0;

        TickType_t last_wake = xTaskGetTickCount();
        const TickType_t tick = pdMS_TO_TICKS(tick_speed);   // 30 Hz send rate

        while (1) {   // INNER
            // 1. SEND every tick
            if (!joined) {
                packet_joinRequest_t join = { .requestType = MSG_SERVER_JOIN };
                sendto(sock, &join, sizeof(join), 0,
                       (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            } else {
                
                uint8_t buttons =
                    ((INPUT_JUMP     ? 1 : 0) << 0) |
                    ((INPUT_INTERACT ? 1 : 0) << 1);   // add more bits as needed

                packet_clientInput_t inputPacket = {
                    .requestType = MSG_SERVER_INPUT,
                    .playerId    = my_player_id,
                    .axis_X      = joystick_X,//Read_Joystick_X(), ///////////////////////////////////////////////////
                    .axis_Y      = joystick_Y,//Read_Joystick_Y(),
                    .buttons     = buttons,
                };

                // packet_clientPosition_t inputPacket = {
                //     .requestType = MSG_SERVER_INPUT,
                //     .playerId    = my_player_id,
                //     .position_X      = player_Current->position_x,
                //     .position_Y      = player_Current->position_y,
                // };
                //ESP_LOGI(DISPLAY_TAG, "SENDING POSITION");
                sendto(sock, &inputPacket, sizeof(inputPacket), 0,
                       (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            }
            
            // 2. DRAIN all pending replies without blocking
            for (;;) {
                
                struct sockaddr_storage source_addr;
                socklen_t socklen = sizeof(source_addr);

                int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0,
                                   (struct sockaddr *)&source_addr, &socklen);

                
                if (len < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    ESP_LOGE(DISPLAY_TAG, "recvfrom: errno %d", errno);
                    goto rebuild;
                }
                
                if (len < 1) continue;
                //ESP_LOGI(DISPLAY_TAG, "client rx: type=%d", rx_buffer[0]);
                uint8_t type = rx_buffer[0];
                switch (type) {
                    case MSG_CLIENT_ASSIGN: {
                        if (len == sizeof(packet_assignRequest_t)) {
                            packet_assignRequest_t a;
                            memcpy(&a, rx_buffer, sizeof(a));
                            my_player_id = a.player_id;
                            assign_player(my_player_id);   // use the real id, not 1
                            joined = true;
                            ESP_LOGI(DISPLAY_TAG, "assigned player %d", my_player_id);
                        }
                        break;
                    }
                    case MSG_CLIENT_SNAPSHOT: {
                        
                        if (len == sizeof(packet_serverSnapshot_t)) {
                           
                            packet_serverSnapshot_t snap;
                            memcpy(&snap, rx_buffer, sizeof(snap));
                            apply_snapshot(&snap);
                        }
                        break;
                    }
                    case MSG_CLIENT_INPUT:{

                        if (len == sizeof(packet_clientInput_t)) {
                            memcpy(&cachedInputPacket, rx_buffer, sizeof(cachedInputPacket));
                        }  
                        break; 
                    }
                }
            }

            // 3. Steady tick — send rate no longer depends on replies
            xTaskDelayUntil(&last_wake, tick);
        }

    rebuild:
        shutdown(sock, 0);
        close(sock);
    }

    vTaskDelete(NULL);
}


void init_host(){

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));


    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID, 
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL, 
            .password = WIFI_PASS,
            .max_connection = WIFI_MAX_STATION_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = true,
            },
            .gtk_rekey_interval = EXAMPLE_GTK_REKEY_INTERVAL,
            .ssid_hidden = 0
        }
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);

}

void init_client(){
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_OPEN,
            .sae_pwe_h2e = WPA3_SAE_PWE_HUNT_AND_PECK,
            .sae_h2e_identifier = "",
            .disable_wpa3_compatible_mode = 0
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(DISPLAY_TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(DISPLAY_TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(DISPLAY_TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(DISPLAY_TAG, "UNEXPECTED EVENT");
    }

    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);

}