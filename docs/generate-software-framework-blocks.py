"""Generate a source-verified nested-block architecture diagram (PNG + SVG)."""
from pathlib import Path
from html import escape
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent
W, H = 2800, 2170
GREEN = '#379E87'
INK = '#183D38'
MUTED = '#56716C'
LIGHT = '#F5FAF8'
LIME = '#DCECBA'
ORANGE = '#FFE0A4'
GRAY = '#E8ECEB'
im = Image.new('RGB', (W, H), 'white')
d = ImageDraw.Draw(im)
svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">', '<rect width="100%" height="100%" fill="white"/>']

def font(size, bold=False):
    return ImageFont.truetype('C:/Windows/Fonts/' + ('msyhbd.ttc' if bold else 'msyh.ttc'), size)

def txt(x, y, s, size=28, bold=False, color=INK, center=False, maxw=None):
    f = font(size, bold)
    while maxw and d.textlength(s, font=f) > maxw and size > 17:
        size -= 1
        f = font(size, bold)
    anchor = 'mm' if center else 'lm'
    d.text((x, y), s, font=f, fill=color, anchor=anchor)
    svg.append(f'<text x="{x}" y="{y}" font-family="Microsoft YaHei, sans-serif" font-size="{size}" font-weight="{700 if bold else 400}" text-anchor="{"middle" if center else "start"}" dominant-baseline="central" fill="{color}">{escape(s)}</text>')

def line(x1, y1, x2, y2, color=GREEN, width=3):
    d.line((x1,y1,x2,y2), fill=color, width=width)
    svg.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{width}"/>')

def rect(x,y,w,h,fill=LIGHT):
    d.rectangle((x,y,x+w,y+h),fill=fill,outline=GREEN,width=3)
    svg.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{fill}" stroke="{GREEN}" stroke-width="3"/>')

def group(x,y,w,h,title):
    # Dashed region border distinguishes architectural grouping from objects.
    for start in range(x, x+w, 22):
        d.line((start,y,min(start+12,x+w),y),fill=GREEN,width=2)
        d.line((start,y+h,min(start+12,x+w),y+h),fill=GREEN,width=2)
    for start in range(y,y+h,22):
        d.line((x,start,x,min(start+12,y+h)),fill=GREEN,width=2)
        d.line((x+w,start,x+w,min(start+12,y+h)),fill=GREEN,width=2)
    svg.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="none" stroke="{GREEN}" stroke-width="2" stroke-dasharray="12 10"/>')
    txt(x+18,y+29,title,27,True)

def box(x,y,w,h,title,sub='',fill=LIGHT):
    rect(x,y,w,h,fill)
    txt(x+w/2,y+h/2-(19 if sub else 0),title,29,True,center=True,maxw=w-26)
    if sub:
        txt(x+w/2,y+h/2+26,sub,23,color=MUTED,center=True,maxw=w-24)

txt(50,60,'CBoard 磁轮机器人 · 软件框架',52,True)
txt(1970,65,'当前源码  /  嵌套方框表示包含关系',25,color=MUTED)

group(50,125,2700,180,'接口层')
for x,w,a,b in [(80,470,'NSH 命令台','start / stop / status / param'),(590,490,'MK32 遥控输入','SBUS  ·  /dev/ttyS2'),(1120,530,'消息契约','msg/*.msg  →  C++ 消息类型'),(1690,490,'CAN 设备接口','/dev/can0'),(2220,500,'执行器反馈','ActuatorStatus')]:
    box(x,183,w,96,a,b)

group(50,340,380,1210,'工程构建与启动')
box(75,410,330,125,'Kconfig / defconfig','功能选择、板级裁剪')
box(75,570,330,125,'Makefile / Make.defs','NuttX 应用构建接入')
box(75,730,330,125,'tools/','构建 / 烧录 / 消息生成')
box(75,890,330,125,'rc.sysinit / rcS','系统初始化与模块启动')
box(75,1050,330,125,'rc.autostart','按 SYS_AUTOSTART 选构型')
box(75,1210,330,150,'1_dual_magneticwheel','set-default 默认参数',LIME)
txt(240,1425,'startup/ + config/',27,True,center=True)
txt(240,1472,'构型选择与业务实现分离',23,color=MUTED,center=True)

group(470,340,1770,1210,'robot/ 核心软件')
group(495,402,1720,425,'modules/ 运行模块  ·  并列任务，通过 uORB 通信')
for x,a,b in [(520,'SbusInput','读取与解析 SBUS'),(940,'RcUpdate','通道标定、映射、归一化'),(1360,'Command','上电 SA 互锁与解锁状态'),(1780,'Failsafe','检测遥控丢失、发布标志')]:
    box(x,465,405,132,a,b)
