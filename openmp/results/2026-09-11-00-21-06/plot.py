#!/usr/bin/env python

# tempo per fare 1000 iterazioni con 1000 boid, a seconda del numero di thread e della versione del codice utilizzata
import matplotlib as mpl
import matplotlib.pyplot as plt
soa_seq_time=1678
aos_seq_time=3464
soa_parallel_times = {
        1: 1676,
        2: 1316,
        4: 771,
        8: 577,
        16: 656,
}
aos_parallel_times = {
        1: 3459, 
        2: 1844,
        4: 918,
        8: 741,
        16: 1199,
}
fig, (ax1, ax2) = plt.subplots(1, 2, sharey=True)
fig.suptitle("runtime for 1000 iterations with 1000 boids")
ax1.set_title("SoA Implementation")
t = sorted(soa_parallel_times.keys())
ax1.plot(t, [soa_parallel_times[i] for i in t])
ax1.set_xscale('log')
ax1.set_xticks(t, map(str, t))
ax1.set_xlabel('number of threads')
ax1.set_ylabel('execution time (ms)')
ax1.hlines([soa_seq_time], 1, 16, linestyle='--')

ax2.set_title("AoS Implementation")
t = sorted(aos_parallel_times.keys())
ax2.plot(t, [aos_parallel_times[i] for i in t])
ax2.set_xscale('log')
ax2.set_xticks(t, map(str, t))
ax2.set_xlabel('number of threads')
ax2.set_ylabel('execution time (ms)')
ax2.hlines([aos_seq_time], 1, 16, linestyle='--')

plt.savefig('plot.svg')
