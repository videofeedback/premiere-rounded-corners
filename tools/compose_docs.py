"""Create labeled documentation figures from actual C++ renderer output. Requires Pillow."""
from pathlib import Path
import os
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
RAW=ROOT/'build/docs'
OUT=ROOT/'docs/images'
OUT.mkdir(parents=True,exist_ok=True)
BG='#10191f'; FG='#eef5f2'; MUTED='#a9b9bf'; ACCENT='#80e0bb'; PANEL='#19262e'
fonts=[os.environ.get('DOCS_FONT',''),'/System/Library/Fonts/Supplemental/Arial.ttf',
       '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf']
fontpath=next((p for p in fonts if p and Path(p).is_file()),None)
def font(size):
    return ImageFont.truetype(fontpath,size) if fontpath else ImageFont.load_default(size=size)
def canvas(title,sub,h=970):
    im=Image.new('RGB',(1600,h),BG);d=ImageDraw.Draw(im)
    d.text((64,38),'VIDEO FEEDBACK  /  ADOBE PREMIERE PRO',font=font(19),fill=ACCENT)
    d.text((60,83),title,font=font(64),fill=FG)
    d.text((64,168),sub,font=font(25),fill=MUTED)
    return im,d
def text(d,x,y,s,size=24,color=FG):d.text((x,y),s,font=font(size),fill=color)
def image(im,name,xy,width):
    a=Image.open(RAW/(name+'.ppm'));a=a.resize((width,round(width*a.height/a.width)),Image.Resampling.LANCZOS)
    im.paste(a,xy);return a.height
def footer(d,h):
    d.line((64,h-68,1536,h-68),fill='#31434d',width=1)
    text(d,64,h-46,'Actual C++ renderer output · Synthetic source · Checkerboard = transparency',19,MUTED)

im,d=canvas('Rounded corners. On your terms.',
               'Set the frame edges, then round the corners. The picture stays in place.',860)
text(d,64,255,'01  Original source',27)
text(d,832,255,'02  Frame defined + corners rounded',27)
image(im,'source',(64,307),704);image(im,'trimmed',(832,307),704)
text(d,64,728,'Black bars are part of the original image.',21,MUTED)
text(d,832,728,'Top 20% · Bottom 15% · Radius 15%',21,ACCENT)
footer(d,860);im.save(OUT/'overview.png',optimize=True)

im,d=canvas('Every edge is yours.',
               'Move Top, Bottom, Left and Right independently. No stretching or resampling.',1410)
# Large main result + literal control reference, intentionally not an Adobe UI replica.
image(im,'trimmed',(64,260),960)
d.rounded_rectangle((1072,260,1536,800),radius=18,fill=PANEL)
text(d,1100,290,'Example settings',30)
rows=[('Frame Width','100%'),('Frame Height','100%'),('Frame Top','20%'),
      ('Frame Bottom','15%'),('Frame Left','0%'),('Frame Right','0%'),('Corner Radius','15%')]
for i,(label,value) in enumerate(rows):
    y=350+i*56;text(d,1100,y,label,22,MUTED);text(d,1422,y,value,24,ACCENT)
text(d,64,837,'Asymmetric crop',28);text(d,832,837,'Shallow frame, circular corners',28)
image(im,'portrait',(64,886),640);image(im,'wide',(832,886),640)
text(d,64,1270,'Top 20 · Bottom 15 · Left 35 · Right 5',21,MUTED)
text(d,832,1270,'Top 30 · Bottom 25 · Left 5 · Right 5',21,MUTED)
footer(d,1410);im.save(OUT/'frame-controls.png',optimize=True)

im,d=canvas('One radius. Always circular.',
               'Radius = selected percentage × the shorter side of the final frame.',1320)
for i,r in enumerate([0,10,25,50]):
    x=64+(i%2)*768;y=260+(i//2)*490
    text(d,x,y,f'{r}% radius',28,ACCENT)
    image(im,f'radius-{r}',(x,y+50),704)
footer(d,1320);im.save(OUT/'radius-examples.png',optimize=True)
print('Created overview.png, frame-controls.png and radius-examples.png')
