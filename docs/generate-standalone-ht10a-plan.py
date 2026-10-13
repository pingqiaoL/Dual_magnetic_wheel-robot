from pathlib import Path
from html import escape
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent
W,H = 2600,1640
im=Image.new('RGB',(W,H),'white')
d=ImageDraw.Draw(im)
svg=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}"><rect width="100%" height="100%" fill="white"/>']
BLUE='#176EB2'; GREEN='#23816B'; GRAY='#66778A'; INK='#20354B'; ORANGE='#C87B20'

def txt(x,y,t,size=30,color=INK,center=False,bold=False):
    f=ImageFont.truetype('C:/Windows/Fonts/'+('msyhbd.ttc' if bold else 'msyh.ttc'),size)
    d.text((x,y),t,font=f,fill=color,anchor='mm' if center else 'lm')
    svg.append(f'<text x="{x}" y="{y}" font-family="Microsoft YaHei,sans-serif" font-size="{size}" fill="{color}" font-weight="{700 if bold else 400}" text-anchor="{"middle" if center else "start"}" dominant-baseline="central">{escape(t)}</text>')

def frame(x,y,w,h,title,color):
    d.rounded_rectangle((x,y,x+w,y+h),radius=22,outline=color,width=3)
    svg.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="22" fill="none" stroke="{color}" stroke-width="3"/>')
    txt(x+24,y+38,title,34,color,bold=True)

def box(x,y,w,h,title,lines=(),color=BLUE,fill='#EDF6FD'):
    d.rounded_rectangle((x,y,x+w,y+h),radius=18,fill=fill,outline=color,width=3)
    svg.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="18" fill="{fill}" stroke="{color}" stroke-width="3"/>')
    txt(x+w/2,y+38,title,31,color,True,True)
    for i,t in enumerate(lines):txt(x+w/2,y+84+i*40,t,25,INK,True)

def arrow(points,label='',color=BLUE,dash=False,lpos=None):
    for (x1,y1),(x2,y2) in zip(points,points[1:]):
        if dash:
            length=((x2-x1)**2+(y2-y1)**2)**.5
            for k in range(0,int(length),22):
                a=k/max(length,1);b=min(k+12,length)/max(length,1)
                d.line((x1+(x2-x1)*a,y1+(y2-y1)*a,x1+(x2-x1)*b,y1+(y2-y1)*b),fill=color,width=3)
        else:d.line((x1,y1,x2,y2),fill=color,width=4)
    x,y=points[-1];px,py=points[-2]
    import math
    a=math.atan2(y-py,x-px)
    tri=[(x,y),(x-19*math.cos(a)+9*math.sin(a),y-19*math.sin(a)-9*math.cos(a)),(x-19*math.cos(a)-9*math.sin(a),y-19*math.sin(a)+9*math.cos(a))]
    d.polygon(tri,fill=color)
    p=' '.join(f'{x},{y}' for x,y in points)
    dashattr='stroke-dasharray="12 10"' if dash else ''
    svg.append(f'<polyline points="{p}" fill="none" stroke="{color}" stroke-width="4" {dashattr}/>')
    svg.append('<polygon points="'+' '.join(f'{x},{y}' for x,y in tri)+f'" fill="{color}"/>')
    if label and lpos:txt(*lpos,label,25,color,True)

txt(55,58,'磁轮机器人 · 独立遥控与录像版',50,bold=True)
txt(55,111,'方案版：HT-10A + CBoard + 独立网络摄像头；不配置 Nano / Orin 与 MK32',28,GRAY)
frame(50,160,2500,550,'① 运动控制链  ·  使用支持 SBUS 的匹配接收机',BLUE)
box(85,295,410,165,'HT-10A 遥控器',['摇杆：前进后退 / 转向','开关：解锁 / 推杆操作'])
box(690,295,430,165,'匹配接收机',['输出接口需确认','优先 SBUS，复用现有解析'])
box(1370,255,700,290,'大疆 C 板 · NuttX',['SbusInput / RcUpdate','Command + Failsafe + RobotControl','ControlAllocator + CanOutput','标定、解锁、失联与分配'])
box(2220,295,290,205,'达妙电机',['2 路动力','2 路转向','CAN 模式控制'])
arrow([(495,377),(690,377)],'2.4 GHz',lpos=(590,343))
arrow([(1120,377),(1370,377)],'SBUS / 现有适配',lpos=(1245,340))
arrow([(2070,377),(2220,377)],'CAN',lpos=(2144,343))
txt(1370,605,'保留当前控制链；更换遥控后重新标定通道与开关',27,BLUE)
txt(85,663,'接收机只有 PWM 时：需增加输入捕获模块或更换兼容 SBUS 接收机。',26,GRAY)

frame(50,750,2500,370,'② 视频链  ·  摄像头独立工作，具体商品 SKU 待确认',GREEN)
box(85,861,450,174,'独立网络摄像头',['内置编码 / 网络 / TF 卡槽','仅供电即可运行：须确认'],GREEN,'#EDF8F2')
box(925,830,410,133,'TF 卡',['上电录像 / 断网继续：须确认'],GREEN,'#EDF8F2')
box(925,983,410,107,'无线网络',['Wi-Fi AP / 路由器 / 4G'],GREEN,'#EDF8F2')
box(1790,902,630,152,'手机 · 摄像头厂商 APP',['实时预览 / 录像回放 / 下载','不自动显示测厚或 C 板数据'],GREEN,'#EDF8F2')
arrow([(535,910),(690,910),(690,897),(925,897)],'本地存储',GREEN,lpos=(730,864))
arrow([(535,975),(750,975),(750,1036),(925,1036)],'视频传输',GREEN,lpos=(665,946))
arrow([(1335,1036),(1560,1036),(1560,978),(1790,978)],'网络查看',GREEN,lpos=(1500,1002))

frame(50,1160,2500,290,'③ 可选扩展  ·  需要新增驱动或明确设备协议',ORANGE)
box(85,1235,510,174,'触碰 / 行程开关',['GPIO 输入 → C 板','去抖 / 到位 / 超时'],ORANGE,'#FFF6E7')
box(700,1235,600,174,'电推杆',['C 板 → 互锁继电器 / 驱动板','伸出 / 收回 / 停止'],ORANGE,'#FFF6E7')
box(1405,1235,510,174,'电池 BMS',['CAN 协议与波特率待确认','SOC 读取不等于遥控屏显示'],ORANGE,'#FFF6E7')
box(2020,1235,490,174,'测厚数据采集',['另定 RS485 / USB Host 路径','视频存储不等于数据存储'],ORANGE,'#FFF6E7')
txt(55,1505,'购买前要求演示：仅接电源 + 插 TF 卡 → 本地录像；手机查看；关掉手机 / 网络后继续录像。',29,bold=True)
txt(55,1568,'控制与视频独立运行；手机 APP 不替代机器人上位机。所有“须确认”能力以所选型号实测为准。',27,GRAY)
svg.append('</svg>')
im.save(OUT/'standalone-ht10a-camera-plan.png')
(OUT/'standalone-ht10a-camera-plan.svg').write_text('\n'.join(svg),encoding='utf-8')
print('Generated standalone-ht10a-camera-plan.png / .svg')
