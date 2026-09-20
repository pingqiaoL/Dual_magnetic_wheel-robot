#!/usr/bin/env python3
"""复制正式NSH脚本执行器，提供主机桩以验证EOF与命令失败处理。"""
import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[1]
fixture = root / "build/tests/nsh_eof"
fixture.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("eof_fix", root / "tools/fix_nsh_script_eof.py")
fixer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixer)
source = root / "upstream/apps/nshlib/nsh_script.c"
target = fixture / "nshlib/nsh_script.c"
target.parent.mkdir(exist_ok=True)
target.write_text(source.read_text(encoding="utf-8"), encoding="utf-8")
fixer.fix(fixture)
first = target.read_bytes()
fixer.fix(fixture)
assert target.read_bytes() == first, "EOF fix must be idempotent"
(fixture / "nuttx").mkdir(exist_ok=True)
(fixture / "system").mkdir(exist_ok=True)
(fixture / "nuttx/config.h").write_text("#define CONFIG_NSH_DISABLE_LOOPS 1\n")
(fixture / "nshlib/nsh_console.h").write_text("#include \"nsh.h\"\n")
(fixture / "system/readline.h").write_text(
    "#include <stddef.h>\nint readline_fd(char *, int, int, int);\n")
(fixture / "nshlib/nsh.h").write_text(r'''#pragma once
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#define FAR
#define OK 0
#define ERROR -1
#define LINE_MAX 128
#define NSH_PFLAG_SILENT 1
#define NSH_PFLAG_IGNORE 2
#define NSH_ERRNO errno
struct nsh_vtbl_s { struct { int np_fd; int np_flags; } np; };
extern const char *g_fmtcmdfailed;
char *nsh_getfullpath(struct nsh_vtbl_s *, const char *);
void nsh_freefullpath(char *);
char *nsh_linebuffer(struct nsh_vtbl_s *);
void nsh_error(struct nsh_vtbl_s *, const char *, ...);
void nsh_output(struct nsh_vtbl_s *, const char *, ...);
int nsh_parse(struct nsh_vtbl_s *, char *);
int nsh_script(struct nsh_vtbl_s *, const char *, const char *, bool);
''', encoding="utf-8")
print("[OK] Real NSH script executor prepared for host EOF regression test.")
