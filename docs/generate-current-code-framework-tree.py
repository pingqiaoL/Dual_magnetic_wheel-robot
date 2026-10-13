from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


HERE = Path(__file__).resolve().parent
OUT = HERE / "current-code-framework-tree.png"

W, H = 3200, 3600
BG = "#F4F7FB"
NAVY = "#143B63"
BLUE = "#1F6FB2"
CYAN = "#E8F5FD"
GREEN = "#2E7D5B"
GREEN_BG = "#EAF7F0"
ORANGE = "#C76522"
ORANGE_BG = "#FFF3E8"
PURPLE = "#7550A4"
PURPLE_BG = "#F3ECFA"
GRAY = "#5D6B78"
LINE = "#AFC2D4"
WHITE = "#FFFFFF"


def font(size, bold=False):
    name = "msyhbd.ttc" if bold else "msyh.ttc"
    return ImageFont.truetype(str(Path("C:/Windows/Fonts") / name), size)


F_TITLE = font(72, True)
F_SUB = font(30)
F_ROOT = font(38, True)
F_HEAD = font(31, True)
F_BODY = font(25)
F_SMALL = font(22)
F_FOOT = font(27, True)


img = Image.new("RGB", (W, H), BG)
d = ImageDraw.Draw(img)


def round_box(xy, fill, outline, radius=28, width=4, shadow=True):
    x1, y1, x2, y2 = xy
    if shadow:
        d.rounded_rectangle((x1 + 10, y1 + 12, x2 + 10, y2 + 12), radius, fill="#DDE5EE")
    d.rounded_rectangle(xy, radius, fill=fill, outline=outline, width=width)


def text(x, y, value, f=F_BODY, fill=NAVY, anchor="la"):
    d.text((x, y), value, font=f, fill=fill, anchor=anchor)


def connector(points, color=LINE, width=6):
    d.line(points, fill=color, width=width, joint="curve")


def card(x, y, w, h, title, path, lines, color, fill):
    round_box((x, y, x + w, y + h), fill, color)
    d.rounded_rectangle((x, y, x + w, y + 62), 26, fill=color)
    d.rectangle((x, y + 34, x + w, y + 62), fill=color)
    text(x + 24, y + 31, title, F_HEAD, WHITE, "lm")
    text(x + 24, y + 91, path, F_SMALL, color)
    yy = y + 134
    for line in lines:
        text(x + 26, yy, line, F_BODY, NAVY)
        yy += 43


# Header
d.rectangle((0, 0, W, 190), fill=WHITE)
d.rectangle((0, 184, W, 190), fill=BLUE)
text(120, 74, "CBoard 当前代码框架树", F_TITLE, NAVY, "lm")
text(120, 139, "按 PX4 风格拆分：板级与构建 · 启动与契约 · 业务模块 · 通用输出 · 工程支撑", F_SUB, GRAY, "lm")

# Root
round_box((1080, 240, 2120, 360), WHITE, NAVY, radius=32)
text(1600, 286, "_code/", F_ROOT, NAVY, "mm")
text(1600, 329, "CBoard / NuttX 机器人控制工程", F_SUB, GRAY, "mm")

# Main branch connector
connector([(1600, 360), (1600, 430)])
connector([(420, 430), (2780, 430)])
for x in (420, 1160, 2040, 2780):
    connector([(x, 430), (x, 485)])

# Row 1
card(80, 485, 680, 430, "1  板级与构建", "platform/dji_cboard/ + config/", [
    "configs/robot/defconfig  板级裁剪",
    "Kconfig / Make.defs      编译选择",
    "board.h / ld.script      引脚与内存",
    "stm32_boot/bringup       启动与挂载",
    "stm32_can/led/buttons    外设适配",
], BLUE, CYAN)

card(820, 485, 680, 430, "2  启动与构型", "startup/", [
    "rc.sysinit        系统级初始化",
    "rcS              工程启动入口",
    "rc.autostart     选择机器人编号",
    "1_dual_magneticwheel",
    "  默认参数 + 模块启动顺序",
], ORANGE, ORANGE_BG)

card(1560, 485, 680, 430, "3  消息与协议", "msg/ + protocol/can/", [
    "*.msg             数据契约源文件",
    "generate_messages 生成 C++ 类型",
    "CanProtocol       CAN 协议接口",
    "DamiaoProtocol    达妙帧编解码",
    "业务状态与字节协议相互隔离",
], PURPLE, PURPLE_BG)

card(2300, 485, 820, 430, "4  工程支撑", "tools/ + tests/ + docs/", [
    "tools      环境检查、构建、烧录",
    "tests      主机单测与结构验证",
    "docs       架构、接口、调试说明",
    "upstream   固定版 NuttX / apps",
    "artifacts  固件、map、构建记录",
], GREEN, GREEN_BG)

# robot root
connector([(1160, 915), (1160, 990)])
round_box((240, 990, 2960, 1120), WHITE, NAVY, radius=32)
text(320, 1030, "robot/", F_ROOT, NAVY)
text(565, 1039, "与具体 STM32 板卡解耦的 C++ 业务层", F_SUB, GRAY)
text(2860, 1054, "核心", F_HEAD, BLUE, "rm")

# robot branch rows
connector([(1600, 1120), (1600, 1180)])
connector([(440, 1180), (2760, 1180)])
for x in (440, 1020, 1600, 2180, 2760):
    connector([(x, 1180), (x, 1235)])

card(80, 1235, 700, 410, "基础设施", "common/ + os/", [
    "ModuleBase<T>  模块生命周期父类",
    "Task / Thread  NuttX 任务封装",
    "Mutex / Semaphore  同步原语",
    "Clock          单调时钟与延时",
    "上层代码不直接散用 NuttX API",
], BLUE, CYAN)

