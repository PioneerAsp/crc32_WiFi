#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "driver/i2c.h"
#include "esp_spiffs.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"


#include "string.h"
#include "stdlib.h"
#include <math.h>
#include <limits.h>
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "nvs_flash.h"

#include "packagedata.h"
#include "crc32.h"

// #include "mpu6050/mpu6050.h"

static const char *TAG = "NODE A: ";
float self_test[6] = {0, 0, 0, 0, 0, 0};
float accel_bias[3] = {0, 0, 0};
float gyro_bias[3] = {0, 0, 0};

#define PI 3.14159265358979323846f
#define AVG_BUFF_SIZE 20
#define I2C_MASTER_SCL_IO 22        /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO 21        /*!< GPIO number used for I2C master data  */
#define I2C_FREQ_HZ 100000          /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS 1000
#define SAMPLE_SIZE 2000
#define I2C_PORT I2C_NUM_0

#define ESP_CHANNEL 1

static uint8_t peer_mac[ESP_NOW_ETH_ALEN] = {0x40, 0x22, 0xd8, 0x4f, 0x4b, 0x58}; //estacion AP R (40:91:51:bf:f5:94)
                                                                                  // STA  I (40:22:d8:ee:6d:a4)                 
//I (797) wifi:mode : sta (08:3a:f2:b8:9f:18)
//MAC: 08:3a:f2:b8:9f:18
static esp_err_t init_wifi(void)
{
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&wifi_init_config);
    esp_netif_init();
    esp_event_loop_create_default();
    nvs_flash_init();
    esp_wifi_init(&wifi_init_config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi init completed");
    return ESP_OK;
}

void recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
    
    ESP_LOGI(TAG, "Data received " MACSTR "%s", MAC2STR(esp_now_info->src_addr), data);
}

void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "ESP_NOW_SEND_SUCCESS");
    }
    else
    {
        ESP_LOGW(TAG, "ESP_NOW_SEN_FAIL");
    }
}

static esp_err_t init_esp_now(void)
{
    esp_now_init();
    esp_now_register_recv_cb(recv_cb);
    esp_now_register_send_cb(send_cb);

    ESP_LOGI(TAG, "esp now init completed");
    return ESP_OK;
}

static esp_err_t register_peer(uint8_t *peer_addr)
{
    esp_now_peer_info_t esp_now_peer_info = {};
    memcpy(esp_now_peer_info.peer_addr, peer_mac, ESP_NOW_ETH_ALEN);
    esp_now_peer_info.channel = ESP_CHANNEL;
    esp_now_peer_info.ifidx = ESP_IF_WIFI_STA;
    
    esp_now_add_peer(&esp_now_peer_info);
    return ESP_OK;
}
static esp_err_t esp_now_send_data(const uint8_t *peer_addr, const uint8_t *data, uint8_t len)
{
    esp_now_send(peer_addr, data, len);
    return ESP_OK;
}

void app_main(void)
{   
    /**START Creating package to send**/
        //Creo el apuntador a un paquete vacio
        NODE_Package *pkgs = (NODE_Package *)malloc(sizeof(NODE_Package));

        //Valor de ejemplo para enviar a la estacion
        uint8_t dato = 18;
        
        //Cadena auxiliar
        char msg_pack[MSG_TAM_STR];

        //Creo el paquete con contenido 
        createPackage(pkgs, 0x5B, 0x10, 1, 0, 0, 0, dato, 0xB2);

        //Convierto El paquete a Cadena
        PackageToString(pkgs, msg_pack);

        //Convertimos str* -> uint8_t * para poder usar la funcion de esp_now_send_data
        const uint8_t *message = (const uint8_t *)msg_pack;

        //Enviamos message con la funcion sig:
        //esp_now_send_data(peer_mac, message, strlen(msg_pack));
        
        
    /**END Creating package to send**/
    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(init_esp_now());
    ESP_ERROR_CHECK(register_peer(peer_mac));
    while (true)
    {
        /*Sugiero que creemos/modifiquemos el paquete cada que cambie la temperatura o cada t cantidad de segundos*/
        esp_now_send_data(peer_mac, message, strlen(msg_pack));
        showPackage(pkgs);
        vTaskDelay(pdMS_TO_TICKS(1000));

        /** Si quiero modificar el valor tendria que... **/

        //Valor de ejemplo para enviar a la estacion
        uint8_t dato = 12;

        //Creo el paquete con contenido 
        createPackage(pkgs, 0x5B, 0x10, 1, 0, 0, 0, dato, 0xB2);

        //Convierto El paquete a Cadena
        PackageToString(pkgs, msg_pack);

        //Convertimos str* -> uint8_t * para poder usar la funcion de esp_now_send_data
        const uint8_t *message = (const uint8_t *)msg_pack;
    }
}