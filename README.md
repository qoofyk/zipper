See [this page](https://fengggli.github.io/zipper-runtime) for sphinx documentation.

checkout
```
git clone -b zipper-workflow --recurse-submodules git@github.com:fengggli/zipper-runtime.git
```

Build HPC components
---------------------

```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain.stampede2-knl.cmake
cmake .. -Duse_itac=on -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain.stampede2-knl.cmake
```

Prepare Cloud components
---------------------------

0. copy all artifacts into cloud-components/artifacts/
  - this include admin.conf, the k8s authentication info (is using 127.0.0.1, do ssh port forwarding ``ssh ubuntu@129.114.16.33 -L6443:localhost:6443``)
  - kubectl binary

1. login to a compute node via interactive session.
  - ``idev -p normal``
  - login node have process/memory limits which doesn't allow me to launch spark-submit.

2. prepare local environments (e.g. spark-submit):
```
python3 -m venv .venv
source sourceme.sh
pip install -r requirements.txt
```

See [this documents](cloud-components/HOWTO.md)
