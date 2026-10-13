"""Source-based wiring plan: two USB devices, air telemetry, video, MAVLink."""
from pathlib import Path
from html import escape
import math
from PIL import Image, ImageDraw, ImageFont

OUT=Path(__file__).resolve().parent
W,H=3200,2340
im=Image.new('RGB',(W,H),'#FAFCFF');d=ImageDraw.Draw(im)
svg=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}"><rect width="100%" height="100%" fill="#FAFCFF"/>']
INK='#24384C';GRAY='#66778A';BLUE='#2476BC';USB='#118A9C';TEL='#C97929';MAV='#7753B0';RC='#2B8763';POWER='#B44C52'

def text(x,y,s,size=28,color=INK,bold=False,center=False):
 f=ImageFont.truetype('C:/Windows/Fonts/'+('msyhbd.ttc' if bold else 'msyh.ttc'),size)
 d.text((x,y),s,font=f,fill=color,anchor='mm' if center else 'lm')
 svg.append(f'<text x="{x}" y="{y}" font-family="Microsoft YaHei,sans-serif" font-size="{size}" fill="{color}" font-weight="{700 if bold else 400}" dominant-baseline="central" text-anchor="{"middle" if center else "start"}">{escape(s)}</text>')

def rect(x,y,w,h,fill='#FFFFFF',color=BLUE,r=22):
 d.rounded_rectangle((x,y,x+w,y+h),radius=r,fill=fill,outline=color,width=3)
 svg.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}" stroke="{color}" stroke-width="3"/>')

def box(x,y,w,h,title,lines=(),color=BLUE,fill='#EFF6FD'):
 rect(x,y,w,h,fill,color)
 text(x+w/2,y+43,title,35,color,True,True)
 for i,s in enumerate(lines):text(x+w/2,y+98+44*i,s,27,INK,False,True)

def wire(points,color=BLUE,both=False):
 d.line(points,fill=color,width=5,joint='curve')
 svg.append('<polyline points="'+' '.join(f'{x},{y}' for x,y in points)+f'" fill="none" stroke="{color}" stroke-width="5" stroke-linejoin="round"/>')
 def tip(p,q):
  x,y=p;a=math.atan2(y-q[1],x-q[0]);pts=[p,(x-22*math.cos(a)+10*math.sin(a),y-22*math.sin(a)-10*math.cos(a)),(x-22*math.cos(a)-10*math.sin(a),y-22*math.sin(a)+10*math.cos(a))]
  d.polygon(pts,fill=color);svg.append('<polygon points="'+' '.join(f'{x},{y}' for x,y in pts)+f'" fill="{color}"/>')
 tip(points[-1],points[-2])
 if both:tip(points[0],points[1])

text(65,65,'磁轮机器人 · 完整系统接线图',54,INK,True)
text(65,122,'方案：Type-C 采集 / UART 测厚数传 / LAN 视频 / UART MAVLink / SBUS 遥控',31,GRAY)

# Video channel on its own upper lane.
wire([(1535,300),(1535,205),(2715,205),(2715,300)],BLUE,True)
text(2130,174,'视频：RTSP / RTP，经两端专用转接统一成 RJ45',28,BLUE,False,True)

# Left USB instruments.
box(65,355,580,220,'USB 摄像头',['当前 500 万 USB 相机','输出格式通过 V4L2 查询','MJPEG / 原始图像需在 Orin 编码'],USB,'#ECF9FB')
box(65,735,580,220,'USB 测厚仪',['读取厂家 USB 协议','由 Orin 解析厚度、时间与状态','USB 类型 / 驱动按实机确认'],USB,'#ECF9FB')

# Central processing board and physical interfaces.
rect(1035,300,1035,835,'#FFFFFF',BLUE)
text(1552,350,'达妙 DM-ORIN NX 载板 + 核心模组',38,BLUE,True,True)
text(1552,406,'Linux 上位机：视频服务 + 测厚采集 + 数据桥接',29,GRAY,False,True)
box(1065,448,380,130,'Type-C USB Host ①',['摄像头输入'],USB,'#ECF9FB')
box(1065,767,380,130,'Type-C USB Host ②',['测厚仪输入'],USB,'#ECF9FB')
box(1490,448,550,164,'网口 LAN',['载板接口 → 达妙 RJ45 转接板','编码 / 转发视频 → RTSP 服务'],BLUE)
box(1490,648,550,164,'UART0：天空端数传',['解析测厚数据 → 打包 → 串口发送','TX / RX / GND，电平确认兼容'],TEL,'#FFF5E9')
box(1490,848,550,164,'UART1：下位机通讯',['MAVLink 指令下发 / 状态回传','3.3V TTL · TX / RX / GND'],MAV,'#F5EFFB')
text(1552,1076,'USB 口需处于 Host 模式；UART 设备名按系统确认',27,GRAY,False,True)

wire([(645,490),(1065,490)],USB)
text(855,455,'Type-C USB 数据线',29,USB,False,True)
wire([(645,832),(1065,832)],USB)
text(855,795,'Type-C USB 数据线',29,USB,False,True)

# Air unit and its two wired paths.
box(2390,300,650,600,'MK32 天空端 / 接收机',[
 'LAN & PWR：8-pin 网络 / 电源接口',
 '专用分线 → RJ45：连接 Orin 网口',
 '',
 'UART：4-pin 数传接口',
 '接 Orin UART0，传输测厚与状态',
 '',
 'S.Bus Out：遥控信号输出',
 'Type-C 升级口不参与上述连接',
 ],BLUE)
