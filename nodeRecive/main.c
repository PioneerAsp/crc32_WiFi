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

static const char *TAG = "SMARTWATCH: ";
float self_test[6] = {0, 0, 0, 0, 0, 0};
float accel_bias[3] = {0, 0, 0};
float gyro_bias[3] = {0, 0, 0};

//Apuntador global
    NODE_Package *pkgs1, *pkgs2;

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

static uint8_t peer_mac[ESP_NOW_ETH_ALEN] = {0x40, 0x22, 0xd8, 0x4f, 0x4b, 0x58}; //  AP (40:91:51:bf:f5:94)
                                                                                  //  STA  (40:22:d8:ee:6d:a5)
static esp_err_t init_wifi(void)
{
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&wifi_init_config);
    esp_netif_init();
    esp_event_loop_create_default();
    nvs_flash_init();
    esp_wifi_init(&wifi_init_config);
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    esp_wifi_start();

    ESP_LOGI(TAG, "wifi init completed");
    return ESP_OK;
}

void recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
    ////Convertimos uint8_t * -> str* para poder usar la funcion de StringToPackage
    char *message = (  char *)data;
    
    //En la funcion recv_cb Recivo el string y lo convierto a paquete
    StringToPackage(pkgs1, message);

    //Revisamos si llego correctamente
    if(checkCrc32(pkgs1->crc32, message)==1) ESP_LOGI(TAG, "ESP_NOW_RECIVED_SUCCESS");
    else ESP_LOGW(TAG, "ESP_NOW_RECIVED_FAIL");
    ESP_LOGI(TAG, "Data received" MACSTR " %d", MAC2STR(esp_now_info->src_addr), pkgs1->data[3]);
    
   ESP_LOGI(TAG, "Data received" MACSTR "%s", MAC2STR(esp_now_info->src_addr), data);
    
    
}

void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "ESP_NOW_SEND_SUCCESS");
    }
    else
    {
        ESP_LOGW(TAG, "ESP_NOW_SEND_FAIL");
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
    esp_now_peer_info.ifidx = ESP_IF_WIFI_AP;

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
    NODE_Package *pkgs = (NODE_Package *)malloc(sizeof(NODE_Package));
    createPackage(pkgs, 0x5A, 0x10, 0, 0, 0, 0, 0, 0xB2);
    pkgs1=pkgs;
    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(init_esp_now());
    //ESP_ERROR_CHECK(register_peer(peer_mac));
    //uint8_t temperatura[] = "Temperatura: 18 GC";
    //uint8_t humedad[] = "Humedad = 72 %";

   
}