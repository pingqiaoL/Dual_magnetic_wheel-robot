"""Render the current CBoard software architecture as a high-resolution PNG."""

from math import atan2, cos, sin
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent
OUTPUT = ROOT / "current-software-architecture.png"
FONT_REGULAR = r"C:\Windows\Fonts\msyh.ttc"
FONT_BOLD = r"C:\Windows\Fonts\msyhbd.ttc"
W, H = 3200, 1900
image = Image.new("RGB", (W, H), "#f7f9fc")
draw = ImageDraw.Draw(image)

def font(size, bold=False):
    return ImageFont.truetype(FONT_BOLD if bold else FONT_REGULAR, size)

TITLE, SUBTITLE = font(48, True), font(25)
LANE_TITLE, BOX_TITLE, BOX_DETAIL = font(27, True), font(27, True), font(21)
EDGE_LABEL, LEGEND = font(18), font(20)
COLORS = {
    "input": ("#e7f4fd", "#2684b5"), "safety": ("#fff0e6", "#df6d2d"),
    "control": ("#eaf7ec", "#459251"), "output": ("#f1ebfb", "#7955a6"),
    "param": ("#e6f7f3", "#2f887a"), "base": ("#eef1f5", "#687487"),
    "hardware": ("#e8eef8", "#4b6f9f"),
}

def centered_text(x, y, text, used_font, fill, spacing=5):
    bounds = draw.multiline_textbbox((0, 0), text, font=used_font, spacing=spacing, align="center")
    tw, th = bounds[2] - bounds[0], bounds[3] - bounds[1]
    draw.multiline_text((x - tw / 2, y - th / 2), text, font=used_font,
                        fill=fill, spacing=spacing, align="center")

def lane(rect, title, edge_color):
    draw.rounded_rectangle(rect, radius=24, fill="#ffffff", outline=edge_color, width=3)
    draw.text((rect[0] + 26, rect[1] + 18), title, font=LANE_TITLE, fill=edge_color)

def box(name, rect, title, detail, kind):
    fill, edge_color = COLORS[kind]
    draw.rounded_rectangle(rect, radius=22, fill=fill, outline=edge_color, width=4)
    x1, y1, x2, y2 = rect
    centered_text((x1+x2)/2, y1+(y2-y1)*0.38, title, BOX_TITLE, "#1d2a38")
    centered_text((x1+x2)/2, y1+(y2-y1)*0.73, detail, BOX_DETAIL, "#344556", 4)
    return {"name": name, "rect": rect}

def anchor(item, side):
    x1, y1, x2, y2 = item["rect"]
    if side == "l": return x1, (y1+y2)/2
    if side == "r": return x2, (y1+y2)/2
    if side == "t": return (x1+x2)/2, y1
    return (x1+x2)/2, y2

def arrow_head(p1, p2, color, size=18):
    angle = atan2(p2[1]-p1[1], p2[0]-p1[0])
    left = (p2[0]-size*cos(angle-0.55), p2[1]-size*sin(angle-0.55))
    right = (p2[0]-size*cos(angle+0.55), p2[1]-size*sin(angle+0.55))
    draw.polygon([p2, left, right], fill=color)

