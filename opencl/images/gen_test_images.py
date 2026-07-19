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

i = 0
print('starting')
horizontal_gradient(100, 100, (1,0,0), (1,1,1)).save('./test_red_to_white.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (0,1,0), (1,1,1)).save('./test_green_to_white.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (0,0,1), (1,1,1)).save('./test_blue_to_white.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (1,1,0), (1,1,1)).save('./test_yellow_to_white.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (0,1,1), (1,1,1)).save('./test_turquoise_to_white.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (1,0,1), (1,1,1)).save('./test_purple_to_white.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (1,0,0), (0,0,0)).save('./test_red_to_black.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (0,1,0), (0,0,0)).save('./test_green_to_black.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (0,0,1), (0,0,0)).save('./test_blue_to_black.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (1,1,0), (0,0,0)).save('./test_yellow_to_black.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (0,1,1), (0,0,0)).save('./test_turquoise_to_black.png')
print(f'image number {i}')
i+=1
horizontal_gradient(100, 100, (1,0,1), (0,0,0)).save('./test_purple_to_black.png')
print(f'image number {i}')
i+=1
