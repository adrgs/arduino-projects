#!/usr/bin/env python

from PIL import Image
import sys

def get_RGB565(r,g,b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

if len(sys.argv)<2:
    print ("Usage: %s path_to_image (custom_height/N) (custom_width/N)" % sys.argv[0])
    sys.exit()

file_name = sys.argv[1]
im = Image.open(file_name)
pix = im.load()

height_array = im.size[1]
width_array = im.size[0]
FILL_COLOR = 0

def min_or_0(s):
    if s<0:
         return 0
    return s

#If the custom width or height is greater than the actual Image, then the Image will be centrated in the array
if len(sys.argv)==3:
    if 'N' not in sys.argv[2].upper():
        height_array = int(sys.argv[2])
if len(sys.argv)==4:
    if 'N' not in sys.argv[3].upper():
        width_array = int(sys.argv[3])

l = []
for i in range(height_array):
    l.append([])
    for j in range(width_array):
        l[-1].append(FILL_COLOR)

offset_width = min_or_0((width_array - im.size[0])/2)
offset_height = min_or_0((height_array - im.size[1])/2)

for y in range(min(height_array, im.size[1])):
    for x in range(min(width_array, im.size[0])):
        p = pix[x,y]
        l[y+offset_height][x+offset_width] = get_RGB565(p[0],p[1],p[2])

out = "uint16_t image[%d][%d] = {\n" % (height_array,width_array)
for i in range(len(l)):
    out += '{'
    for j in range(len(l[i])):
        out += str(l[i][j]) + ' '
        if j != len(l[i])-1:
            out += ','
    out += '}'
    if i!=len(l)-1:
        out += ',\n'
out += "\n};"

print out