def edge(a, b, a_side="r", b_side="l", label="", color="#3578a8",
         via=None, dashed=False, label_at=None):
    points = [anchor(a, a_side)] + (via or []) + [anchor(b, b_side)]
    if dashed:
        for p1, p2 in zip(points, points[1:]):
            dx, dy = p2[0]-p1[0], p2[1]-p1[1]
            length = max((dx*dx+dy*dy)**0.5, 1)
            ux, uy, pos = dx/length, dy/length, 0
            while pos < length:
                end = min(pos+14, length)
                draw.line([(p1[0]+ux*pos, p1[1]+uy*pos),
                           (p1[0]+ux*end, p1[1]+uy*end)], fill=color, width=4)
                pos += 24
    else:
        draw.line(points, fill=color, width=5, joint="curve")
    arrow_head(points[-2], points[-1], color)
    if label:
        if label_at is None:
            p1, p2 = points[len(points)//2-1], points[len(points)//2]
            label_at = ((p1[0]+p2[0])/2, (p1[1]+p2[1])/2)
        bounds = draw.multiline_textbbox((0, 0), label, font=EDGE_LABEL, spacing=3, align="center")
        tw, th = bounds[2]-bounds[0], bounds[3]-bounds[1]
        x, y = label_at
        draw.rounded_rectangle((x-tw/2-8, y-th/2-5, x+tw/2+8, y+th/2+5), radius=8, fill="#ffffff")
        centered_text(x, y, label, EDGE_LABEL, color, 3)

centered_text(W/2, 62, "CBoard 当前软件框架", TITLE, "#17365d")
centered_text(W/2, 120, "双磁轮机器人 · NuttX · 进程内 uORB · 参数化通用执行器输出", SUBTITLE, "#53677d")
lane((40,165,3160,660), "① 遥控输入、失效检测与安全目标", "#2684b5")
lane((40,690,3160,1260), "② 构型分配、公共输出层与 CAN 驱动", "#459251")
lane((40,1290,3160,1800), "③ 启动、参数与公共基础层", "#687487")

mk32 = box("mk32", (90,315,390,485), "MK32 接收机", "SBUS", "hardware")
sbus = box("sbus", (470,295,830,505), "SbusInput", "/dev/ttyS2\n帧解析 / 500 ms 丢失检测", "input")
rc = box("rc", (920,295,1320,505), "RcUpdate", "校准 · 归一化\n通道功能映射", "input")
failsafe = box("failsafe", (1430,205,1770,375), "Failsafe", "只发布 RC 故障标志", "safety")
command = box("command", (1390,430,1810,600), "Command + ArmSafety", "SA 上电 OFF 互锁", "safety")
robot_control = box("robot_control", (1920,285,2330,515), "RobotControl", "订阅手动量与故障标志\n按 FS_* 生成速度 / 转向目标", "safety")
edge(mk32, sbus, label="SBUS\n100000 8E2")
edge(sbus, rc, label="input_rc")
edge(rc, failsafe, "r", "l", "manual_control", via=[(1370,400),(1370,290)], label_at=(1370,325))
edge(rc, command, "r", "l", "manual_control_switches", via=[(1370,400),(1370,515)], label_at=(1250,555))
edge(rc, robot_control, "r", "l", "manual_control", via=[(1835,400)], color="#df6d2d", label_at=(1810,430))
edge(failsafe, robot_control, "r", "t", "failsafe_status", via=[(1850,290),(1850,240),(2125,240)], color="#c64a32", label_at=(1850,260))

allocator = box("allocator", (100,855,490,1070), "ControlAllocator", "选择 CA_AIRFRAME\n检查目标新鲜度", "control")
effect = box("effect", (590,735,1050,920), "ActuatorEffectiveness", "双磁轮构型：2 Motor + 2 Servo", "control")
allocation = box("allocation", (590,1010,1050,1185), "Allocation", "a = M × [speed, steering]", "control")
mixing = box("mixing", (1190,835,1640,1090), "MixingOutput", "OutputFunction 路由\n解锁 / 新鲜度 / 硬故障处理", "output")
canout = box("canout", (1740,835,2110,1090), "CanOutput", "4 个物理槽位\n协议 / ID / 模式 / 反馈", "output")
protocol = box("protocol", (2210,835,2570,1090), "DamiaoProtocol", "速度 / 位置速度帧\n管理帧与反馈解码", "output")
motors = box("motors", (2680,820,3080,1105), "达妙 CAN 电机", "DM3510 ×2 动力轮\nDM4310 ×2 转向轮", "hardware")
edge(robot_control, allocator, "b", "t", "robot_control_setpoint", color="#df6d2d", via=[(2125,625),(295,625)], label_at=(1140,625))
edge(effect, allocation, "b", "t", "CA_THR_* / CA_YAW_*", color="#459251")
edge(allocator, allocation, "r", "l", "speed / steering", color="#459251")
edge(allocation, mixing, "r", "l", "actuator_motors\nactuator_servos", color="#459251")
edge(command, mixing, "b", "t", "actuator_armed", color="#c64a32", via=[(1600,650),(1415,650)], label_at=(1510,650))
edge(mixing, canout, label="OutputInterface\nupdateOutputs()", color="#7955a6")
edge(canout, protocol, label="按槽位配置", color="#7955a6")
edge(protocol, motors, label="/dev/can0", color="#7955a6")
edge(motors, canout, "b", "b", "CAN 反馈 → actuator_status", color="#55708f", via=[(2880,1180),(1925,1180)], label_at=(2400,1180))

startup = box("startup", (80,1430,520,1650), "rcS + rc.autostart", "启动 7 个业务任务\n选择 1_dual_magneticwheel", "base")
runtime = box("runtime", (600,1430,1000,1650), "RobotRuntime", "模块就绪检查 · 低频提示\n参数自动保存", "base")
params = box("params", (1080,1430,1470,1650), "ParamManager", "参数树 · generation\n发布 parameter_update", "param")
flash = box("flash", (1550,1430,1930,1650), "FlashParamStorage", "A/B 双扇区 · CRC\n原子提交", "param")
uorb = box("uorb", (2010,1430,2380,1650), "进程内 uORB", "Topic · Publication\nSubscription", "base")
nuttx = box("nuttx", (2460,1420,3100,1660), "NuttX 与公共封装", "ModuleBase · os::Task · Mutex\nUART · CAN · progmem · pthread", "base")
edge(startup, runtime, label="启动 / 监视", color="#687487")
edge(startup, params, "r", "l", "构型默认参数", color="#2f887a", via=[(780,1375),(1275,1375)], label_at=(1025,1375))
edge(params, flash, label="load / save", color="#2f887a")
edge(params, rc, "t", "b", "parameter_update", color="#2f887a", dashed=True, via=[(1275,1330),(1120,1330),(1120,560)], label_at=(1200,1300))
edge(params, allocator, "t", "b", "分配参数", color="#2f887a", dashed=True, via=[(1275,1280),(295,1280)], label_at=(820,1280))
edge(params, canout, "t", "b", "CAN_*", color="#2f887a", dashed=True, via=[(1275,1250),(1925,1250)], label_at=(1600,1250))
edge(uorb, mixing, "t", "b", "消息总线", color="#687487", dashed=True, via=[(2195,1320),(1415,1320)], label_at=(1810,1320))
edge(nuttx, canout, "t", "b", "驱动系统调用", color="#687487", dashed=True, via=[(2780,1350),(1925,1350)], label_at=(2350,1350))

draw.line((120,1735,260,1735), fill="#3578a8", width=5)
draw.text((280,1718), "控制 / 数据流", font=LEGEND, fill="#425466")
draw.line((560,1735,700,1735), fill="#c64a32", width=5)
draw.text((720,1718), "安全状态", font=LEGEND, fill="#425466")
for x in range(940,1080,24): draw.line((x,1735,min(x+14,1080),1735), fill="#2f887a", width=5)
draw.text((1100,1718), "参数 / 公共依赖", font=LEGEND, fill="#425466")
draw.text((3090,1718), "当前实现：CAN / 达妙；PWM 预留在 OutputInterface + MixingOutput 框架中", font=LEGEND, fill="#53677d", anchor="ra")

image.save(OUTPUT, quality=95)
print(OUTPUT)
