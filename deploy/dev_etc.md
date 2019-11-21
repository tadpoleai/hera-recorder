# 传感器硬件配置说明

对于Tron，先参考Confluence页面[硬件设置](https://confluence.newayz.com/pages/viewpage.action?pageId=20644872)  
获取网卡KERNELS，传感器连接顺序和网络地址的约定

## 配置网卡

### 分配网卡设备名

用root权限编辑`/etc/udev/rules.d/50-pci-usb-net.rules`，并根据Confluence的说明添加网卡记录

> 例如, Tron4工控机的配置为如下
>
> *`/etc/udev/rules.d/50-pci-usb-net.rules`*
>
> ```plain-text
> SUBSYSTEM=="net",ACTION=="add",KERNELS=="0000:00:1c.0",NAME="eth_lan1"
> SUBSYSTEM=="net",ACTION=="add",KERNELS=="0000:00:1c.1",NAME="eth_lan2", ENV{NM_UNMANAGED}="1"
> SUBSYSTEM=="net",ACTION=="add",KERNELS=="0000:00:1c.2",NAME="eth_lan3", ENV{NM_UNMANAGED}="1"
> SUBSYSTEM=="net",ACTION=="add",KERNELS=="0000:00:1c.3",NAME="eth_lan4", ENV{NM_UNMANAGED}="1"
> SUBSYSTEM=="net",ACTION=="add",KERNELS=="0000:00:1f.6",NAME="eth_lan5", ENV{NM_UNMANAGED}="1"
> ```
>
> *`/etc/udev/rules.d/92-pci-usb-net.rules`*
>
> ```plain-text
> SUBSYSTEM=="net",ACTION=="add",ATTRS{idVendor}=="0bda",ATTRS{idProduct}=="8152",KERNELS=="1-8",NAME="eth_usb0", ENV{NM_UNMANAGED}="1"
> SUBSYSTEM=="net",ACTION=="add",ATTRS{idVendor}=="0bda",ATTRS{idProduct}=="8152",KERNELS=="1-9",NAME="eth_usb1", ENV{NM_UNMANAGED}="1"
> SUBSYSTEM=="net",ACTION=="add",ATTRS{idVendor}=="0bda",ATTRS{idProduct}=="8152",KERNELS=="1-10",NAME="eth_usb2", ENV{NM_UNMANAGED}="1"
> ```

### 分配网络地址

用root权限编辑`/etc/dhcp/dhcpd.conf`，并在最后添加

```plain-text
subnet 10.0.0.0 netmask 255.255.255.0 {
    option      routers         10.0.0.1;
    option      subnet-mask     255.255.255.0;
    range       10.0.0.100      10.0.0.200;
}
```

### 配置无线AP服务

用root权限编辑`/etc/network/interfaces`，添加

```plain-text
source /etc/network/interfaces.d/*
```

然后在目录`/etc/network/interfaces.d/`下，为**每一个**传感器用到的网卡分配一个网络地址

> 例如，对于eth_lan5，分配10.0.5.1/24  
> 则新建文件`/etc/network/interfaces.d/eth_lan5`
>
> ```plain-text
> allow-hotplug eth_lan5
> iface eth_lan5 inet static
> address 10.0.5.1
> netmask 255.255.255.0
> gateway 10.0.5.1
> mtu 9000
> ```

这里设置MTU为9000是因为有一部分传感器(如FLIR)需要传输大的网络包

### 设置内核网络缓存

参考[FLIR的网口相机设置指南](https://www.flir.com/support-center/iis/machine-vision/knowledge-base/lost-ethernet-data-packets-on-linux-systems)

首先增大内核缓存大小，用root权限编辑`/etc/sysctl.conf`，添加

```plain-text
net.core.rmem_max=67108864
net.core.rmem_default=67108864
```

## 配置USB串口模块

IMU和GPS等很多传感器使用USB串口传输数据，需要用UDEV规则给他们分配固定的内核名

该部分说明尚待补充