for x,a,b in [(520,'RobotControl','按安全策略生成 setpoint'),(1080,'ControlAllocator','选择模型并分配逻辑输出'),(1640,'RobotRuntime','核验模块启动与就绪状态')]:
    box(x,630,545,132,a,b)
txt(1355,799,'RcConfig 是 RcUpdate 的参数组件；CanOutput 位于 output/，也是独立任务。',23,color=MUTED,center=True)

group(495,853,840,402,'configuration/ + control/  构型与分配')
box(520,915,790,81,'RobotConfiguration','执行器类型与数量接口')
box(520,1013,790,91,'ActuatorEffectiveness','继承构型接口与 ModuleParams')
box(545,1122,490,108,'ActuatorEffectivenessDualMagneticWheel','子类：2 路动力 + 2 路转向',LIME)
box(1050,1122,260,108,'Allocation','线性矩阵分配')

group(1360,853,855,402,'output/ + protocol/can/  执行器输出')
rect(1385,915,805,237)
txt(1405,946,'CanOutput : OutputInterface',30,True)
txt(1405,987,'一个任务管理 4 路 CAN 执行器；按参数选择协议 / 模式 / ID',22,color=MUTED)
box(1405,1020,765,111,'MixingOutput  ·  CanOutput 持有的成员','FUNC 映射 / 解锁与数据有效性检查 / 回调 updateOutputs()',LIME)
box(1385,1170,805,60,'DamiaoProtocol  ·  达妙协议编码与反馈解码')

box(495,1284,1720,100,'msg/ + robot/orb/  ·  uORB 发布订阅总线','InputRc / ManualControl / FailsafeStatus / RobotControlSetpoint / ActuatorMotors / ActuatorServos',LIME)
box(495,1410,840,100,'common/ModuleBase<T>','模块生命周期与独立 NSH 命令入口')
box(1360,1410,855,100,'os/  操作系统适配','Task / Thread / Mutex / Semaphore / Clock')

group(2280,340,470,1210,'参数系统与验证工具')
box(2305,410,420,125,'ParamManager','全局参数值、校验与通知')
box(2305,565,420,125,'ModuleParams','模块与所属组件的参数树')
box(2305,720,420,125,'ParamInt / ParamFloat','参数注册、句柄与缓存')
box(2305,875,420,125,'FlashParamStorage','参数持久化后端')
box(2305,1030,420,125,'ParamDefinitions','名称、类型、默认值')
box(2305,1185,420,125,'tests/','分配 / 参数 / 安全 / 输出测试',GRAY)
box(2305,1340,420,150,'docs/','架构与接口、配置调试说明',GRAY)

group(50,1590,2700,215,'操作系统与板级驱动层')
for x,w,a,b in [(80,490,'NuttX RTOS','任务调度 / 同步 / 定时'),(610,490,'字符设备与文件系统','read / write / ioctl / ROMFS'),(1140,510,'platform/dji_cboard','启动、引脚、外设适配'),(1690,490,'UART / CAN 驱动','SBUS 输入与电机收发'),(2220,500,'Flash / 链接布局','ld.script 与参数存储区域')]:
    box(x,1654,w,116,a,b,ORANGE)

group(50,1845,2700,177,'硬件层')
for x,w,a,b in [(80,490,'STM32F407 · 大疆 C 板','控制器'),(610,490,'MK32 天空端','遥控接收'),(1140,510,'CAN 总线','设备通信'),(1690,490,'DM3510 × 2','前后动力轮 · 速度模式'),(2220,500,'DM4310 × 2','前后转向 · 位置速度模式')]:
    box(x,1903,w,93,a,b)

txt(50,2075,'图例',25,True)
rect(143,2060,32,28,LIME)
txt(191,2075,'核心组件',23)
rect(365,2060,32,28,ORANGE)
txt(414,2075,'系统与驱动',23)
rect(620,2060,32,28,GRAY)
txt(671,2075,'开发支撑',23)
txt(1050,2075,'虚线框：职责分组   实线框：模块或组件   嵌套框：对象包含',24,color=MUTED)
txt(50,2128,'遥控丢失策略：Failsafe 发布故障标志 → RobotControl 置零动力目标并保持转向目标；控制分配继续负责输出分配。',25,color=MUTED)

svg.append('</svg>')
im.save(ROOT / 'current-software-framework.png')
(ROOT / 'current-software-framework.svg').write_text('\n'.join(svg),encoding='utf-8')
print('Generated current-software-framework.png and .svg')