card(810, 1235, 700, 410, "发布订阅", "orb/", [
    "uORB.hpp       发布/订阅模板",
    "Topics         全局主题实例",
    "InputRc / ManualControl",
    "Setpoint / Actuator / Failsafe",
    "ParameterUpdate  参数变更通知",
], PURPLE, PURPLE_BG)

card(1540, 1235, 700, 410, "参数系统", "params/", [
    "ParamDefinitions  元数据与默认值",
    "ParamManager      查找/读写/校验",
    "ModuleParams      父子对象树",
    "FlashParamStorage 双页持久化",
    "param_main        NSH 参数命令",
], GREEN, GREEN_BG)

card(2270, 1235, 850, 410, "遥控输入", "drivers/rc/", [
    "sbus             解码 16 路通道",
    "SbusInput        读取 /dev/ttyS2",
    "RcConfig         通道标定与映射",
    "RcUpdate         归一化并发布状态",
    "连接状态通过 uORB 提供给上层",
], ORANGE, ORANGE_BG)

# modules expanded section
connector([(1600, 1645), (1600, 1730)])
round_box((80, 1730, 3120, 2350), WHITE, NAVY, radius=34)
text(140, 1792, "modules/  可启动任务", F_ROOT, NAVY, "lm")
text(2930, 1792, "均由 ModuleBase<T> 管理", F_SUB, GRAY, "rm")

module_cards = [
    (140, 1850, "RobotRuntime", "启动编排\n运行状态/就绪状态"),
    (645, 1850, "Command", "订阅开关状态\n生成解锁命令"),
    (1150, 1850, "Failsafe", "判断 RC/系统状态\n只发布 fail 标志"),
    (1655, 1850, "RobotControl", "订阅人工输入/安全位\n生成控制目标"),
    (2160, 1850, "ControlAllocator", "选择构型模型\n分配逻辑执行器"),
    (2665, 1850, "CanOutput", "订阅执行器输出\n调用公共输出链"),
]
for x, y, name, desc in module_cards:
    round_box((x, y, x + 395, y + 255), CYAN if name not in ("Failsafe", "CanOutput") else (ORANGE_BG if name == "Failsafe" else GREEN_BG), BLUE if name not in ("Failsafe", "CanOutput") else (ORANGE if name == "Failsafe" else GREEN), radius=24, width=3)
    text(x + 197, y + 49, name, F_HEAD, NAVY, "mm")
    yy = y + 102
    for line in desc.split("\n"):
        text(x + 197, yy, line, F_SMALL, GRAY, "mm")
        yy += 44

# Arrowed logical chain
for x in (535, 1040, 1545, 2050, 2555):
    connector([(x, 1977), (x + 90, 1977)], BLUE, 5)
    d.polygon([(x + 90, 1977), (x + 68, 1965), (x + 68, 1989)], fill=BLUE)
text(1600, 2185, "消息经 uORB 解耦；模块之间不直接持有彼此的具体对象", F_SUB, GRAY, "mm")
text(1600, 2240, "Failsafe 发布状态 → RobotControl 决定油门置零、姿态保持 → 分配器只做分配", F_SUB, ORANGE, "mm")

# control/config and output composition
connector([(1600, 2350), (1600, 2430)])
connector([(760, 2430), (2440, 2430)])
connector([(760, 2430), (760, 2490)])
connector([(2440, 2430), (2440, 2490)])

card(80, 2490, 1450, 480, "控制分配与构型模型", "configuration/ + control/", [
    "RobotConfiguration                 执行器类型/数量",
    "ActuatorEffectiveness              构型有效性接口",
    "ActuatorEffectivenessDualMagneticWheel",
    "                                     当前磁轮构型子类",
    "Allocation                         根据矩阵分配输出",
    "ControlAllocator 持有模型和算法；驱动不认识机器人构型",
], BLUE, CYAN)

card(1670, 2490, 1450, 480, "通用输出链", "output/", [
    "OutputFunction                     逻辑功能编号",
    "OutputInterface                    具体驱动回调接口",
    "MixingOutput                       FUNC 映射/安全/限幅",
    "CanOutput : OutputInterface        CAN 输出后端",
    "CanOutput 拥有 MixingOutput，并按参数选择协议、模式和 ID",
    "未来 PWMOutput 可复用同一 MixingOutput 公共处理",
], GREEN, GREEN_BG)

# dependency band
round_box((80, 3060, 3120, 3415), WHITE, NAVY, radius=34)
text(140, 3122, "主依赖方向", F_ROOT, NAVY, "lm")

nodes = [
    (260, "startup/config"),
    (800, "modules"),
    (1280, "msg + uORB"),
    (1780, "control"),
    (2220, "output"),
    (2660, "protocol"),
    (3020, "BSP/NuttX"),
]
for i, (x, label) in enumerate(nodes):
    text(x, 3240, label, F_FOOT, BLUE if i < 5 else GREEN, "mm")
    if i < len(nodes) - 1:
        nx = nodes[i + 1][0]
        connector([(x + 100, 3240), (nx - 105, 3240)], LINE, 5)
        d.polygon([(nx - 105, 3240), (nx - 128, 3228), (nx - 128, 3252)], fill=LINE)

text(1600, 3330, "params 为横向服务  ·  hardware-specific 代码收敛在 platform 与驱动边界", F_SUB, GRAY, "mm")
text(1600, 3488, "图中内容由当前源码目录生成 · docs/current-code-framework-tree.md", F_SMALL, GRAY, "mm")

img.save(OUT, quality=95)
print(OUT)
