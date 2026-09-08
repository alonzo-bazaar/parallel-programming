#!/usr/bin/env python3

import numpy as np
from PIL import Image

def gradient_value_row(width:int, start:float, end:float):
    zero_to_one = np.arange(width)/(width-1)
    one_to_zero = 1 - zero_to_one
    return (one_to_zero * start) + (zero_to_one * end)

def gradient_pixel_row(width:int, start:tuple, end:tuple):
    values = np.stack([gradient_value_row(width, start[0], end[0]),
                       gradient_value_row(width, start[1], end[1]),
                       gradient_value_row(width, start[2], end[2])]).T
    return np.floor(values * 255).astype(np.uint8)
                       
def horizontal_gradient(width:int, height:int, start:tuple, end:tuple):
    pixel_row = gradient_pixel_row(width, start, end)
    image_pixels = np.stack([pixel_row for _ in range(height)])
    return Image.fromarray(image_pixels, 'RGB')

def flat_image(width:int, height:int, color:tuple):
    pixels=np.stack([np.astype(np.floor(np.ones((width, height)) * c * 255),
                               np.uint8)
                     for c in color],
                     dtype=np.uint8).transpose((1, 2, 0))
    return Image.fromarray(pixels)

colors = {
    'red' : (1, 0, 0),
    'green' : (0, 1, 0),
    'blue' : (0, 0, 1),

    'yellow' : (1, 1, 0),
    'orange' : (1, 0.5, 0),
    'turquoise' : (0, 1, 1),
    'purple' : (1, 0, 1),

    'white' : (1, 1, 1),
    'black' : (0, 0, 0),
}

def target(startcol:str, endcol:str):
    return (colors[startcol],
            colors[endcol],
            f"./test_{startcol}_to_{endcol}.png")

def cat(*args):
    res = []
    for arg in args:
        if type(arg) == list:
            res.extend(arg)
        else:
            res.append(arg)
    return res

ims = cat(
    [target(c, 'black')
        for c in colors.keys()
        if c != 'black'],
    [target(c, 'white')
        for c in colors.keys()
        if c != 'white'],
    [target(a, b)
        for a in ['red', 'green', 'blue']
        for b in ['red', 'green', 'blue']
        if a != b]
)

# generate gradient images
for (start_color, end_color, image_name) in ims:
    print(f'generating image "{image_name}"...')
    horizontal_gradient(100, 100, start_color, end_color).save(image_name)

# generate flat images
for color_name in colors.keys():
    image_name=f'./flat_{color_name}.png'
    print(f'generating image "{image_name}"...')
    flat_image(100, 100, colors[color_name]).save(image_name)
