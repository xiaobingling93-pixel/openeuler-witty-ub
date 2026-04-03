---
name: "ub-keyfunc-analysis"
description: "按 bind/unbind/post/poll 语义检索 UB 关键函数，区分跨模块入口函数与执行原子步骤函数，并输出结构化清单。"
---

# ub-keyfunc-analysis

## 目标

在指定组件源码中，围绕以下语义检索“关键函数”：

- 建链（`bind`）
- 删链（`unbind`）
- 数据面发送（`post`）
- 数据面收发完成处理（`poll`）

并将函数分为两类：

- `cross_module_entry`：跨模块入口函数（公共 API、ops 分发入口、汇聚 impl）
- `execution_atom`：执行原子步骤函数（`static` 私有关键动作、核心 WR/CQ 操作函数）

## 支持组件

- `umq`

## 输入

- `component`：必选，当前仅支持 `umq`
- `source_path`：必选，组件源码根路径
- `keywords`：可选，默认 `bind,unbind,post,poll`

### 关键词规则

1. 若未传 `keywords`，默认按 `bind/unbind/post/poll` 全量检索
2. 若只传子集（如 `bind,post`），仅输出对应语义域关键函数
3. 关键词匹配不只看函数名，需结合调用路径语义（如 `umq_tp_*` 分发到 `*_impl`）

## 输出

输出目录：`/var/witty-ub/keyfunc-analysis/`

输出文件：

- `keyfunc.json`

### keyfunc.json 结构

```json
{
  "component": "umq",
  "domains": ["bind", "unbind", "post", "poll"],
  "functions": [
    {
      "name": "umq_ub_post_impl",
      "file": "umq_ub_impl.c",
      "domain": "post",
      "category": "cross_module_entry",
      "layer": "impl_dispatch",
      "reason": "UB/UB+ pro post 汇聚入口，按 io_direction 分发 tx/rx"
    }
  ]
}
```

字段约束：

- `functions[].domain`：仅允许 `bind/unbind/post/poll`
- `functions[].category`：仅允许 `cross_module_entry/execution_atom`
- `functions[].layer` 建议取值：
  - `public_api`
  - `mode_adapter`
  - `impl_dispatch`
  - `core_orchestration`
  - `execution_atom`
- `functions[].file` 必须是裸文件名

## 关键函数判定规则

### A. 跨模块入口函数（cross_module_entry）

至少满足其一：

1. 公共 API 直接入口（如 `umq_bind/umq_unbind/umq_post/umq_poll`）
2. ops/pro_ops 分发入口（典型形态：`umq_tp_*`）
3. 多 mode 共用汇聚 `*_impl`（如 UB/UB+ 共同调用）

### B. 执行原子步骤函数（execution_atom）

至少满足其一：

1. 在关键路径中执行单一关键动作（如 `import/bind_jetty`、`post recv wr`、`poll jfc`）
2. 虽为 `static` 或私有函数，但被核心编排函数直接调用，且失败会影响主路径
3. 具备明显的设备/传输语义操作（URMA WR/CR、jetty/tjetty、credit 等）

### C. 去噪与排除

以下函数默认不纳入关键函数：

1. 仅做日志、简单参数转发且无语义分发价值
2. 通用工具函数（字符串、容器、纯内存操作）
3. 与 `bind/unbind/post/poll` 无关的旁路功能

## 分析步骤

### 1. 语义锚点检索

1. 先检索公共 API 与 pro API（`umq_*`）
2. 识别 `tp_ops/pro_tp_ops` 字段映射，建立 `umq_tp_* -> *_impl` 关系

### 2. 汇聚函数定位

1. 对 `bind/unbind/post/poll` 分别定位 mode 汇聚函数
2. 统计同名实现被多少 mode 入口复用

### 3. 执行原子函数下钻

1. 从汇聚函数向下追踪一级到两级调用
2. 标注 `static` 私有但处于关键路径的动作函数
3. 按“动作必要性 + 失败影响”筛选 `execution_atom`

### 4. 分类与证据生成

1. 为每个函数打上 `domain/category/layer`
2. 记录 `reason`，要求可解释、可复核

## umq 组件代表性保留标准（推荐默认输出）

目标：在每个 domain 中保留“最具代表性、具备实际语义价值”的少量函数，避免仅保留纯转发函数。

- `bind`（建议 2-3 个）
  - 1 个 `impl_dispatch` 或 `core_orchestration`（负责建链主路径编排）
  - 1 个 `execution_atom`（直接执行 import/bind jetty 或同等级设备动作）
  - 可选 1 个入口锚点（公共 API 或 mode_adapter）
- `unbind`（建议 1-2 个）
  - 1 个主执行函数（包含 unbind/unimport/状态收敛）
  - 可选 1 个入口锚点（用于跨模块追踪）
- `post`（建议 3 个，必须覆盖 TX/RX 双向）
  - 1 个汇聚分发函数（按 io_direction 分流）
  - 1 个 TX 发送原子函数（直接 post send WR 或等价动作）
  - 1 个 RX 预投递原子函数（直接 post recv WR 或等价动作）
- `poll`（建议 3 个，必须覆盖 TX/RX 双向）
  - 1 个汇聚分发函数（按 io_direction 分流）
  - 1 个 TX 完成处理函数（poll tx CQ/JFC）
  - 1 个 RX 完成处理函数（poll rx CQ/JFC + 消息语义处理）

说明：

1. 数据面（`post/poll`）默认必须同时包含发送侧（TX）与接收侧（RX）关键函数。
2. 具体函数名不预设；按“语义角色 + 调用证据 + 失败影响”筛选代表函数。
3. 若实现差异导致候选不存在，可替换为同语义、同层级函数，并在 `reason` 中说明替代关系。
4. 纯日志/纯参数转发函数默认不纳入代表性集合，除非其是跨模块唯一入口锚点。
5. 规则正文禁止写死具体函数名；如需示例，仅可放在附录或示例章节，且不得作为判定条件。

## 失败与回退策略

- 仅能定位到入口层，无法下钻原子函数：保留入口函数并标记 `reason` 为“下钻不足”
- 存在多候选同地位函数：全部保留，并在 `reason` 中说明并列关系
- 文件不存在或解析失败：跳过单文件，继续整体分析

## 质量自检清单

- 是否覆盖输入关键词对应全部语义域
- 每个语义域是否同时考虑 `cross_module_entry` 与 `execution_atom`
- `reason` 是否能解释“为什么关键”
- 是否避免将纯工具函数误判为关键函数
- 输出路径与 JSON 字段是否符合约束
