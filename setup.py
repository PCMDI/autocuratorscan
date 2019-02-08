from setuptools import setup, find_packages
from subprocess import Popen, PIPE


Version = "1.0"
p = Popen(
    ("git",
     "describe",
     "--tags"),
    stdin=PIPE,
    stdout=PIPE,
    stderr=PIPE)
try:
    descr = p.stdout.readlines()[0].strip().decode("utf-8")
    Version = "-".join(descr.split("-")[:-2])
    if Version == "":
        Version = descr
except:
    descr = Version

setup (name = "autocurator",
       author="Charles Doutriaux",
       version=descr,
       description = "PYthon layer on top of hyperion's autocurator",
       url = "http://github.com/pcmdi/autocuratorscan",
       packages = find_packages(),
       scripts = ["scripts/autocuratorscan"],
)
