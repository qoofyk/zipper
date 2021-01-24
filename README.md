See [this page](https://fengggli.github.io/zipper-runtime) for sphinx documentation.

checkout
```
git clone -b zipper-workflow --recurse-submodules git@github.com:fengggli/zipper-runtime.git
```

How to build
----------------

```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain.stampede2-knl.cmake
cmake .. -Duse_itac=on -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain.stampede2-knl.cmake
```


Zipper In-situ Runtime System
----------------------------------------------------------------
For paper **Yuankun Fu**, Feng Li, Fengguang Song, **Performance Analysis and Optimization of In-situ Integration of Simulation with Data Analysis: Zipping Applications Up**. To appear in *Proceedings of the 27th ACM International Symposium on High-Performance Parallel and Distributed Computing* (**HPDC'18**), Tempe, Arizona, June 2018. [[PDF](https://par.nsf.gov/servlets/purl/10095818)]

This paper targets an important class of applications that requires combining HPC simulations with data analysis for online or real-time scientific discovery. We use the state-of-the-art parallel-IO and data-staging libraries to build simulation-time data analysis workflows, and conduct performance analysis with real-world applications of computational fluid dynamics (CFD) simulations and molecular dynamics (MD) simulations. Driven by in-depth performance inefficiency analysis, we design an end-to-end application-level approach to eliminating the interlocks and synchronizations existent in the present methods. Our new approach employs both task parallelism and pipeline parallelism to reduce synchronizations effectively. In addition, we design a fully asynchronous, fine-grain, and pipelining runtime system, which is named Zipper. Zipper is a multi-threaded distributed runtime system and executes in a layer below the simulation and analysis applications. To further reduce the simulation application's stall time and enhance the data transfer performance, we design a concurrent data transfer optimization that uses both HPC network and parallel file system for improved bandwidth. The scalability of the Zipper system has been verified by a performance model and various empirical large scale experiments. The experimental results on an Intel multicore cluster as well as a Knight Landing HPC system demonstrate that the Zipper based approach can outperform the fastest state-of-the-art I/O transport library by up to 220% using 13,056 processor cores.
