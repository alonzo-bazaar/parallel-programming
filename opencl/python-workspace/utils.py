import numpy as np
import pyopencl as cl

import time # for timing how different versions run
import sys  # to die on error failure

import copy
import functools # for functools.wraps() which we use in decorators

# for quick visual compairison of differing histograms
import matplotlib as mpl
import matplotlib.pyplot as plt

# opencl shit
def compile_file(filename:str, ctx:cl.Context):
    with open(filename, 'r') as file:
        src = file.read().strip()
        return cl.Program(ctx, src).build()

# arithmetic shit
def round_up_to_divide(a, b):
    if (a%b) == 0:
        return a
    if a < b:
        return b
    return(a + b - (a%b))

# timing shit
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

# utilities for better logging/displaying of computation errors
def diff(expected, actual):
    if expected.dtype in [np.int8, np.int16,np.int32, np.int64,
                          np.uint8, np.uint16, np.uint32, np.uint64]:
        return expected.astype(np.int64)-actual.astype(np.int64)
    else:
        return expected.astype(np.float64)-actual.astype(np.float64)

def plot_compare_histograms(expected, actual):
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1)

    ax1.set_title("expected")
    ax1.bar(np.arange(len(expected)), expected)

    ax2.set_title("actual")
    ax2.bar(np.arange(len(actual)), actual)

    ax3.set_title("absolute value of difference")
    ax3.bar(np.arange(len(expected)), diff(expected, actual))

    plt.show()

def error_on_diff(expected, actual,
                  test_name=None,
                  log_expected=False, log_actual=False, log_diff=False,
                  plot_diff=False,
                  die_on_error=False):

    if not np.all(expected == actual):
        tn_note=f" in test [{test_name}]" if test_name is not None else ""
        print(f"now you fucked up!{tn_note}")

        if log_expected:
            print("expected value:")
            print(expected)
        if log_actual:
            print("got value:")
            print(actual)
        if log_diff:
            d = diff(expected, actual)
            print("the difference is:")
            print(d)
            print("at indices:")
            idxs=d.nonzero()[0]
            print(idxs)
            print("where it is:")
            print(d[idxs])
            if log_expected:
                print("expected values at fucky indices:")
                print(expected[idxs])
            if log_actual:
                print("actual values at fucky indices:")
                print(actual[idxs])
        if plot_diff:
            print("plotting...")
            plot_compare_histograms(expected, actual)
        if die_on_error:
            sys.exit(1)
        return True
    return False

# buncha global state to wrap around `error_on_diff` calls
curr_kernel_test_name=None
curr_kernel_compare_kwargs = {
    'log_expected' :True,
    'log_actual'   :True,
    'log_diff'     :True,
    #'plot_diff'    :True,
    'die_on_error' :True,
}

# decorators to manage the global state in question
# https://realpython.com/primer-on-python-decorators/#finding-yourself
# https://realpython.com/primer-on-python-decorators/#defining-decorators-with-arguments
def kernel_test(name:str, **kt_kwargs):
    def kernel_test_decorator(fn):
        @functools.wraps(fn)
        def wrapped_kernel_test(*wkt_args, **wkt_kwargs):
            global curr_kernel_test_name, curr_kernel_compare_kwargs

            # backup old global settings
            old_kernel_test_name = copy.copy(curr_kernel_test_name)
            old_kernel_compare_kwargs = copy.copy(curr_kernel_compare_kwargs)

            # update global settings to reflect configuration parameters
            curr_kernel_test_name = name
            for k in kt_kwargs.keys():
                curr_kernel_compare_kwargs[k] = kt_kwargs[k]

            # call wrapped function in this updated global environment
            res = fn(*wkt_args, **wkt_kwargs)

            # restore old global state
            curr_kernel_test_name = old_kernel_test_name
            curr_kernel_compare_kwargs = old_kernel_compare_kwargs

            return res
        return wrapped_kernel_test
    return kernel_test_decorator

def compare(expected:np.ndarray, actual:np.ndarray):
    error_on_diff(expected, actual, test_name=curr_kernel_test_name,
                  **curr_kernel_compare_kwargs)
