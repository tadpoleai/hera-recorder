# WLAN 配置指南

在可移动的采集平台使用时，需要保证下位机能够和运行客户端的手机连接

我们可以给采集平台配置一个无线AP

## 前置需求

### 安装依赖

1. 安装hostapd

    `sudo apt-get install hostapd`

1. 安装dhcpd

    `sudo apt-get install isc-dhcp-server`

### 无线适配器

#### 购买无线适配器

购买支持AP模式的无线适配器，并安装驱动(若有需要)  
在Ubuntu下支持的具体列表，可参考[RPi USB Wi-Fi Adapters](https://elinux.org/RPi_USB_Wi-Fi_Adapters)或者[Wireless Adapters Chipset Table](https://wikidevi.com/wiki/Wireless_adapters/Chipset_table)这两篇网页，注意需要AP Mode为Yes
*Tron 使用的型号为Ralink USB WiFi RT5370，能够免驱安装，缺点是速率只能到54Mbps (802.11g)*

#### 确定ID

将无线适配器插入工控机，运行`dmesg`，并观察输出  

> 例如，RT5370的输出如下
>
> ```plain-text
> usb 1-2: New USB device found, idVendor=148f, idProduct=5370
> usb 1-2: New USB device strings: Mfr=1, Product=2, SerialNumber=3
> usb 1-2: Product: 802.11 n WLAN
> usb 1-2: Manufacturer: Ralink
>usb 1-2: SerialNumber: 1.0
> ```

*这里的`idVendor=148f`, `idProduct=5370`就是无线适配器的ID*

#### 确认支持AP模式

运行`iw list`，确认该适配器支持AP模式  

> 例如，RT5370的输出如下
>
> ```plain-text
> Supported interface modes:
>         * IBSS
>         * managed
>         * AP
>         * AP/VLAN
>         * monitor
>         * mesh point
>```

## 配置无线AP

### 分配网卡设备名

用root权限编辑`/etc/udev/rules.d/91-usb-net.rules`，并添加一行

```plain-text
SUBSYSTEM=="net",ACTION=="add",ATTRS{idVendor}=="148f",ATTRS{idProduct}=="5370",NAME="wlan0",ENV{NM_UNMANAGED}="1"
```

*这里的`148f`和`5370`是上一步得到的ID，需要根据实际情况更改*  
*如若主板上连接了不止一个同型号的无线网卡，则可能需要在UDEV规则中加入KERNELS选项来指定上级接口号，比如`KERNELS=="1-3"`，可自行参考UDEV设置规则*

### 配置DHCP服务

用root权限编辑`/etc/dhcp/dhcpd.conf`，并在最后添加

```plain-text
subnet 10.0.0.0 netmask 255.255.255.0 {
    option      routers         10.0.0.1;
    option      subnet-mask     255.255.255.0;
    range       10.0.0.100      10.0.0.200;
}
```

### 配置无线AP服务

用root权限编辑`/etc/hostapd/hostapd.conf`，内容如下

```plain-text
interface=wlan0
ssid=WAYZ_TRON_TR040001
channel=7
hw_mode=g
macaddr_acl=0
ignore_broadcast_ssid=0
auth_algs=1
wpa=3
wpa_passphrase=wayztron
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
```

注意，*这里的配置是Tron4-1号机的，请根据需要修改SSID，频道和密码*

### 给无线网卡分配网络地址

用root权限直接编辑`/etc/network/interfaces`  
也可以新建单独的文件，`/etc/network/interfaces.d/wlan0`(
    需确认`/etc/network/interfaces`中包含
    `source /etc/network/interfaces.d/*`
    这句话)  
添加以下记录

```plain-text
allow-hotplug wlan0
iface wlan0 inet static
address 10.0.0.1
netmask 255.255.255.0
network 10.0.0.0
up if pgrep hostapd ; then pkill hostapd ; fi
post-up hostapd -B /etc/hostapd/hostapd.conf
down if pgrep hostapd ; then pkill hostapd ; fi
```

### 重启

重启系统以确认无线AP能够连接上

## 可选的其他工作

### Wifi二维码

访问[QR Code Generator](http://goqr.me/#t=wifi)，填入SSID和密码可以生成连接Wifi的二维码  
iOS11以上的设备直接打开相机扫描二维码即可连接  
Android需要使用QR Scanner扫码（微信不支持）或者手工输入密码
