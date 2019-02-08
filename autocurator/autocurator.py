from __future__ import print_function, division
from lxml import etree
import cdms2
import numpy
import os
import cdtime

typeConv  = { "Float32": "Float",
            "Float": "Double",
            "Float64": "Double",
            "Float128": "Double",
            "Int32": "Int",
            }

def ingestCSV(filename):
    with open(filename) as f:
        csv = f.readlines()
    vars = csv[0].strip().split(",")
    times = []
    for i,l in enumerate(csv[2:],start=2):
        if l.find("file_ix")>-1:
            break
    files = []
    for l in csv[i+1:]:
        files.append(l.strip().split(",")[1][1:-1])
    return vars, files, csv[2:i]

def createMapping(filename):
    vars, files, csv = ingestCSV(filename)

    out = {}

    f = cdms2.open(files[0])
    times = []
    time_values = []
    time_partition = []
    nontimes = []
    for v in f.listvariables():
        tim = f[v].getTime()
        if tim is None:  # not a time one
            nontimes.append(v)
        else:
            times.append(v)

    map_files = []
    index = 0
    for l in csv:
        if l.strip() == "":
            break
        sp = l.strip().split(",")
        time_values.append(cdtime.s2r(sp[0], tim.units, tim.getCalendar()).value)
        indx = vars.index(times[0])
        location = int(sp[indx].split(":")[0])
        if len(map_files) == 0:
            map_files.append([location, [index, index]])
            time_partition.append(index)
        elif map_files[-1][0] != location:
            map_files.append([location, [index, index]])
            time_partition.append(index)
        else:  # same file
            map_files[-1][1][-1] = index + 1
            time_partition[-1] = index + 1
        index += 1
    mapping = "[[" + str(times).replace("'","") + ",["
    mapping += ",".join(["[{},{},-,-,-,{}]".format(v[0],v[1],os.path.basename(files[nm])) for nm,v in map_files])
    mapping+="]],["+str(nontimes).replace("'","") +",[[-,-,-,-,-,{}]]]]".format(os.path.basename(files[map_files[0][0]]))
    out["mapping"] = mapping
    out["time"] = {"id": tim.id, "values":time_values, "partition":time_partition}
    out["files"] = files
    out["vars"] = vars
    out["directory"] = os.path.dirname(files[map_files[0][0]])
    return out

def csv2xml(csv_file):
    info = createMapping(csv_file)

    time_name = info["time"]["id"]
    time_values = " ".join([str(v) for v in info["time"]["values"]])
    time_partition	= "[{}]".format(" ".join([str(v) for v in info["time"]["partition"]]))
    time_length = str(len(info["time"]["values"]))
    dataset = etree.Element("dataset")
    f = cdms2.open(info["files"][0])

    # split vars into timebased and non time based ones
    timesVars = []
    nonTimeVars = []
    for v in f.listvariables():
        V = f[v]
        if V.getTime() is not None:
            timesVars.append(v)
        else:
            nonTimeVars.append(v)

    map = []


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
    dataset.set("cdms_filemap",info["mapping"])
    dataset.set("directory",info["directory"])
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
    return etree.tostring(dataset)