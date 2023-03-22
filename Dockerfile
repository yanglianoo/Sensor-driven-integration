# 指定使用的基础镜像
FROM ubuntu:20.04

# 更新 Ubuntu 软件源并安装必要的软件包
RUN apt-get update && \
    apt-get install -y build-essential && \    
    apt-get install libudev-dev && \
    rm -rf /var/lib/apt/lists/*

# 将当前文件夹的内容拷贝到容器的 /home/Sensor-driven-integration 目录下
COPY . /home/Sensor-driven-integration

# 设置容器的默认工作目录为 /home/Sensor-driven-integration
WORKDIR /home/Sensor-driven-integration


