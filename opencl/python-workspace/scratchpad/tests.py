#!/usr/bin/env python3
from utils import *

# test round_up_to_divide
assert(round_up_to_divide(0, 10) == 0)
for i in range(1, 10):
    assert(round_up_to_divide(i, 10) == 10)
for i in range(11, 20):
    assert(round_up_to_divide(i, 10) == 20)
for i in range(21, 30):
    assert(round_up_to_divide(i, 10) == 30)
