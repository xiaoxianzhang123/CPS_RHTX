#!/bin/bash

echo "-------------检查配置----------------"

# 检查是否存在名为tap0的tun网卡
echo "…  检查tap网卡状态..."
if [[ -e /dev/net/tun && -c /dev/net/tun && -x /sbin/ip ]]; then
    if [[ -n $(ip link show tap0 2>/dev/null) ]]; then
        echo "√  tap0网卡已存在"
        
        # 检查tap0网卡是否处于启用状态
        if [[ $(ip link show tap0 | grep -c "UP") -eq 0 ]]; then
            # 如果没启用，启用它
            echo "…  启动网卡中..."
            sudo ip link set dev tap0 up
            echo "√  tap0网卡已启用"
        else
            echo "√  tap0网卡已处于启用状态"
        fi
    else
        # 创建tap0网卡
        echo "…  未创建网卡，创建网卡中..."
        sudo ip tuntap add dev tap0 mode tap
        echo "√  tap0网卡创建成功。"
        
        # 启用tap0网卡
        sudo ip link set dev tap0 up
        echo "√  tap0网卡已启用。"
    fi
else
    echo "×  请确保系统支持tun/tap设备，并且安装了必要的工具。"
    exit 1
fi


# 检查是否存在./n2sd程序
if [ ! -e ./n2sd ]; then
    echo "×  未找到n2sd程序，开始编译..."
    # 尝试编译nic2serialportd.c文件
    gcc nic2serialportd.c -o n2sd -lpthread

    # 检查编译是否成功
    if [ $? -eq 0 ]; then
        echo "√  编译成功，程序已生成。"
    else
        echo "×  编译失败，请检查nic2serialportd.c文件和依赖项。"
        exit 1
    fi
fi

echo "…  进入网卡-串口连接程序..."
if [[ -x ./n2sd ]]; then
    echo ""
    echo "-------------程序开始----------------"
    sudo ./n2sd 
else
    echo "×  未找到n2sd程序，请确保程序在同层文件夹中。"
fi


# # 运行同层文件夹中的./n2sd程序
# echo "…  进入网卡-串口连接程序..."
# if [[ -x ./n2sd ]]; then
#     echo ""
#     echo "-------------程序开始----------------"
#     sudo ./n2sd 
# else
#     echo "× 未找到n2sd程序，请确保程序在同层文件夹中。"
# fi
