#!/usr/bin/env python3
"""把项目 msg/*.msg 转换为轻量 C++ 消息结构体头文件。"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


TYPE_MAP = {
    "bool": "bool",
    "int8": "int8_t",
    "uint8": "uint8_t",
    "int16": "int16_t",
    "uint16": "uint16_t",
    "int32": "int32_t",
    "uint32": "uint32_t",
    "int64": "int64_t",
    "uint64": "uint64_t",
    "float32": "float",
    "float64": "double",
}

DECLARATION = re.compile(
    r"^(?P<type>[A-Za-z][A-Za-z0-9]*)(?:\[(?P<size>\d+)\])?\s+"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)(?:\s*=\s*(?P<value>.+))?$"
)


def parse_message(path: Path) -> list[tuple[str, str, str | None, str | None, str]]:
    """解析单个 msg 文件，返回类型、名称、数组长度、常量值和中文说明。"""
    declarations = []
    pending_comments: list[str] = []

    for line_number, original in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        stripped = original.strip()
        if not stripped:
            continue
        if stripped.startswith("#"):
            pending_comments.append(stripped[1:].strip())
            continue

        declaration_text, _, inline_comment = stripped.partition("#")
        match = DECLARATION.fullmatch(declaration_text.strip())
        if match is None or match.group("type") not in TYPE_MAP:
            raise ValueError(f"{path}:{line_number}: 不支持的消息声明: {original}")

        comment_parts = pending_comments
        if inline_comment.strip():
            comment_parts.append(inline_comment.strip())
        pending_comments = []
        declarations.append(
            (
                match.group("type"),
                match.group("name"),
                match.group("size"),
                match.group("value"),
                " ".join(comment_parts),
            )
        )

    return declarations


def generate_header(path: Path) -> str:
    """根据一个 msg 文件生成同名 C++ 结构体。"""
    declarations = parse_message(path)
    lines = [
        "/**",
        f" * @file {path.stem}.hpp",
        f" * @brief 由 msg/{path.name} 自动生成，请勿直接修改。",
        " */",
        "",
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "/**",
        f" * @struct {path.stem}",
        f" * @brief {path.stem} uORB 消息的数据结构。",
        " */",
        f"struct {path.stem}",
        "{",
    ]

    for type_name, name, array_size, value, comment in declarations:
        cpp_type = TYPE_MAP[type_name]
        if comment:
            lines.append(f"  /** {comment} */")
        if value is not None:
            if array_size is not None:
                raise ValueError(f"{path}: 常量不能声明为数组: {name}")
            lines.append(f"  static constexpr {cpp_type} {name} = {value};")
        elif array_size is not None:
            lines.append(f"  {cpp_type} {name}[{array_size}]{{}};")
        else:
            lines.append(f"  {cpp_type} {name}{{}};")

    lines.extend(["};", ""])
    return "\n".join(lines)


def write_if_changed(path: Path, content: str) -> None:
    """只在内容变化时写文件，避免无意义地触发全量重编译。"""
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return
    path.write_text(content, encoding="utf-8", newline="\n")


def main() -> int:
    """生成输入目录中全部 msg 文件，并删除已经失效的旧头文件。"""
    parser = argparse.ArgumentParser(description="生成 CBoard uORB C++ 消息头文件")
    parser.add_argument("--input", required=True, type=Path, help="msg 源文件目录")
    parser.add_argument("--output", required=True, type=Path, help="头文件输出目录")
    arguments = parser.parse_args()

    source_files = sorted(arguments.input.glob("*.msg"))
    if not source_files:
        raise RuntimeError(f"没有找到消息文件: {arguments.input}")

    arguments.output.mkdir(parents=True, exist_ok=True)
    expected = {f"{source.stem}.hpp" for source in source_files}
    for old_header in arguments.output.glob("*.hpp"):
        if old_header.name not in expected:
            old_header.unlink()

    for source in source_files:
        write_if_changed(arguments.output / f"{source.stem}.hpp", generate_header(source))

    print(f"[OK] Generated {len(source_files)} uORB message headers in {arguments.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
