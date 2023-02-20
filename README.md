# Sensor-driven-integration
`main`分支为`release`版本，发布传感器驱动SDK，不包含各种传感器驱动库的源文件，只含封装后的头文件与动态库。     
`dev`分支为开发分支，包含各传感器的驱动源文件
# 传感器驱动集成-中间件项目-开发文档
 - `lib`：各类传感器的驱动动态库
 - `include`：二次封装头文件
 - `src`: 二次封装源文件          

现在支持的传感器：     
`3DMGX5-AHRS`：    
`LPMS-IG1`：
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
> 驱动库：https://bitbucket.org/lpresearch/lpmsig1opensourcelib
- 数据读取测试
  ```bash
    cd imu/lpmsig1opensourcelib/linux_example/build
    ./LpmsIG1_SimpleExample /dev/ttyUSB0
  ```
## 位置传感器
## 速度传感器
## 视觉传感器