wire([(2070,725),(2390,725)],TEL,True)
text(2228,653,'UART 数传',30,TEL,True,True)
text(2228,686,'TX/RX 交叉 + GND',25,TEL,False,True)
text(2228,782,'不接 5V / NC',24,GRAY,False,True)

box(2390,1060,650,240,'MK32 遥控器 · 安卓 UI',[
 '视频：网络拉流 / 解码 / 主画面或小窗',
 '测厚：接入思翼数传接口 / SDK',
 '顶部状态 + 点击小窗切换布局',
 ],BLUE)
wire([(2715,900),(2715,1060)],BLUE,True)
text(2535,965,'无线链路',28,BLUE,True,True)
text(2535,1010,'视频 / 数传 / 遥控',24,BLUE,False,True)

# Orin to MCU serial, dedicated vertical lane.
wire([(1552,1135),(1552,1420)],MAV,True)
text(1855,1205,'UART1 / C 板空闲 UART',29,MAV,True,True)
text(1855,1255,'MAVLink：指令 / 心跳 / 状态',27,MAV,False,True)
text(1855,1300,'两端波特率一致；避开 NSH 和 SBUS 口',25,GRAY,False,True)

# Detailed pin assignment card.
rect(65,1160,900,950,'#FFFFFF',GRAY)
text(95,1206,'接线端子对照 · 只接信号与参考地',32,INK,True)
text(95,1272,'A. Orin UART0 / 天空端 UART',31,TEL,True)
for i,s in enumerate([
 'Orin 1：RX    ←    天空端 TX',
 'Orin 2：TX    →    天空端 RX',
 'Orin 3：GND  —    天空端 GND',
 'Orin 4：5V 不接；天空端 NC 不接',
]):text(105,1333+i*46,s,28)
text(95,1570,'B. Orin UART1 / C 板 UART6（建议）',30,MAV,True)
for i,s in enumerate([
 'Orin 1：RX    ←    C 板 2：TXD',
 'Orin 2：TX    →    C 板 3：RXD',
 'Orin 3：GND  —    C 板 1：GND',
 'Orin 4：5V 不接；UART6 为 3-pin 接口',
]):text(105,1631+i*46,s,28)
text(95,1870,'C 板外壳 UART1 标识对应 MCU UART6。',27,GRAY)
text(95,1920,'UART6 若已占用，改选空闲串口并重配驱动。',26,GRAY)
text(95,1984,'注意：天空端 UART 与 LAN 转接线虽可能',27,POWER)
text(95,2029,'同为 4-pin，功能不同，不可混插。',27,POWER)

# MCU, SBUS bypass, and actuator connection.
box(1100,1420,1000,380,'大疆 C 板 · STM32 / NuttX',[
 'UART6：接 Orin UART1，运行 MAVLink 模块（待接入）',
 'SBUS：沿用现有反相 / 电平适配与解析',
 '执行：解锁、失联策略、控制分配、CAN 输出',
 'GPIO：推杆继电器方向互锁、限位输入（待增加）',
 ],RC,'#EEF8F2')
wire([(3040,825),(3120,825),(3120,1470),(2100,1470)],RC)
text(2625,1418,'SBUS 直达 C 板：信号 + GND，NC 不接',25,RC,False,True)
text(2625,1374,'保留直接遥控路径',28,RC,True,True)

box(2390,1575,650,205,'执行器与传感器',[
 'CAN → 2 路动力 + 2 路转向电机',
 'GPIO → 继电器驱动板 → 电推杆',
 '触碰 / 行程开关 → GPIO 输入',
 ],RC,'#EEF8F2')
wire([(2100,1680),(2390,1680)],RC,True)
text(2245,1638,'CAN / GPIO',27,RC,False,True)
text(2545,1820,'继电器、电机另接功率电源',24,GRAY,False,True)

# Power must not be inferred from the data connectors.
rect(1100,1850,1940,260,'#FFF4F4',POWER)
text(1130,1897,'供电：电池 / 配电分别供电；图中通信线不能代替功率电源',31,POWER,True)
text(1130,1955,'Orin 载板：12–28V  → XT30；C 板：8–28V → 电源输入',28)
text(1130,2003,'天空端：使用对应硬件版本的原厂供电 / PMU；电机、推杆：额定电源',28)
text(1130,2054,'USB 外设电流须在载板输出预算内；串口 TX/RX 不接 5V、12V 或 RS485。',26,POWER)

text(65,2165,'线色：青色 USB 采集  ·  蓝色网络视频  ·  橙色天空端 UART 数传  ·  紫色 UART MAVLink  ·  绿色遥控与执行',29,GRAY)
text(65,2225,'软件边界：Orin 需提供视频服务、测厚协议解析与数传桥接；MK32 UI 需接入视频播放器和数传接口。',29,INK)
text(65,2284,'方案图 / 2026-10-12：USB 采集、串口设备名与波特率均需实机确认；不代表新增通讯模块已经实现。',27,GRAY)

svg.append('</svg>')
im.save(OUT/'full-system-wiring-usb-uart-lan-mavlink.png')
(OUT/'full-system-wiring-usb-uart-lan-mavlink.svg').write_text('\n'.join(svg),encoding='utf-8')
print('Generated full-system-wiring-usb-uart-lan-mavlink.png / .svg')
