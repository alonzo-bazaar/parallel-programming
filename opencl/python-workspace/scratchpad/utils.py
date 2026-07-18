import numpy as np
import pyopencl as cl

import time # for timing how different versions run
import sys

# for quick visual compairison of differing histograms
import matplotlib as mpl
import matplotlib.pyplot as plt

# timing utilities
class TimedBlock:
    def __init__(self,
                 # mostly logging kwargs
                 section_name=None, log_start=True, trail_nl=True
                 ):
        self.t = 0
        self.section_name = section_name
        self.trail_nl = trail_nl
        if log_start:
            if self.section_name is None:
                print("Starting timed block...")
            else:
                print(f"Starting timed block [{self.section_name}]...")

    def __enter__(self):
        self.t = time.time()

    def __exit__(self, *ignoredargs):
        t = time.time()-self.t
        maybe_name = ""
        if self.section_name is not None:
            maybe_name=f"[{self.section_name}] "

        print(f"timed block {maybe_name}took {t} seconds")
        if self.trail_nl:
            print("")

# plotting utilities
def plot_compare_histograms(expected, actual):
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1)

    ax1.set_title("expected")
    ax1.bar(np.arange(len(expected)), expected)

    ax2.set_title("actual")
    ax2.bar(np.arange(len(actual)), actual)

    ax3.set_title("absolute value of difference")
    ax3.bar(np.arange(len(expected)), abs(expected - actual))

    plt.show()

def error_on_diff(expected, actual, test_name=None,
                  log_expected=False, log_actual=False, log_diff=False,
                  plot_diff=False):

    if not np.all(expected == actual):
        tn_note=f", in test [{test_name}]" if test_name is not None else ""
        print(f"now you fucked up!{tn_note}")

        if log_expected:
            print("expected value:")
            print(expected)
        if log_actual:
            print("got value:")
            print(actual)
        if log_diff:
            print("the difference is:")
            print(expected-actual)
        if plot_diff:
            print("plotting...")
            plot_compare_histograms(expected, actual)
        return True
    return False

def flush():
    sys.stdout.flush()

def compile_file(filename:str, ctx:cl.Context):
    with open(filename, 'r') as file:
        src = file.read().strip()
        return cl.Program(ctx, src).build()

    
