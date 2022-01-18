# 硬件配置说明

对于 Tron, 先参考 Confluence 页面 [硬件设置](https://confluence.newayz.com/pages/viewpage.action?pageId=20644872)  
获取网卡 KERNELS, 传感器连接顺序和网络地址的约定

## 配置网卡

### 分配网卡设备名

用 root 权限编辑`/etc/udev/rules.d/50-pci-usb-net.rules`, 并根据 Confluence 的说明添加网卡记录

> 例如, Tron4 工控机的配置为如下
>
> _`/etc/udev/rules.d/50-pci-usb-net.rules`_
>
> ```plain-text
> SUBSYSTEM=="net",ACTION=="add",KERNELS=="0000:00:1c.0",NAME="eth_lan1"
> SUBSYSTEM=="net",ACTION=="add",KERNELS=="0000:00:1c.1",NAME="eth_lan2", ENV{NM_UNMANAGED}="1"
> SUBSYSTEM=="net",ACTION=="add",KERNELS=="0000:00:1c.2",NAME="eth_lan3", ENV{NM_UNMANAGED}="1"
> SUBSYSTEM=="net",ACTION=="add",KERNELS=="0000:00:1c.3",NAME="eth_lan4", ENV{NM_UNMANAGED}="1"
> SUBSYSTEM=="net",ACTION=="add",KERNELS=="0000:00:1f.6",NAME="eth_lan5", ENV{NM_UNMANAGED}="1"
> ```
>
> _`/etc/udev/rules.d/92-pci-usb-net.rules`_
>
> ```plain-text
> SUBSYSTEM=="net",ACTION=="add",ATTRS{idVendor}=="0bda",ATTRS{idProduct}=="8152",KERNELS=="1-8",NAME="eth_usb0", ENV{NM_UNMANAGED}="1"
> SUBSYSTEM=="net",ACTION=="add",ATTRS{idVendor}=="0bda",ATTRS{idProduct}=="8152",KERNELS=="1-9",NAME="eth_usb1", ENV{NM_UNMANAGED}="1"
> SUBSYSTEM=="net",ACTION=="add",ATTRS{idVendor}=="0bda",ATTRS{idProduct}=="8152",KERNELS=="1-10",NAME="eth_usb2", ENV{NM_UNMANAGED}="1"
> ```

### 分配网络地址

用 root 权限编辑 `/etc/network/interfaces`, 添加

```plain-text
source /etc/network/interfaces.d/*
```

然后在目录 `/etc/network/interfaces.d/`下, 为**每一个**传感器用到的网卡分配一个网络地址

> 例如, 对于 eth_lan5, 分配 10.0.5.1/24  
> 则新建文件 `/etc/network/interfaces.d/eth_lan5`
>
> ```plain-text
> allow-hotplug eth_lan5
> iface eth_lan5 inet static
> address 10.0.5.1
> netmask 255.255.255.0
> gateway 10.0.5.1
> mtu 9000
> ```

这里设置 MTU 为 9000 是因为有一部分传感器(如 FLIR)需要传输大的网络包

### 设置内核网络缓存

参考 [FLIR 的网口相机设置指南](https://www.flir.com/support-center/iis/machine-vision/knowledge-base/lost-ethernet-data-packets-on-linux-systems)

增大内核缓存大小, 用 root 权限编辑 `/etc/sysctl.conf`, 添加

```plain-text
net.core.rmem_max=67108864
net.core.rmem_default=67108864
```
并可用ethtool修改网口缓存 rx buffer深度
`ethtool -G eth_lan1 rx 2048`
可写入rc local

## 配置 USB 串口模块

IMU 和 GPS 等很多传感器使用 USB 串口传输数据, 需要用 UDEV 规则给他们分配固定的内核名

可考虑使用厂商名区分,例如

```plain-text
KERNEL=="ttyUSB*",ATTRS{idVendor}=="xxxx",ATTRS{idProduct}=="xxxx",SYMLINK+="ttyXXXX"
```

或者使用 PCI 通道号区分, 该部分待补充
