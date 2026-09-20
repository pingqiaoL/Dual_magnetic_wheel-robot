/** @file nsh_script_eof_test.c
 * @brief 编译正式nsh_script.c，验证正常EOF成功、命令错误和读错误失败。
 */
#include "nshlib/nsh.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/** 测试桩计数和输入控制，仅替换解析器及字符读取依赖。 */
static unsigned parsed;
static int read_error;
static char line[LINE_MAX];
const char *g_fmtcmdfailed = "%s %s %d";

/** 为实际脚本打开流程提供可释放路径。 */
char *nsh_getfullpath(struct nsh_vtbl_s *v, const char *path)
{
  (void)v;
  char *copy = malloc(strlen(path) + 1);
  if (copy) { strcpy(copy, path); }
  return copy;
}
/** 释放路径。 */
void nsh_freefullpath(char *path) { free(path); }
/** 提供脚本行缓冲。 */
char *nsh_linebuffer(struct nsh_vtbl_s *v) { (void)v; return line; }
/** 输出不影响测试行为。 */
void nsh_error(struct nsh_vtbl_s *v, const char *f, ...) { (void)v; (void)f; }
/** 输出不影响测试行为。 */
void nsh_output(struct nsh_vtbl_s *v, const char *f, ...) { (void)v; (void)f; }
/** 用fail命令模拟解析失败，其余命令成功。 */
int nsh_parse(struct nsh_vtbl_s *v, char *text)
{
  (void)v;
  ++parsed;
  return strncmp(text, "fail", 4) == 0 ? -1 : 0;
}
/** 保持正式readline的EOF与errno语义，可注入读取错误。 */
int readline_fd(char *buffer, int size, int fd, int out)
{
  (void)out;
  if (read_error) { errno = EIO; return EOF; }
  int count = 0;
  while (count < size - 1)
    {
      char ch;
      const ssize_t n = read(fd, &ch, 1);
      if (n <= 0) { buffer[count] = '\0'; return count ? count : EOF; }
      buffer[count++] = ch;
      if (ch == '\n') { break; }
    }
  buffer[count] = '\0';
  return count;
}
/** 将脚本写到测试临时文件并调用正式执行器。 */
static int execute(const char *contents)
{
  const char *path = "build/tests/nsh_eof/script.nsh";
  FILE *file = fopen(path, "wb");
  assert(file);
  fputs(contents, file);
  fclose(file);
  struct nsh_vtbl_s v = {{-7, NSH_PFLAG_SILENT}};
  parsed = 0;
  const int result = nsh_script(&v, "source", path, true);
  assert(v.np.np_fd == -7);
  return result;
}
/** 验证完整脚本执行循环，而非复制修正条件表达式。 */
int main(void)
{
  assert(execute("ok\nready\n") == 0 && parsed == 2);
  assert(execute("") == 0 && parsed == 0);
  assert(execute("fail\nready\n") < 0 && parsed == 1);
  read_error = 1;
  assert(execute("ready\n") < 0 && parsed == 0);
  puts("real NSH source: normal EOF succeeds, command/read failures stop startup");
  return 0;
}
