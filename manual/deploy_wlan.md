# WLAN 配置指南

在可移动的采集设备使用 Hera 时, 为保证 `hera-daemon` 和运行 `hera-client` 的移动设备之间的连接  
需给采集设备增加一个无线 AP

## 前置需求

### 安装依赖

1. 安装 hostapd

   `sudo apt-get install hostapd`

1. 安装 dhcpd

   `sudo apt-get install isc-dhcp-server`

### 无线适配器

#### 购买无线适配器

购买支持 AP 模式的无线适配器, 最好能够免驱, 若有需要安装驱动

- 在 Ubuntu 下支持的具体列表, 可参考 [RPi USB Wi-Fi Adapters](https://elinux.org/RPi_USB_Wi-Fi_Adapters) 或者 [Wireless Adapters Chipset Table](https://wikidevi.com/wiki/Wireless_adapters/Chipset_table) 这两篇网页, 注意需要 AP Mode 为 Yes

- _Tron 使用的型号为 Ralink USB WiFi RT5370, 能够免驱安装, 缺点是速率只能到 54Mbps (802.11g)_

#### 确定 WLAN 适配器厂商 ID 和产品 ID

将无线适配器插入采集设备, 并运行 `dmesg`, 观察输出, 记录厂商 ID 和产品 ID

- _例如, RT5370 的输出如下_

  > ```plain-text
  > usb 1-2: New USB device found, idVendor=148f, idProduct=5370
  > usb 1-2: New USB device strings: Mfr=1, Product=2, SerialNumber=3
  > usb 1-2: Product: 802.11 n WLAN
  > usb 1-2: Manufacturer: Ralink
  > usb 1-2: SerialNumber: 1.0
  > ```

  _这里的 `idVendor=148f`, `idProduct=5370` 就是无线适配器的 ID_

#### 确认适配器支持 AP 模式

运行`iw list`, 确认该适配器支持 AP 模式

- _例如, RT5370 的输出如下_

  > ```plain-text
  > Supported interface modes:
  >         * IBSS
  >         * managed
  >         * AP
  >         * AP/VLAN
  >         * monitor
  >         * mesh point
  > ```

  _这里的 AP 表明该适配器支持 AP 模式_

## 配置无线 AP

### 分配网卡设备名

用 root 权限编辑`/etc/udev/rules.d/91-usb-net.rules`, 并添加一行

```plain-text
SUBSYSTEM=="net", ACTION=="add", ATTRS{idVendor}=="148f", ATTRS{idProduct}=="5370", NAME="wlan0", ENV{NM_UNMANAGED}="1"
```

_这里的`148f`和`5370`是上一步得到的 ID, 需要根据实际情况更改_  
_如若主板上连接了不止一个同型号的无线网卡, 则可能需要在 UDEV 规则中加入 KERNELS 选项来指定上级接口号, 比如`KERNELS=="1-3"`, 可自行参考 UDEV 设置规则_

### 配置 DHCP 服务

用 root 权限编辑`/etc/dhcp/dhcpd.conf`, 并在最后添加

```plain-text
subnet 10.0.0.0 netmask 255.255.255.0 {
    option      routers         10.0.0.1;
    option      subnet-mask     255.255.255.0;
    range       10.0.0.100      10.0.0.200;
}
```

### 配置无线 AP 服务

用 root 权限编辑`/etc/hostapd/hostapd.conf`, 内容如下

```plain-text
interface=wlan0
driver=nl80211

ssid=WAYZ_TRON_TR040001
auth_algs=1
wpa=2
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
wpa_passphrase=wayztron

macaddr_acl=0
ignore_broadcast_ssid=0

hw_mode=g
channel=7
preamble=1

ieee80211n=1
wmm_enabled=1
ht_capab=[HT40-][HT40+][SHORT-GI-40][RX-STBC1]
require_ht=1
```

注意, _这里的配置是 Tron4-1 号机的, 请根据需要修改 SSID, 驱动, 频道和密码_

### 给无线网卡分配网络地址

以 Root 权限编辑 `/etc/network/interfaces`  
也可以新建单独的文件 `/etc/network/interfaces.d/wlan0` (需确认 `/etc/network/interfaces` 中包含 `source /etc/network/interfaces.d/*` 这句话)

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

### 测试 DHCP 服务

执行

```shell
sudo rm -rf /var/lib/dhcp/dhcpd.leases
sudo touch /var/lib/dhcp/dhcpd.leases
sudo dhcpd wlan0
```

测试手机是否能连接上 AP 并获得 IP 地址, 

若一切正常, 将启动 hostapd 和 dhcpd 的命令加入到 interfaces 设定里, 如下：

```plain-text
allow-hotplug wlan0
iface wlan0 inet static
address 10.0.0.1
netmask 255.255.255.0
dns-nameservers 8.8.8.8

up if pgrep hostapd ; then pkill hostapd ; fi
up if pgrep dhcpd ; then pkill dhcpd ; fi
post-up hostapd -B /etc/hostapd/hostapd.conf
post-up rm -rf /var/lib/dhcp/dhcpd.leases
post-up touch /var/lib/dhcp/dhcpd.leases
post-up dhcpd wlan0

down if pgrep hostapd ; then pkill hostapd ; fi
down if pgrep dhcpd ; then pkill dhcpd ; fi
```

### 重启

输入以下命令, 或者直接重启系统

```shell
sudo ifdown --force wlan0
sudo ifup wlan0
```

然后再次确认无线 AP 能够被手机连接上

## 可选的其他工作

### Wifi 二维码

访问 [QR Code Generator](http://goqr.me/#t=wifi), 填入 SSID 和密码可以生成连接 Wifi 的二维码

iOS11 以上的设备直接打开相机扫描二维码即可连接
Android 需要使用 QR Scanner 扫码（微信不支持）或者手工输入密码
