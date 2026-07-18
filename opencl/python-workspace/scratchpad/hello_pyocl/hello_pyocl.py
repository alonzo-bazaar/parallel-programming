import numpy as np
import pyopencl as cl

#add two vectors (all ones for testing purposes)
in1_np = 3 * np.ones(100, dtype=np.float32)
in2_np = 5 * np.ones(100, dtype=np.float32)

ctx = cl.create_some_context(interactive=False)
queue = cl.CommandQueue(ctx)

mf = cl.mem_flags
in1_dev = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=in1_np)
in2_dev = cl.Buffer(ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=in2_np)

vecadd_prog = None
with open('hello_pyocl.cl', 'r') as file:
    src = file.read().strip()
    vecadd_prog = cl.Program(ctx, src).build()

# in1_np.nbytes is basically len(in1_np) * sizeof(in1_np[0])
# number of bytes used by the underlying byffer of in1_np
res_dev = cl.Buffer(ctx, mf.WRITE_ONLY, in1_np.nbytes)
kern = vecadd_prog.add_into

# https://stackoverflow.com/questions/67292857/pyopencl-invalid-kernel-argument
# pyopencl understands numpy shit, it does not understand python shit
# when passing data to a pyopencl kernel, only pass numpy data
# be it scalar or array
kern(queue,
     in1_np.shape, None,
     in1_dev, in2_dev, res_dev, np.int32(in1_np.shape[0]))

res_np = np.empty_like(in1_np)
cl.enqueue_copy(queue, res_np, res_dev)

# and voilà
