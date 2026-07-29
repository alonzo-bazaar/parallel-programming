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
                 section_name=None,
                 # if not none rules over all logging args
                 log=None,
                 # logging args
                 log_start:bool=True, log_end:bool=True, trail_empty_line:bool=True,
                 # place where to put results after done
                 append_time_into=None):
        self.t = 0
        self.section_name = section_name

        if log is not None:
            log_start=log
            log_end=log
            if log==False:
                trail_empty_line=False

        self.log_start = log_start
        self.log_end = log_end

        self.trail_empty_line = trail_empty_line
        self.append_time_into=append_time_into

    def __enter__(self):
        self.t = time.time()
        if self.log_start:
            if self.section_name is None:
                print("Starting timed block...")
            else:
                print(f"Starting timed block [{self.section_name}]...")

    def __exit__(self, *ignoredargs):
        t = time.time()-self.t
        maybe_name = ""
        if self.section_name is not None:
            maybe_name=f"[{self.section_name}] "

        if self.log_end:
            print(f"timed block {maybe_name}took {t} seconds")
            if self.trail_empty_line:
                print("")

        if self.append_time_into is not None:
            self.append_time_into.append(t)

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

def error_on_diff(expected, actual,
                  test_name=None,
                  log_expected=False, log_actual=False, log_diff=False,
                  plot_diff=False,
                  die_on_error=False):

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
        if die_on_error:
            sys.exit(1)
        return True
    return False

def compile_file(filename:str, ctx:cl.Context):
    with open(filename, 'r') as file:
        src = file.read().strip()
        return cl.Program(ctx, src).build()
