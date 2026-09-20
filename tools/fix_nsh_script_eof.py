#!/usr/bin/env python3
"""修正固定NuttX 13.0.0的NSH脚本正常EOF退出码，重复构建不会重复修改。"""
import argparse
from pathlib import Path


def fix(apps: Path) -> None:
    """仅识别已知源码；正常EOF成功，解析错误及读取错误仍然失败。"""
    path = apps / "nshlib" / "nsh_script.c"
    source = path.read_text(encoding="utf-8")
    original = """          ret = readline_fd(buffer, LINE_MAX, vtbl->np.np_fd, -1);
          if (ret >= 0)"""
    replacement = """          /* CBoard: a normal EOF is a successful script completion. */

          errno = 0;
          ret = readline_fd(buffer, LINE_MAX, vtbl->np.np_fd, -1);
          if (ret < 0 && errno == 0)
            {
              ret = OK;
              break;
            }

          if (ret >= 0)"""
    if replacement in source:
        print("[OK] NSH normal EOF status fix is already applied.")
        return
    if source.count(original) != 1:
        raise RuntimeError("Unknown NSH script source; refusing an unverified modification.")
    source = source.replace("#include <fcntl.h>", "#include <errno.h>\n#include <fcntl.h>", 1)
    source = source.replace(original, replacement, 1)
    path.write_text(source, encoding="utf-8")
    print("[OK] NSH normal EOF now succeeds; command/read errors still fail.")


def main() -> None:
    """接收已下载的Apache apps路径并应用构建兼容修正。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apps", type=Path, required=True)
    fix(parser.parse_args().apps)


if __name__ == "__main__":
    main()
