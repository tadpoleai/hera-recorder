# Hera 二进制数据记录文件

本文描述 Hera 数据记录文件(`.hera`)的二进制格式, 使用 Hera 库读写记录文件的方法, 以及不使用 Hera 的库读写记录文件时候的编程注意事项

## 文件格式

因 Hera 自身主版本从 3 升级到 4 时候, 将补充信息(ExtraInfo)和日志(Log)嵌入了数据记录文件中, 原有 V3 格式和新的 V4 有所区别

Hera 主版本 3 只能读写 V3 格式的文件,不能读写 V4 格式的文件

Hera 主版本 4 可以读写两种格式(向下兼容), 写入新文件时, 默认写入 V4 格式的文件

### V3 格式

Hera 主版本 3 使用的储存格式, 文件由两部分组成, `记录头(StorageHeader)` 和 `记录数据序列(StorageDataArray)`

#### V3 记录头(StorageHeader)

包含以下信息

- 指示文件版本的魔数
- 采集开始时间
- 采集结束时间
- 传感器数量
- 各传感器数据的数量
- 各传感器数据的容量
- 各传感器的名字

| 记录字段           |            代码字段 |             类型 | 起始偏移 |            长度 | 结束偏移 | 备注                                   |
| :----------------- | ------------------: | ---------------: | -------: | --------------: | :------- | -------------------------------------- |
| 版本魔数           |             MagicV3 |           string |        0 |              16 | 16       | 固定的 `HERA_STORAGE_V3`, 包含结尾`\0` |
| 采集开始时间       |     timestamp_start |         uint64_t |       16 |               8 | 24       | UTC 时间, 单位为 ns                    |
| 采集结束时间       |       timestamp_end |         uint64_t |       24 |               8 | 32       | UTC 时间, 单位为 ns                    |
| 传感器数量         |          device_num |         uint32_t |       32 |               4 | 36       |
| 各传感器数据的数量 | device_message_nums | vector<uint32_t> |       36 | 4 \* device_num | \$END1   | \*A                                    |
| 各传感器数据的容量 |   device_data_sizes | vector<uint64_t> |   \$END1 | 8 \* device_num | \$END2   | \*B                                    |
| 各传感器的名字     |        device_names |   vector<string> |   \$END2 |             \*C | \$END_V3 | \*D                                    |

\*A: 从 `device_message_nums[0]` 到 `device_message_nums[device_num-1]`, 依次储存

\*B: 从 `device_data_sizes[0]` 到 `device_data_sizes[device_num-1]`, 依次储存

\*C: 长度为 sum(device_names) + 4 \* device_num

\*D: 从 `device_names[0]` 到 `device_names[device_num-1]`, 依次储存, 先储存长度 `device_names[i].size()`, 再按 `array` 依次储存, 通常不储存结尾`\0`

#### V3 记录数据序列(StorageDataArray)

由紧实连续的 `原始储存数据(DeviceData)` 构成(关于 `原始储存数据(DeviceData)` 的内容, 参考 [Hera 代码说明](source.md))

从偏移地址 `END_V3` 开始连续储存

### V4 格式

Hera 主版本 4 默认使用的储存格式
和 V3 格式的主要区别为记录头信息(StorageHeader)增加了补充信息(ExtraInfo)和日志(Log)

#### V4 记录头(StorageHeader)

