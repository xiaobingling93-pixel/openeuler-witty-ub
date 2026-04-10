#!/usr/bin/env python3
"""Aggregate UB callstack graphs and connect cross-component function calls."""

from __future__ import annotations

import argparse
import json
import re
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Set, Tuple

SUPPORTED_COMPONENTS = ("ubsocket", "umq", "liburma", "libudma")
SOURCE_EXTS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hh", ".hxx"}
SKIP_TOKENS = {
    "if",
    "for",
    "while",
    "switch",
    "return",
    "sizeof",
    "catch",
    "new",
    "delete",
    "static_cast",
    "dynamic_cast",
    "reinterpret_cast",
    "const_cast",
}
CALL_PATTERN = re.compile(r"([A-Za-z_~][A-Za-z0-9_:~]*)\s*\(")


@dataclass(frozen=True)
class Node:
    node_id: str
    name: str
    component: str
    file: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Aggregate callstack graphs of selected UB components and add "
            "cross-component call edges by source-code body matching."
        )
    )

    for comp in SUPPORTED_COMPONENTS:
        parser.add_argument(f"--{comp}-src")
        parser.add_argument(f"--{comp}-callstack")

    parser.add_argument(
        "--output-dir",
        default="/var/witty-ub/callstack-analysis",
    )
    parser.add_argument(
        "--drop-existing-edges",
        action="store_true",
        help="Only keep newly discovered cross-component edges.",
    )
    return parser.parse_args()


def collect_component_inputs(
    args: argparse.Namespace,
) -> Tuple[List[str], Dict[str, Path], Dict[str, Path]]:
    selected_components: List[str] = []
    source_roots: Dict[str, Path] = {}
    callstack_paths: Dict[str, Path] = {}

    for comp in SUPPORTED_COMPONENTS:
        src_raw = getattr(args, f"{comp}_src")
        callstack_raw = getattr(args, f"{comp}_callstack")
        if bool(src_raw) != bool(callstack_raw):
            raise ValueError(
                f"component {comp} requires both --{comp}-src and --{comp}-callstack"
            )
        if not src_raw:
            continue
        selected_components.append(comp)
        source_roots[comp] = Path(src_raw).expanduser().resolve()
        callstack_paths[comp] = Path(callstack_raw).expanduser().resolve()

    if not selected_components:
        raise ValueError("at least one component pair (--<component>-src/--<component>-callstack) is required")

    return selected_components, source_roots, callstack_paths


def strip_comments_and_strings(code: str) -> str:
    result: List[str] = []
    i = 0
    n = len(code)
    in_sl_comment = False
    in_ml_comment = False
    in_string: Optional[str] = None

    while i < n:
        ch = code[i]
        nxt = code[i + 1] if i + 1 < n else ""

        if in_sl_comment:
            if ch == "\n":
                in_sl_comment = False
                result.append(ch)
            else:
                result.append(" ")
            i += 1
            continue

        if in_ml_comment:
            if ch == "*" and nxt == "/":
                in_ml_comment = False
                result.extend([" ", " "])
                i += 2
            else:
                result.append("\n" if ch == "\n" else " ")
                i += 1
            continue

        if in_string is not None:
            if ch == "\\":
                result.extend([" ", " "])
                i += 2
                continue
            if ch == in_string:
                in_string = None
            result.append("\n" if ch == "\n" else " ")
            i += 1
            continue

        if ch == "/" and nxt == "/":
            in_sl_comment = True
            result.extend([" ", " "])
            i += 2
            continue

        if ch == "/" and nxt == "*":
            in_ml_comment = True
            result.extend([" ", " "])
            i += 2
            continue

        if ch in {'"', "'"}:
            in_string = ch
            result.append(" ")
            i += 1
            continue

        result.append(ch)
        i += 1

    return "".join(result)


def find_source_files(root: Path) -> Dict[str, List[Path]]:
    mapping: Dict[str, List[Path]] = defaultdict(list)
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in SOURCE_EXTS:
            continue
        mapping[path.name].append(path)
    return mapping


