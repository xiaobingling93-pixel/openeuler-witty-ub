# 快速使用指导

- [快速使用指导](#快速使用指导)
  - [编译指导](#编译指导)
  - [使用指导](#使用指导)
    - [超节点系统拓扑实时感知工具witty-ub-topo使用指导](#超节点系统拓扑实时感知工具witty-ub-topo使用指导)
    - [超节点多源系统日志解析工具witty-ub-log使用指导](#超节点多源系统日志解析工具witty-ub-log使用指导)

## 编译指导

* 编译环境： openEuler Linux x86/aarch64

* 编译需要安装以下包
    * cmake gcc-c++ make log4cplus-devel cpp-httplib-devel sqlite-devel jsoncpp-devel tinyxml2-devel openssl-devel zlib-devel brotli-devel

```shell
sudo yum install -y gcc-c++ make log4cplus-devel cpp-httplib-devel sqlite-devel jsoncpp-devel tinyxml2-devel openssl-devel zlib-devel brotli-devel
```

* 获取源码

```shell
sudo git clone https://gitcode.com/openeuler/witty-ub.git
```

* 编译

```shell
mkdir build && cd build
sudo cmake ..
sudo make 
```

* 生成的二进制在build/src目录下
    * witty-ub-topo: 超节点系统拓扑实时感知工具，基于管理组件提供的接口或日志，采集超节点计算节点上资源拓扑信息。
    * witty-ub-log: 超节点多源系统日志解析工具，采集超节点各组件日志，识别组件故障事件及关联日志。
    * 当前witty-ub-topo和witty-ub-log主要面向URMA通信场景，采集节点和URMA通信资源拓扑信息，识别URMA通信相关组件日志中的故障日志。
## 使用指导
### 超节点系统拓扑实时感知工具witty-ub-topo使用指导
* 在witty-ub根目录下执行以下命令，输出对应json格式的拓扑信息到文件```/var/witty-ub```目录下```lcne-topology.json```和```urma-topology.json```文件中。        
* 命令行参数详细解析请见[witty-ub工具使用指导](./witty-ub数据采集工具指导.md)
    ``` shell
    ./build/src/witty-ub-topo --network-mode fullmesh --pod-mode on --umq-log-path "pod-name1:/log/path/to/pod-name1,pod-name2:/log/path/to/pod-name2" --pod-id pod-name1,pod-name2
    ```
### 超节点多源系统日志解析工具witty-ub-log使用指导
* 在witty-ub根目录下执行以下命令，输出对应json格式的拓扑信息到文件```/var/witty-ub```目录下```failiure-event.json```文件中。
* 命令行参数详细解析请见[witty-ub工具使用指导](./witty-ub数据采集工具指导.md)
    ```shell
    witty-ub-log --pod-mode on \
    --ubsocket-log-path "pod-name1:/log/messages/path/to/pod-name1,pod-name2:/log/messages/path/to/pod-name2" \
    --umq-log-path "pod-name1:/path/to/pod-name1/umdk/umq/,pod-name2:/path/to/pod-name2/umdk/umq/" \
    --liburma-log-path "pod-name1:/path/to/pod-name1/umdk/urma/,pod-name2:/path/to/pod-name2/umdk/urma/" \
    --libudma-log-path "pod-name1:/log/messages/path/to/pod-name1,pod-name2:/log/messages/path/to/pod-name2" \
    --start-time "2026-03-09 00:00:00" --end-time "2026-03-10 00:00:00"
    ```
