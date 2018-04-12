zsim
====

zsim is a fast x86-64 simulator. It was originally written to evaluate ZCache
(Sanchez and Kozyrakis, MICRO-44, Dec 2010), hence the name, but it has since
outgrown its purpose.
zsim's main goals are to be fast, simple, and accurate, with a focus on
simulating memory hierarchies and large, heterogeneous systems. It is parallel
and uses DBT extensively, resulting in speeds of hundreds of millions of
instructions/second in a modern multicore host. Unlike conventional simulators,
zsim is organized to scale well (almost linearly) with simulated core count.

You can find more details about zsim in our ISCA 2013 paper:
http://people.csail.mit.edu/sanchez/papers/2013.zsim.isca.pdf.


================================================================================

This version is modified by Shen & Shang

Adding more partition schemes: PriSM and Futility Scaling,
New replacement policy: SRRIP.

version: 4.12