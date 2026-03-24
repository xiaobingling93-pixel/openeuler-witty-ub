# witty-ub数据采集工具指导

## 简介

witty-ub数据采集工具witty-ub-topo和witty-ub-log是C++语言编写的命令行工具，通过解析超节点资源接口数据或日志文件，生成超节点系统拓扑、故障等信息。

## 命令介绍

witty-ub-topo ：超节点系统拓扑实时感知工具，基于管理组件提供的接口或日志，采集超节点计算节点上资源拓扑信息

* 当前支持基于UBSE接口和URMA日志和进行节点拓扑和URMA拓扑数据分析采集，输出拓扑数据到```/var/witty-ub```目录下```lcne-topology.json```和```urma-topology.json```文件中。

### witty-ub-topo 命令行参数说明如下：
| 参数      | 参数类型   | 参数说明  | 使用说明 | 是否必选 |
|:--------:|:----------:|:---------|:---------|:-------:|
| --network-mode | string | 指定组网形式 | 需要为fullmesh或者clos, 当为clos时，lcne-topology.json中的port字段中的remote_port,remote_slot,remote_ubpu和remote_iou字段不打印 | 是 |
| --pod-mode | string | 指定是否是容器化部分 | 需为on(容器化部署)或者off(非容器化部署) | 是 |
| --umq-log-path | string | 指定umq日志路径 | 1.pod-mode=on时，该参数格式需要为pod_id:path列表，即多个pod时格式为 pod_id1:path1,pod_id2:path...，其中pod_id需要符合k8s pod的命名规范（只包含小写字母，数字和连字符-，并且不能以连字符-开头或者结尾），pod_id不能重复，path为文件或者目录的绝对路径，不允许包含逗号“,”<br> 2.pod-mode=off时，该参数为某个文件或者目录的绝对路径，不支持多个路径的形式即不支持"path1,path2"，如不指定，默认为```/var/log/messages```<br>3.当日志路径为目录时，会读取该目录下所有后缀为.log文件的内容，如该路径没有后缀为.log文件则跳过 | pod-mode=on时必选 |
| --pod-id | string | 指定要采集的pod id列表 | 1.pod-mode=off时无效<br> 2.pod-mode=on时，该参数不指定则默认采集全部umq-log-path中指定的pod，如指定pod_id，pod_id需要在umq-log-path指定的pod范围内,pod_id需符合k8s pod的命名规范 | 否 |

* 组网形式为fullmesh，容器化部署
  ```shell
    witty-ub-topo --network-mode fullmesh --pod-mode on --umq-log-path "pod-name1:/log/path/to/pod-name1,pod-name2:/log/path/to/pod-name2" --pod-id pod-name1,pod-name2
  ```
* 组网形式为clos，容器化部署
  ```shell
    witty-ub-topo --network-mode clos --pod-mode on --umq-log-path "pod-name1:/log/path/to/pod-name1,pod-name2:/log/path/to/pod-name2" --pod-id pod-name1,pod-name2
  ```
* 组网形式为fullmesh, 非容器化部署
  ```shell
    witty-ub-topo --network-mode fullmesh --pod-mode off --umq-log-path "/path/to/log/messages"
  ```
* 组网形式为clos, 非容器化部署
  ```shell
    witty-ub-topo --network-mode clos --pod-mode off --umq-log-path "/path/to/log/messages"
  ```

### witty-ub-log
witty-ub-log命令行参数说明如下：

