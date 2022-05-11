# rkauto_tool授权工具使用说明

# 申请用户名和密码

请联系对应业务申请用户名和密码（每个账号有次数限制，请妥善保管）

# 执行授权

## 直接设备上授权（设备可以联网）

- 将对应平台的rkauth_tool程序，通过adb或拷贝方式部署到设备中
  
- 执行授权命令

```
export RKAUTH_IP=10.10.10.16（如果在rk公司的话）
./rkauth_tool_bin --user="xxxxxx" --passwd="xxxxxx" --output="key.lic"
```

执行成功后授权文件保存到"key.lic"文件，如果设备重新烧写固件可能会丢失，可以备份一份到PC

## PC授权（设备无法联网）

- 将对应平台的rkdevice_info执行程序，通过adb或拷贝方式部署到设备中

- 执行`rkdevice_info`命令，执行成功将生成`device.inf`文件，将该文件拷贝回pc

- 在PC执行授权命令（使用x86版本的授权程序）

确保PC能够联网

```
./rkauth_tool_bin --user="xxxxxx" --passwd="xxxxxx" --output="key.lic" --device_info=/path/to/device.inf
```

执行成功后授权文件保存到"key.lic"文件，将其拷贝到设备中使用。

Demo添加license的示例代码如下:
```
    const char *license_path = "./key.lic";

    rockx_config_t *config = rockx_create_config();
    if (license_path != nullptr) {
        rockx_add_config(config, ROCKX_CONFIG_LICENCE_KEY_PATH, (char *)license_path, strlen(license_path));
    }

    ret = rockx_create(&bg_matting_handle, ROCKX_MODULE_BG_MATTING, config, sizeof(rockx_config_t));
    if (ret != ROCKX_RET_SUCCESS) {
        printf("init rockx module ROCKX_MODULE_BG_MATTING error %d\n", ret);
        return -1;
    }

    rockx_release_config(config);
```
