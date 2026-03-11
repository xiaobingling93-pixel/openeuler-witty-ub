# witty-ub-topo 并发写JSON文件解决方案

## 问题描述

当多个witty-ub-topo进程同时执行并尝试写入同一个JSON文件时，会出现以下问题：
1. **数据竞争**：多个进程同时写入导致数据混乱
2. **数据丢失**：后写入的进程可能覆盖先写入的数据
3. **文件损坏**：并发写入可能导致JSON文件格式错误

## 解决方案

针对Linux平台，实现了基于文件锁的并发安全机制，采用以下策略：

### 1. 文件锁机制（flock）

使用Linux的`flock`系统调用实现进程级别的文件锁：
- `LOCK_EX`：获取排他锁（阻塞等待）
- `LOCK_UN`：释放锁
- 确保同一时间只有一个进程能够写入文件

### 2. 临时文件 + 原子重命名

采用"先写临时文件，再原子重命名"的策略：
1. 将JSON数据写入临时文件（`filename.tmp`）
2. 获取目标文件的排他锁
3. 使用`std::filesystem::rename`原子性地将临时文件重命名为目标文件
4. 释放文件锁

### 3. 完善的错误处理

- 文件打开失败时清理临时文件
- 文件锁获取失败时清理临时文件
- 重命名失败时释放锁并清理临时文件
- 所有错误都有详细的日志记录

## 代码实现

### 修改的文件

`src/framework/json-io/witty_json.h`

### 关键代码逻辑

```cpp
// 1. 生成临时文件路径
std::filesystem::path temp_path = file_path.parent_path() / 
    (file_path.stem().string() + ".tmp" + file_path.extension().string());

// 2. 写入临时文件
std::ofstream output(temp_filename);
// ... 写入JSON数据 ...

// 3. 获取文件锁
int lock_fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0644);
flock(lock_fd, LOCK_EX);  // 阻塞等待排他锁

// 4. 原子重命名
std::filesystem::rename(temp_filename, filename);

// 5. 释放文件锁
flock(lock_fd, LOCK_UN);
close(lock_fd);
```

## 技术优势

### 1. 进程安全
- 使用`flock`系统调用，适用于多进程环境
- 不同进程之间能够正确同步

### 2. 原子性
- `std::filesystem::rename`在Linux上是原子操作
- 确保文件要么完全写入，要么完全不写入

### 3. 数据完整性
- 临时文件机制确保写入过程中不会影响原文件
- 只有写入成功后才替换原文件

### 4. 可靠性
- 完善的错误处理和资源清理
- 详细的日志记录便于调试

## 使用场景

此解决方案适用于以下调用场景：

1. **URMA拓扑数据写入**
   - 文件路径：`URMA_JSON_PATH`
   - 调用位置：`src/plugins/urma/urma_topology.cpp`
   - 包含数据：pod、jetty、urma_device

2. **LCNE拓扑数据写入**
   - 文件路径：`lcne::common::JSON_OUTPUT_FILE`
   - 调用位置：`src/plugins/lcne/lcne_topology.cpp`
   - 包含数据：iodie、port

## 测试建议

### 1. 并发写入测试

```bash
# 启动多个witty-ub-topo进程同时写入
for i in {1..10}; do
    witty-ub-topo --config config.json &
done
wait
```

### 2. 数据一致性验证

```bash
# 验证JSON文件格式
python3 -m json.tool output.json

# 检查数据完整性
# 比较多次写入后的数据是否一致
```

### 3. 性能测试

```bash
# 测试并发写入的性能
time witty-ub-topo --config config.json
```

## 注意事项

1. **文件系统要求**
   - 需要支持`flock`系统调用（Linux标准支持）
   - 需要支持原子重命名操作（ext4、xfs等）

2. **权限要求**
   - 进程需要有目标文件的写权限
   - 进程需要有目标目录的读写权限

3. **性能影响**
   - 文件锁会引入一定的等待时间
   - 对于高并发场景，建议优化写入频率

## 扩展性

如果需要支持其他平台，可以考虑：

1. **Windows平台**
   - 使用`LockFileEx`/`UnlockFileEx` API
   - 使用`MoveFileEx`实现原子重命名

2. **跨平台方案**
   - 使用第三方库如`Boost.Interprocess`
   - 使用数据库替代文件存储

## 总结

通过实现文件锁和临时文件机制，成功解决了witty-ub-topo在多进程并发写入JSON文件时的数据竞争问题。该解决方案：
- ✅ 保证数据完整性
- ✅ 避免文件损坏
- ✅ 支持多进程并发
- ✅ 提供完善的错误处理
- ✅ 便于调试和监控

适用于Linux平台的witty-ub-topo应用场景。
