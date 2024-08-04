# ESP32智能物联网花瓶
环境配置 idf ESP-IDF v5.2.1-dirty 
LVGL 组件包在release中下载 

[LVGL download](https://github.com/Gyxqq/esp32_vase/releases/tag/lvgl-components)

```
esp32_vase
├─ .gitignore
├─ .vscode
│  ├─ c_cpp_properties.json
│  └─ settings.json
├─ CMakeLists.txt
├─ img_conv.py
├─ io.csv
├─ light_sensor.py
├─ light_sensor_list.c
├─ log.txt
├─ main
│  ├─ CMakeLists.txt
│  ├─ esp32_vase.c
│  ├─ esp_lcd_panel_st7735.c
│  ├─ GUI_TASK
│  │  └─ show_qrcode.c
│  ├─ http_func.c
│  ├─ img.c
│  ├─ init_ble.c
│  ├─ init_mesh.c
│  ├─ init_wifi.c
│  ├─ light_sensor_list.c
│  ├─ mqttcl.c
│  ├─ sensor.c
│  ├─ speech_recognition.c
│  ├─ static_value.c
│  └─ yahei_16.c
├─ partitions.csv
├─ pytest_hello_world.py
├─ README.md
├─ sdkconfig
├─ sdkconfig.ci
├─ sdkconfig.old
├─ size.txt
└─ test.py

```