| 参数      | 参数类型   | 参数说明  | 使用说明 | 是否必选 |
|:--------:|:----------:|:---------|:---------|:-------:|
| --pod-mode | string | 指定是否是容器化部分 | 需为on(容器化部署)或者off(非容器化部署) | 是 |
| --ubsocket-log-path | string | 指定UBsocket的日志路径 | 1.pod-mode=on时，该参数格式需要为pod_id:path列表，即多个pod时格式为 pod_id1:path1,pod_id2:path<br> 2.pod-mode=off时，该参数为某一日志文件路径，不支持多个文件的形式，如不指定，默认为```/var/log/messages``` | pod-mode=on时必选 |
| --umq-log-path | string | 指定UMQ日志路径 | 1.pod-mode=on时，该参数格式需要为pod_id:path列表，即多个pod时格式为 pod_id1:path1,pod_id2:path<br> 2.pod-mode=off时，该参数为某个目录路径，不支持多个目录的形式，如不指定，默认为```/var/log/umdk/umq/```目录下所有文件 | pod-mode=on时必选 |
| --liburma-log-path | string | 指定libURMA日志路径 | 1.pod-mode=on时，该参数格式需要为pod_id:path列表，即多个pod时格式为 pod_id1:path1,pod_id2:path<br> 2.pod-mode=off时，该参数为某个目录路径，不支持多个目录的形式，如不指定，默认为```/var/log/umdk/urma/```目录下所有文件 | pod-mode=on时必选 |
| --urmacore-log-path | string | 指定URMA内核日志路径 | 如不指定默认为```dmesg``` | 否 |
| --libudma-log-path | string | 指定libUDMA日志路径 |  1.pod-mode=on时，该参数格式需要为pod_id:path列表，即多个pod时格式为 pod_id1:path1,pod_id2:path<br> 2.pod-mode=off时，该参数为某一日志文件路径，不支持多个文件的形式，如不指定，默认为```/var/log/messages``` | pod-mode=on时必选 |
| --udmacore-log-path | string | 指定UDMA内核日志路径 | 如不指定默认为```dmesg``` | 否 |
| --start-time | string | 筛选发生在该时间后的故障事件 | 格式需要为yyyy-mm-dd hh:mm:ss | 是 |
| --end-time | string | 筛选发生在该时间前的故障事件 | 格式需要为yyyy-mm-dd hh:mm:ss | 是 |
| --event-type | string | 用于筛选的事件类型 | 需要为bind(建链)，unbind(删链)，post(数据面)，如不指定则默认全部采集 | 否 |
| --pod-id | string | 用于筛选的容器pod-id | 仅当pod-mode=on时有效，pod_id需要符合k8s pod的命名规范（只包含小写字母，数字和连字符-，并且不能以连字符-开头或者结尾），如不指定则默认全部采集 | 否 |
| --local-eid | string | 用于筛选故障的本端urma设备eid | 如不指定，默认全部采集 | 否 |
| --jetty-id | string | 用于筛选故障的本端jetty的jetty-id | 如不指定，默认全部采集 | 否 |
 
* 当前支持基于URMA通信组件日志进行分析采集，输出故障的相关数据到```/var/witty-ub```目录下```failure-event.json```文件中

* 容器化部署
    ```shell
    witty-ub-log --pod-mode on \
      --ubsocket-log-path "pod-name1:/log/messages/path/to/pod-name1,pod-name2:/log/messages/path/to/pod-name2" \
      --umq-log-path "pod-name1:/path/to/pod-name1/umdk/umq/,pod-name2:/path/to/pod-name2/umdk/umq/" \
      --liburma-log-path "pod-name1:/path/to/pod-name1/umdk/urma/,pod-name2:/path/to/pod-name2/umdk/urma/" \
      --libudma-log-path "pod-name1:/log/messages/path/to/pod-name1,pod-name2:/log/messages/path/to/pod-name2" \
      --start-time "2026-03-09 00:00:00" --end-time "2026-03-10 00:00:00"
    ```
* 非容器化部署
    ```shell
    witty-ub-log --pod-mode on \
      --ubsocket-log-path "/path/to/log/messages" \
      --umq-log-path "/path/to/log/umdk/umq/" \
      --liburma-log-path "/path/to/log/umdk/urma/" \
      --libudma-log-path "/path/to/log/messages" \
      --start-time "2026-03-09 00:00:00" --end-time "2026-03-10 00:00:00"
    ```