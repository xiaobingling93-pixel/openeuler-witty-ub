---
name: "ub-callstack-aggregation"
description: "聚合 ubsocket/umq/liburma/libudma 的函数调用图，并补充跨组件调用边。"
---

# ub-callstack-aggregation

## 目标

将指定组件集合各自的 `callstack.json` 聚合为一张统一调用图，并补充跨组件调用边：

- `ubsocket`
- `umq`
- `liburma`
- `libudma`

跨组件边的识别方法：

1. 以各组件 `callstack.json` 的 `nodes` 作为候选函数集合
2. 逐节点回溯到源码函数体（依据 `file`）
3. 在函数体中提取调用表达式
4. 将调用名匹配到“其他组件”的函数节点，形成 `src -> dst`

## 输入

组件集合输入（至少 1 个组件，支持任意子集）：

- 每个被聚合组件都必须成对提供：
  - `--<component>-src`
  - `--<component>-callstack`
- `<component>` 必须是以下之一：
  - `ubsocket`
  - `umq`
  - `liburma`
  - `libudma`

约束规则：

1. 不需要聚合的组件可以不传
2. 对于需要聚合的组件，`src` 与 `callstack` 缺一不可
3. 至少传入 1 个组件；若传入多个组件，则允许生成跨组件调用边

## 输出

输出目录：`/var/witty-ub/callstack-analysis`

输出文件：

- `overall_callstack.json`：聚合后的统一调用图

### callstack.json 结构

```json
{
  "nodes": [
    {
      "id": "umq::umq_send@send.c",
      "name": "umq_send",
      "component": "umq",
      "file": "send.c"
    }
  ],
  "edges": [
    {
      "src": "umq::rpc_entry@rpc.c",
      "dst": "liburma::urma_cmd_xxx@urma_cmd.c",
      "confidence": "confirmed",
      "kind": "cross_component",
      "evidence": {
        "caller_component": "umq",
        "callee_component": "liburma",
        "matched_token": "urma_cmd_xxx"
      }
    }
  ]
}
```

补充字段约定：

- `edges[].kind`：`intra_component` 或 `cross_component`
- `edges[].evidence`：仅建议用于跨组件边，记录匹配证据

## 实现脚本

脚本位置：`skill/ub-callstack-aggregation/scripts/aggregate_callstack.py`

### 运行示例

```bash
python3 skill/ub-callstack-aggregation/scripts/aggregate_callstack.py \
  --liburma-src /path/to/liburma \
  --liburma-callstack /var/witty-ub/callstack-analysis/liburma/callstack.json \
  --libudma-src /path/to/libudma \
  --libudma-callstack /var/witty-ub/callstack-analysis/libudma/callstack.json
```

## 匹配规则

### 函数体定位

- 优先使用 `nodes[].file` 在组件源码路径中定位文件并提取函数体
- 若同名文件存在多个路径，优先选择可成功提取函数体的文件
- 若仅凭 `file` 无法稳定提取，执行按函数名的兜底定位：在候选文件内搜索 `nodes[].name` 的函数定义（支持 `Class::Func` 与短名 `Func`），并优先选择可成功提取函数体的位置
- 若函数名在同一文件中命中多个定义且无法唯一判定，保留全部候选；由后续跨组件匹配阶段按规则输出并标记 `inferred`
- 若无法提取函数体，跳过该节点，不阻塞整体输出
- 注意：若传入的“源码路径”目录里只有 `callstack.json` 而没有真实源码文件，跨组件连边会为空

### 调用提取

- 识别 `foo(...)`、`ns::foo(...)`、`obj->foo(...)`、`obj.foo(...)` 样式
- 过滤注释与字符串中的伪命中
- 过滤关键字与控制语句（如 `if/for/while/switch/return/sizeof`）
- 宏调用不建边

### 跨组件匹配

- 仅在“调用者组件 != 被调组件”时建立跨组件边
- 优先使用完整名匹配（如 `Class::Func`），其次使用短名匹配（如 `Func`）
- 唯一匹配：`confidence=confirmed`
- 多候选匹配：为每个候选建边并标记 `confidence=inferred`

## 失败与回退策略

- 已传入组件的 callstack 解析失败：终止并报错（输入不完整时无法可靠聚合）
- 参数中出现 `src/callstack` 不成对：终止并报错
- 单个函数体解析失败：先尝试“按函数名兜底定位”，仍失败则记录到统计信息并跳过
- 同名多定义无法判定：全部保留并标记 `inferred`

## 质量自检清单

- 已传入组件节点是否全部进入聚合图
- `edges[].src/dst` 是否全部可回溯到 `nodes[].id`
- 跨组件边是否全部满足 `src.component != dst.component`
- `confidence` 是否仅为 `confirmed/inferred`
- 是否输出了 `stats.json` 便于快速评估质量
