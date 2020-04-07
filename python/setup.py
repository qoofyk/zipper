from setuptools import setup, find_packages
setup(
    name="fluiddmd",
    version="0.1",
    packages=find_packages(),
    script=["run_fluiddmd.py"]
#    install_requires=["pydmd>=0.2.1"]
)
