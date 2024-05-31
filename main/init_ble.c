#ifndef __INIT_BLE_H__
#define __INIT_BLE_H__
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_bt_defs.h"

#define PROFILE_NUM 1
#define PROFILE_APP_IDX 0
#define GATTS_SERVICE_UUID 0x00FF
#define GATTS_CHAR_NUM 1


void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
        // Handle GAP events here

    default:
        break;
    }
}
struct gatts_char_inst
{
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t char_handle;
    esp_bt_uuid_t descr_uuid;
    uint16_t descr_handle;
    esp_attr_control_t control;
    esp_attr_value_t value;
} gatts_char_inst[GATTS_CHAR_NUM];

esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
}; // 设置广播参数
struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};
void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
        .char_handle = 0,
    },
};
void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    // Handle GATT Server events here
    case ESP_GATTS_REG_EVT:
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        }
        else
        {
            // ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
            //         param->reg.app_id,
            //         param->reg.status);
            printf("Reg app failed, app_id %04x, status %d\n",
                   param->reg.app_id,
                   param->reg.status);
            return;
        }

        gl_profile_tab[PROFILE_APP_IDX].service_id.is_primary = true;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.inst_id = 0x00; // 设置service_id
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.len = ESP_UUID_LEN_128;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[0] = 0x12;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[1] = 0x34;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[2] = 0x56;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[3] = 0x78;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[4] = 0x90;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[5] = 0xAB;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[6] = 0xCD;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[7] = 0xEF;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[8] = 0x12;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[9] = 0x34;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[10] = 0x56;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[11] = 0x78;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[12] = 0x90;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[13] = 0xAB;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[14] = 0xCD;
        gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid128[15] = 0xEF;

        esp_err_t ret = esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_APP_IDX].service_id, 4);
        if (ret != ESP_OK)
        {
            printf("create service failed\n");
            printf("Error: %s\n", esp_err_to_name(ret));
        }
        else
        {
            printf("create service success\n");
        }

        break;
    }
    case ESP_GATTS_CREATE_EVT:
    {
        if (param->create.status != ESP_GATT_OK)
        {
            printf("create service failed\n");
            printf("Error: %s\n", esp_err_to_name(param->create.status));
        }
        else
        {
            printf("create service success\n");
        }
        gl_profile_tab[PROFILE_APP_IDX].service_handle = param->create.service_handle;               // 获取service_handle
        gl_profile_tab[PROFILE_APP_IDX].char_uuid.len = ESP_UUID_LEN_16;                             // 设置char_uuid
        gl_profile_tab[PROFILE_APP_IDX].char_uuid.uuid.uuid16 = 0xFF03;                              // 设置char_uuid
        esp_err_t ret = esp_ble_gatts_start_service(gl_profile_tab[PROFILE_APP_IDX].service_handle); // 启动service
        if (ret != ESP_OK)
        {
            printf("start service failed\n");
            printf("Error: %s\n", esp_err_to_name(ret));
        }
        else
        {
            printf("start service success\n");
        }

        gatts_char_inst[0].char_uuid.len = ESP_UUID_LEN_16;
        gatts_char_inst[0].char_uuid.uuid.uuid16 = 0xFF01;
        gatts_char_inst[0].perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
        gatts_char_inst[0].property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
        gatts_char_inst[0].char_handle = 0;
        gatts_char_inst[0].control.auto_rsp = ESP_GATT_AUTO_RSP;
        uint8_t char_value[] = "Hello World!23452345";
        gatts_char_inst[0].value.attr_len = sizeof(char_value);
        gatts_char_inst[0].value.attr_max_len = 100;
        gatts_char_inst[0].value.attr_value = char_value;
        ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_APP_IDX].service_handle, &gatts_char_inst[0].char_uuid, gatts_char_inst[0].perm, gatts_char_inst[0].property, &gatts_char_inst[0].value, &gatts_char_inst[0].control);

        if (ret != ESP_OK)
        {
            printf("add char failed\n");
            printf("Error: %d\n", ret);
            printf("Error: %s\n", esp_err_to_name(ret));
        }
        else
        {
            printf("add char success\n");
        }
        break;
    }
    case ESP_GATTS_START_EVT:
    {
        if (param->start.status != ESP_GATT_OK)
        {
            printf("start service failed\n");
            printf("Error: %s\n", esp_err_to_name(param->start.status));
        }
        else
        {
            printf("start service success\n");
        }

        break;
    }

    case ESP_GATTS_ADD_CHAR_EVT:
    {
        esp_err_t err0 = param->add_char.status;
        if (err0 != ESP_OK)
        {
            printf("add char failed\n");
            printf("Error: %d\n", err0);
            printf("Error: %s\n", esp_err_to_name(err0));
        }
        else
        {
            printf("add char success\n");
        }
        int num = 0;
        for (int i = 0; i < GATTS_CHAR_NUM; i++)
        {
            if (gatts_char_inst[i].char_uuid.uuid.uuid16 == param->add_char.char_uuid.uuid.uuid16)
            {
                gatts_char_inst[i].char_handle = param->add_char.attr_handle;
                break;
            }
        } // 获取char_handle 用于后续操作

        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
    {
        gl_profile_tab[PROFILE_APP_IDX].descr_handle = param->add_char_descr.attr_handle;
        break;
    }
    case ESP_GATTS_CONNECT_EVT:
    {
        gl_profile_tab[PROFILE_APP_IDX].conn_id = param->connect.conn_id;
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
    {
        gl_profile_tab[PROFILE_APP_IDX].conn_id = 0;
        esp_ble_gap_start_advertising(&adv_params);

        break;
    }
    case ESP_GATTS_WRITE_EVT:
    {
        if (!param->write.is_prep)
        {
            printf("write event\n");
            printf("handle: %d\n", param->write.handle);
            printf("len: %d\n", param->write.len);
            printf("value: %s\n", param->write.value);
            // 根据handle判断是哪个char的写事件
            for (int i = 0; i < GATTS_CHAR_NUM; i++)
            {
                if (param->write.handle == gatts_char_inst[i].char_handle)
                {
                    printf("char %d\n", i);
                    printf("char handle: %d\n", gatts_char_inst[i].char_handle);
                    printf("char uuid: %x\n", gatts_char_inst[i].char_uuid.uuid.uuid16);
                     if(gatts_char_inst[i].char_uuid.uuid.uuid16==0xFF01){
                        //将value 从空格分割成ssid和password
                        char *ssid = strtok((char *)param->write.value, ",");
                        char *password = strtok(NULL, ",");
                        printf("ssid: %s\n", ssid);
                        printf("password: %s\n", password);
                        //将ssid和password写入nvs
                        nvs_handle nvs;
                        esp_err_t ret = nvs_open("storage", NVS_READWRITE, &nvs);
                        if (ret == ESP_OK)
                        {
                            printf("nvs open success\n");
                        }
                        else
                        {
                            printf("nvs open failed\n");
                        }
                        ret = nvs_set_str(nvs, "ssid", ssid);
                        if (ret == ESP_OK)
                        {
                            printf("ssid set success\n");
                        }
                        else
                        {
                            printf("ssid set failed\n");
                        }
                        ret = nvs_set_str(nvs, "password", password);
                        if (ret == ESP_OK)
                        {
                            printf("password set success\n");
                        }
                        else
                        {
                            printf("password set failed\n");
                        }
                        nvs_close(nvs);
                        esp_restart();
                     }

                    break;
                }
            }
           
        }
        break;
    }

    default:
        break;
    }
}
void init_ble(){
        esp_err_t ret;
    // config
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    bt_cfg.mode = ESP_BT_MODE_BLE;
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE); // enable bluetooth
    esp_bluedroid_init();
    esp_bluedroid_enable();

    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x20,
        .max_interval = 0x40,
        .appearance = 0x00,
        .manufacturer_len = 0,
        .p_manufacturer_data = NULL,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 0,
        .p_service_uuid = NULL,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    }; // 设置广播数据
    esp_ble_gap_config_adv_data(&adv_data);
    esp_ble_gap_register_callback(gap_event_handler);
    // 开启广播
    esp_ble_gap_start_advertising(&adv_params);

    

    ret = esp_ble_gatts_register_callback(gatts_profile_event_handler);
    if (ret != ESP_OK)
    {
        printf("register callback failed\n");
        printf("Error: %s\n", esp_err_to_name(ret));
    }
    ret = esp_ble_gatts_app_register(PROFILE_APP_IDX);
    if (ret != ESP_OK)
    {
        printf("app register failed\n");
        printf("Error: %s\n", esp_err_to_name(ret));
    }
}
void deinit_ble(){
    
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
}
#endif
