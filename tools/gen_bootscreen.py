from PIL import Image, ImageDraw, ImageFont
import sys
W,H=128,64
im=Image.new('1',(W,H),0); d=ImageDraw.Draw(im)
# icon on the left, 0..54 wide
ox,oy=2,2
# heater block + nozzle
d.rectangle([ox+16,oy+4,ox+36,oy+14],fill=1)
d.rectangle([ox+23,oy+0,ox+29,oy+4],fill=1)
d.polygon([(ox+19,oy+15),(ox+33,oy+15),(ox+29,oy+22),(ox+23,oy+22)],fill=1)
d.rectangle([ox+25,oy+23,ox+27,oy+25],fill=1)
# drop
d.polygon([(ox+26,oy+27),(ox+22,oy+34),(ox+30,oy+34)],fill=1)
d.ellipse([ox+21,oy+31,ox+31,oy+40],fill=1)
# sparks
for a,b in [((12,30),(7,27)),((40,30),(45,27)),((13,37),(7,38)),((39,37),(45,38))]:
    d.line([(ox+a[0],oy+a[1]),(ox+b[0],oy+b[1])],fill=1)
# anvil
d.polygon([(ox+2,oy+43),(ox+50,oy+43),(ox+46,oy+49),(ox+36,oy+51),(ox+33,oy+51),(ox+33,oy+54),(ox+40,oy+54),(ox+40,oy+59),(ox+12,oy+59),(ox+12,oy+54),(ox+19,oy+54),(ox+19,oy+51),(ox+12,oy+50),(ox+4,oy+47)],fill=1)
d.line([(ox+5,oy+45),(ox+47,oy+45)],fill=0)
# text
fb=ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf',22)
fs=ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf',11)
d.text((60,6),'SV06',font=fb,fill=1)
d.text((60,32),'REFORGED',font=fs,fill=1)
d.line([(60,48),(124,48)],fill=1)
im.save(sys.argv[1]+'/boot.png')
im.resize((W*6,H*6),Image.NEAREST).save(sys.argv[1]+'/boot_x6.png')
# emit Marlin bitmap
rows=[]
px=im.load()
for y in range(H):
    bs=[]
    for xb in range(W//8):
        v=0
        for b in range(8):
            if px[xb*8+b,y]: v|=0x80>>b
        bs.append('B%s'%format(v,'08b'))
    rows.append('  '+','.join(bs)+',')
open(sys.argv[1]+'/bitmap.txt','w').write('\n'.join(rows))