| 记录字段           |            代码字段 |                   类型 |                       起始偏移 |            长度 | 结束偏移    | 备注                                       |
| :----------------- | ------------------: | ---------------------: | -----------------------------: | --------------: | :---------- | ------------------------------------------ |
| 版本魔数           |         **MagicV4** |                 string |                              0 |              16 | 16          | **固定的 `HERA_STORAGE_V4`**, 包含结尾`\0` |
| 采集开始时间       |     timestamp_start |               uint64_t |                             16 |               8 | 24          | 同 V3                                      |
| 采集结束时间       |       timestamp_end |               uint64_t |                             24 |               8 | 32          | 同 V3                                      |
| 传感器数量         |          device_num |               uint32_t |                             32 |               4 | 36          | 同 V3                                      |
| 各传感器数据的数量 | device_message_nums |       vector<uint32_t> |                             36 | 4 \* device_num | \$END1      | 同 V3                                      |
| 各传感器数据的容量 |   device_data_sizes |       vector<uint64_t> |                         \$END1 | 8 \* device_num | \$END2      | 同 V3                                      |
| 各传感器的名字     |        device_names |         vector<string> |                         \$END2 |            同上 | \$END_V3    | 同 V3                                      |
| 补充信息           |          extra_info |                 string |                       \$END_V3 |            不定 | \$END_EXTRA | \E                                         |
| 日志               |                logs |      vector<LogString> |                    \$END_EXTRA |            不定 | \$END_LOGS  | \F                                         |
| 填充               |                   - |                   void |                     \$END_LOGS |               - | 4\*2^20     | **\G**                                     |
| 索引               |             indices | vector<TimestampIndex> | 4MiB - 40 - 16\*indices.size() |       4MiB - 40 | 4MB-40      | **\H**                                     |
| 索引数量           |      indices.size() |                 size_t |                      4MiB - 40 |               8 | 4MiB - 32   | **\H**                                     |
| 索引魔数           |        MagicIndices |                 string |                      4MiB - 32 |              32 | 4MiB        | **\H**                                     |

\E: 将 json 对象序列化成 string 后储存, 可记录任意信息. 通常可记录传感器参数安装位置, 操作人员, 任务 ID, 备注等信息  
写入方式为先写入 uint32_t 的 string.size(), 再按顺序写入字符串

\F: 将 vector 中的 LogString 连续写入, 关于 LogString 的内存格式在此不详细描述  
若 LogString 写入后总长度超过 4MiBytes, 则抛弃后续的数据

\G: 从\$END_LOGS, 开始到 4MiBytes 为止, 使用全 0 填充, 使得记录头总长度始终为 4MBytes, 后续记录数据序列总是从固定的 4MBytes 开始

\H: 从 4.6.3 版本开始，新的记录文件会在原有填充 0 自底向上安排索引，索引传感器时间和数据在记录文件中的偏移，可方便对文件进行快速跳转(seek)

## 使用 Hera 库读写记录文件

## 其他平台读写记录文件

### 采集数据并写入 Hera 文件

1. 新建一个记录文件并按二进制写入方式打开

1. 选择 V3 或者 V4 格式(若不需要高级功能, 可选用 V3 格式)  
   V3 格式在新建文件时, 要确定保证传感器数量和名字已经固定, 否则记录头的长度无法确定, V4 格式则可后续确定

1. 按上文的格式, 向记录文件写入记录头  
   此时**传感器的数据数量和容量, 采集结束时间**都无法确定, 这些字段可设为 0

1. 开始采集, 当采集到数据时候, 按 DeviceData 的格式写入记录文件  
   有些平台的写入为阻塞操作, 可使用写队列缓存数据

   关于 DeviceData 的格式, 参考 [Hera 代码说明](source.md)

1. 按一定的时间间隔建立索引

1. 采集结束时, 等待写队列情况, 文件已经全部写入(flush)

1. 重新计算并确定记录头中**传感器的数据数量和容量, 采集结束时间**  
   若使用了 V4 格式, 还需要重新生成补充信息

1. 将文件头回退到偏移为 0 的位置, 重新写入新的记录头

### 读取 Hera 文件

1. 二进制读取方式打开记录文件

1. 判断文件头是否为 MagicV3 或者 MagicV4  
   若非, 读取失败

1. 按 V3 方式读取文件头, 判断**采集结束时间**是否非 0 且大于**采集开始时间**
   若非, 说明该记录文件在写入时候没有正常退出, 需要使用工具 `hera-storage-tool` 重建文件头

1. 若版本是 V4 且有读取额外信息和日志的需求, 则继续读取解析额外信息和日志, 然后将文件头 `seek` 到 4MBytes 处

1. 依次读取 DeviceData, 直到遇到 EOF
