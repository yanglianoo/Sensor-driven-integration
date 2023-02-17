# Sensor-driven-integration
传感器驱动集成-中间件项目-开发文档
## 姿态传感器
### LORD-MicroStrain
> 官网：https://www.microstrain.com/inertial-sensors/3dm-gx5-25   
> Github：https://github.com/LORD-MicroStrain/MSCL     
> API文档：https://lord-microstrain.github.io/MSCL/Documentation/MSCL%20API%20Documentation/index.html   
- 数据读取测试   
  `setCurrentConfig`: 配置imu角速度和角度输出频率，指定滤波器  
  `getCurrentConfig`: 获取imu配置信息     
  `startSampling`: 开启数据读取
  `parseData`: 解析数据并打印到终端    
  > `3DMGX5-AHRS` 不支持`GNSS`模式，因此在配置文件`setCurrentConfig.h`中注释掉`GNSS`配置代码
   ```bash
   <!-- 新开一个终端 -->
    cd imu/LORD-MicroStrain 
    source setup.bash
    make test
    ./test.out
   ```

### LPMS-IG1
> 官网：https://www.alubi.cn/lpms-ig1-series/    
## 位置传感器
## 速度传感器
## 视觉传感器