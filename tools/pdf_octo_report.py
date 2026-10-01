"""Render the evidence report to a portable PDF using the bundled reportlab runtime."""
from pathlib import Path
import re
from xml.sax.saxutils import escape
from reportlab.pdfgen import canvas
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, Image, KeepTogether
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

root=Path(__file__).resolve().parents[1]/'reports/ethernet-matrix-20260913'
pdfmetrics.registerFont(TTFont('Arial','C:/Windows/Fonts/arial.ttf'))
pdfmetrics.registerFont(TTFont('ArialBold','C:/Windows/Fonts/arialbd.ttf'))
pdfmetrics.registerFontFamily('Arial',normal='Arial',bold='ArialBold',italic='Arial',boldItalic='ArialBold')
styles=getSampleStyleSheet()
for s in styles.byName.values(): s.fontName='Arial'
styles['BodyText'].fontSize=9; styles['BodyText'].leading=13; styles['BodyText'].spaceAfter=8
styles.add(ParagraphStyle('Cell',fontName='Arial',fontSize=6.4,leading=8))
for h in ['Heading1','Heading2','Heading3','Heading4']:
    styles[h].fontName='ArialBold'; styles[h].textColor=colors.HexColor('#123654')
    styles[h].keepWithNext=True

def rich(s):
    s=escape(s)
    s=re.sub(r'\*\*(.*?)\*\*',r'<b>\1</b>',s)
    s=re.sub(r'`([^`]+)`',r'\1',s)
    s=re.sub(r'\[([^]]+)\]\(([^)]+)\)',lambda m:f'<link href="{m[2]}">{m[1]}</link>' if m[2].startswith('http') else m[1],s)
    return s

lines=(root/'BERICHT_DE.md').read_text(encoding='utf-8').splitlines()
story=[]; i=0; width=A4[0]-80
while i<len(lines):
    line=lines[i]; i+=1
    if not line.strip(): continue
    if line.startswith('|'):
        block=[line]
        while i<len(lines) and lines[i].startswith('|'): block.append(lines[i]); i+=1
        data=[]
        for l in block:
            if re.match(r'^\|[-: |]+\|$',l):continue
            data.append([Paragraph(rich(c.strip()),styles['Cell']) for c in l.strip('|').split('|')])
        table=Table(data,colWidths=[width/len(data[0])]*len(data[0]),repeatRows=1,hAlign='LEFT')
        table.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),colors.HexColor('#e8f2f5')),('LINEBELOW',(0,0),(-1,-1),.3,colors.HexColor('#cdd9df')),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),3),('RIGHTPADDING',(0,0),(-1,-1),3)]))
        story += [table,Spacer(1,12)];continue
    m=re.match(r'!\[.*?\]\((.*?)\)',line)
    if m:
        image=Image(str(root/m[1])); ratio=min(width/image.imageWidth,610/image.imageHeight)
        image.drawWidth=image.imageWidth*ratio;image.drawHeight=image.imageHeight*ratio
        story += [image,Spacer(1,10)];continue
    m=re.match(r'^(#{1,4}) (.*)',line)
    if m:
        story.append(Paragraph(rich(m[2]),styles['Heading'+str(len(m[1]))]));continue
    story.append(Paragraph(rich(line),styles['BodyText']))

def footer(c,doc):
    c.saveState();c.setFont('Arial',8);c.setFillColor(colors.HexColor('#576d7c'))
    c.drawString(40,24,'Teensy · reale Ethernet-Messung · 13.09.2026')
    c.drawRightString(A4[0]-40,24,f'Seite {doc.page}');c.restoreState()

doc=SimpleDocTemplate(str(root/'Teensy_Ethernet_Testbericht.pdf'),pagesize=A4,rightMargin=40,leftMargin=40,topMargin=38,bottomMargin=42,title='Teensy Ethernet-Testbericht 13.09.2026',author='Codex')
doc.build(story,onFirstPage=footer,onLaterPages=footer)
print(root/'Teensy_Ethernet_Testbericht.pdf')
