from __future__ import print_function, division
from lxml import etree
import cdms2
import numpy

typeConv = { "Float32": "Float",
            "Float": "Double",
            "Float64": "Double",
            "Float128": "Double",
            "Int32": "Int",
            }
fnm = "/1Tb/miniconda3/envs/nightly/share/cdat/sample_data/tas_ccsr-95a_1979.01-1979.12.nc"
time_name = "time"
time_values =  """15.5   45.    74.5  105.   135.5  166.   196.5  227.5  258.   288.5
  319.   349.5  380.5  410.   439.5  470.   500.5  531.   561.5  592.5
  623.   653.5  684.   714.5  745.5  775.   804.5  835.   865.5  896.
  926.5  957.5  988.  1018.5 1049.  1079.5 1110.5 1140.  1169.5 1200.
 1230.5 1261.  1291.5 1322.5 1353.  1383.5 1414.  1444.5 1475.5 1505.
 1534.5 1565.  1595.5 1626.  1656.5 1687.5 1718.  1748.5 1779.  1809.5
 1840.5 1870.  1899.5 1930.  1960.5 1991.  2021.5 2052.5 2083.  2113.5
 2144.  2174.5"""
time_partition	="[ 0 12 12 24 24 36 36 48 48 60 60 72]"
time_length = str(len(time_values.split(" ")))
dataset = etree.Element("dataset")
f = cdms2.open(fnm)

cdscan_file_Attributes = ["institution", "production", "calendar", "Conventions", "history"]
for att in f.listglobal():
    if att in cdscan_file_Attributes:
        dataset.set(att, getattr(f, att))
    else:
        elt = etree.Element("attr")
        elt.set("datatype", "String")
        elt.set("name", att)
        elt.text = getattr(f, att)
        dataset.append(elt)
dataset.set("cdms_filemap","[[[abs_time,tas,bounds_time],[[0,12,-,-,-,tas_ccsr-95a_1979.01-1979.12.nc],[12,24,-,-,-,tas_ccsr-95a_1980.01-1980.12.nc],[24,36,-,-,-,tas_ccsr-95a_1981.01-1981.12.nc],[36,48,-,-,-,tas_ccsr-95a_1982.01-1982.12.nc],[48,60,-,-,-,tas_ccsr-95a_1983.01-1983.12.nc],[60,72,-,-,-,tas_ccsr-95a_1984.01-1984.12.nc]]],[[bounds_latitude,bounds_longitude,weights_latitude],[[-,-,-,-,-,tas_ccsr-95a_1979.01-1979.12.nc]]]]")
dataset.set("directory","/1Tb/miniconda3/envs/nightly/share/cdat/sample_data/")
dataset.set("id", "none")
for att in cdscan_file_Attributes:
    if not att in dataset.keys():
        dataset.set(att,"")
dims = f.listdimension()
for data in f.listdimension()+f.listvariables():
    if data in dims:
        etype = "axis"
    else:
        etype = "variable"
    elt = etree.Element(etype)
    for att in f[data].attributes:
        value = f[data].attributes[att]
        if not isinstance(value, (str, int, float)):
            elt.set(att, "{}".format(float(f[data].attributes[att])))
        else:
            elt.set(att, "{}".format(f[data].attributes[att]))
    elt.set("id",f[data].id)
    dtype = numpy.typeNA.get(f[data].typecode())
    dtype= typeConv.get(dtype,dtype)
    elt.set("datatype", dtype)
    if data in dims:
        ax = f[data]
        if ax.isTime():
            ax2 = ax.clone()
            ax2.setCalendar(ax.getCalendar())
            dataset.set("calendar",ax2.calendar)
            elt.set("calendar",ax2.calendar)
            elt.set("length", time_length)
            elt.set("partition",time_partition)
            elt.set("id", time_name)
            elt.set("name_in_file", ax2.id)
            values_string = time_values
        else:
            elt.set("length", "{}".format(len(ax)))
            if ax.isCircular():
                topo = "circular"
            else:
                topo = "linear"
            topotree = etree.Element("attr")
            topotree.set("datatype","String")
            topotree.set("name","realtopology")
            topotree.text = topo
            elt.append(topotree)
            values_string = " ".join(["{}".format(x) for x in ax[:]])
        elt.text="[{}]".format(values_string)
    else:
        domain = etree.Element("domain")
        for ax in f[data].getAxisList():
            domElt = etree.Element("domElem")
            domElt.set("start","0")
            if ax.isTime():
                domElt.set("length", time_length)
                domElt.set("name", time_name)
            else:
                domElt.set("length", str(len(ax)))
                domElt.set("name", ax.id)
            domain.append(domElt)
        elt.append(domain)


            

    dataset.append(elt)

print("STRING:",etree.tostring(dataset))
with open("test.xml","w") as f:
    print('<?xml version="1.0"?>', file=f)
    print('<!DOCTYPE dataset SYSTEM "http://www-pcmdi.llnl.gov/software/cdms/cdml.dtd">', file=f)
    print(etree.tostring(dataset), file=f)