def load_callstack(path: Path, expected_component: str) -> Tuple[List[Node], List[dict]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    nodes_data = data.get("nodes", [])
    edges_data = data.get("edges", [])

    nodes: List[Node] = []
    for raw in nodes_data:
        component = str(raw.get("component", "")).strip() or expected_component
        node = Node(
            node_id=str(raw["id"]),
            name=str(raw.get("name", "")),
            component=component,
            file=str(raw.get("file", "")),
        )
        nodes.append(node)
    return nodes, edges_data


def choose_candidate_file(candidates: Sequence[Path]) -> Optional[Path]:
    if not candidates:
        return None
    if len(candidates) == 1:
        return candidates[0]

    # Prefer shorter relative path (often closer to public source tree)
    sorted_candidates = sorted(candidates, key=lambda p: len(str(p)))
    return sorted_candidates[0]


def extract_brace_block(text: str, start_brace_index: int) -> Optional[str]:
    if start_brace_index < 0 or start_brace_index >= len(text) or text[start_brace_index] != "{":
        return None

    i = start_brace_index
    n = len(text)
    depth = 0
    in_string: Optional[str] = None
    in_sl_comment = False
    in_ml_comment = False

    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""

        if in_sl_comment:
            if ch == "\n":
                in_sl_comment = False
            i += 1
            continue

        if in_ml_comment:
            if ch == "*" and nxt == "/":
                in_ml_comment = False
                i += 2
            else:
                i += 1
            continue

        if in_string is not None:
            if ch == "\\":
                i += 2
                continue
            if ch == in_string:
                in_string = None
            i += 1
            continue

        if ch == "/" and nxt == "/":
            in_sl_comment = True
            i += 2
            continue

        if ch == "/" and nxt == "*":
            in_ml_comment = True
            i += 2
            continue

        if ch in {'"', "'"}:
            in_string = ch
            i += 1
            continue

        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[start_brace_index : i + 1]
        i += 1

    return None


def normalize_token(token: str) -> str:
    token = token.strip()
    if token.startswith("::"):
        token = token[2:]
    return token


def split_name_variants(name: str) -> Set[str]:
    name = name.strip()
    if not name:
        return set()
    parts = [p for p in name.split("::") if p]
    variants: Set[str] = {name}
    if parts:
        variants.add(parts[-1])
    return {v for v in variants if v}


def find_function_body_by_name(text: str, func_name: str) -> Optional[str]:
    cleaned = strip_comments_and_strings(text)
    variants = sorted(split_name_variants(func_name), key=len, reverse=True)

    for variant in variants:
        if not variant:
            continue
        pattern = re.compile(rf"{re.escape(variant)}\s*\(")
        for match in pattern.finditer(cleaned):
            start = match.start()
            prev = cleaned[start - 1] if start > 0 else ""
            if prev and (prev.isalnum() or prev in {"_", "~"}):
                continue

            open_paren = cleaned.find("(", start + len(variant))
            if open_paren < 0:
                continue

            depth = 0
            i = open_paren
            n = len(cleaned)
            while i < n:
                ch = cleaned[i]
                if ch == "(":
                    depth += 1
                elif ch == ")":
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            if i >= n or depth != 0:
                continue

            j = i + 1
            while j < n:
                ch = cleaned[j]
                if ch == ";":
                    break
                if ch == "{":
                    return extract_brace_block(text, j)
                j += 1

    return None


def build_callee_index(nodes: Iterable[Node]) -> Dict[str, List[Node]]:
    idx: Dict[str, List[Node]] = defaultdict(list)
    for node in nodes:
        for variant in split_name_variants(node.name):
            idx[variant].append(node)
    return idx


def infer_calls_from_body(body: str) -> Set[str]:
    cleaned = strip_comments_and_strings(body)
    tokens: Set[str] = set()
    for m in CALL_PATTERN.finditer(cleaned):
        raw = normalize_token(m.group(1))
        if not raw:
            continue
        short = raw.split("::")[-1]
        if short in SKIP_TOKENS:
            continue
        tokens.add(raw)
        tokens.add(short)
    return tokens


def ensure_edge_kind(edge: dict, default_kind: str) -> dict:
    copied = dict(edge)
    copied.setdefault("kind", default_kind)
    return copied


def main() -> int:
    args = parse_args()

    selected_components, source_roots, callstack_paths = collect_component_inputs(args)

    for comp in selected_components:
        if not source_roots[comp].exists():
            raise FileNotFoundError(f"source path not found for {comp}: {source_roots[comp]}")
        if not callstack_paths[comp].exists():
            raise FileNotFoundError(f"callstack not found for {comp}: {callstack_paths[comp]}")

    nodes_by_component: Dict[str, List[Node]] = {}
    all_nodes: List[Node] = []
    existing_edges: List[dict] = []

    for comp in selected_components:
        nodes, edges = load_callstack(callstack_paths[comp], comp)
        nodes_by_component[comp] = nodes
        all_nodes.extend(nodes)
        for edge in edges:
            existing_edges.append(ensure_edge_kind(edge, "intra_component"))

    node_by_id = {n.node_id: n for n in all_nodes}

    file_index_by_comp = {comp: find_source_files(root) for comp, root in source_roots.items()}
    callee_index_by_comp = {
        comp: build_callee_index(nodes)
        for comp, nodes in nodes_by_component.items()
    }

    cross_edges: List[dict] = []
    cross_edge_keys: Set[Tuple[str, str]] = set()
    resolved_bodies = 0
    unresolved_bodies = 0

    for caller in all_nodes:
        file_candidates = file_index_by_comp.get(caller.component, {}).get(caller.file, [])
        body: Optional[str] = None
        source_file: Optional[Path] = None
        for candidate in sorted(file_candidates, key=lambda p: len(str(p))):
            try:
                text = candidate.read_text(encoding="utf-8", errors="ignore")
            except OSError:
                continue
            candidate_body = find_function_body_by_name(text, caller.name)
            if candidate_body:
                body = candidate_body
                source_file = candidate
                break
        if source_file is None:
            source_file = choose_candidate_file(file_candidates)
        if not body:
            unresolved_bodies += 1
            continue

        resolved_bodies += 1
        called_tokens = infer_calls_from_body(body)
        if not called_tokens:
            continue

        for callee_comp in selected_components:
            if callee_comp == caller.component:
                continue

            idx = callee_index_by_comp[callee_comp]
            matched_nodes: Dict[str, Node] = {}
            for tok in called_tokens:
                for candidate in idx.get(tok, []):
                    matched_nodes[candidate.node_id] = candidate

            if not matched_nodes:
                continue

            confidence = "confirmed" if len(matched_nodes) == 1 else "inferred"
            for candidate in matched_nodes.values():
                key = (caller.node_id, candidate.node_id)
                if key in cross_edge_keys:
                    continue
                cross_edge_keys.add(key)
                cross_edges.append(
                    {
                        "src": caller.node_id,
                        "dst": candidate.node_id,
                        "confidence": confidence,
                        "kind": "cross_component",
                        "evidence": {
                            "caller_component": caller.component,
                            "callee_component": candidate.component,
                            "source_file": str(source_file),
                        },
                    }
                )

    merged_nodes = [
        {
            "id": n.node_id,
            "name": n.name,
            "component": n.component,
            "file": n.file,
        }
        for n in all_nodes
    ]

    if args.drop_existing_edges:
        merged_edges = cross_edges
    else:
        merged_edges = existing_edges + cross_edges

    # Remove dangling edges defensively.
    valid_ids = {n["id"] for n in merged_nodes}
    merged_edges = [
        e
        for e in merged_edges
        if e.get("src") in valid_ids and e.get("dst") in valid_ids
    ]

    stats = {
        "components": list(selected_components),
        "node_count": len(merged_nodes),
        "existing_edge_count": len(existing_edges),
        "cross_component_edge_count": len(cross_edges),
        "merged_edge_count": len(merged_edges),
        "resolved_function_bodies": resolved_bodies,
        "unresolved_function_bodies": unresolved_bodies,
        "cross_edges_by_pair": {},
    }

    pair_counter: Dict[str, int] = defaultdict(int)
    for e in cross_edges:
        src = node_by_id.get(e["src"])
        dst = node_by_id.get(e["dst"])
        if not src or not dst:
            continue
        pair = f"{src.component}->{dst.component}"
        pair_counter[pair] += 1
    stats["cross_edges_by_pair"] = dict(sorted(pair_counter.items()))

    output_dir = Path(args.output_dir).expanduser().resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    overall_callstack_path = output_dir / "overall_callstack.json"
    overall_callstack_path.write_text(
        json.dumps({"nodes": merged_nodes, "edges": merged_edges}, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    print(f"[done] output_dir={output_dir}")
    print(f"[done] overall_callstack={overall_callstack_path}")
    print(f"[done] node_count={stats['node_count']}")
    print(f"[done] merged_edge_count={stats['merged_edge_count']}")
    print(f"[done] cross_component_edge_count={stats['cross_component_edge_count']}")
    print(f"[done] resolved_function_bodies={stats['resolved_function_bodies']}")
    print(f"[done] unresolved_function_bodies={stats['unresolved_function_bodies']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
