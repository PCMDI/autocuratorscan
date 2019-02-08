import cdms2
import cdat_info
import os

cdat_info.download_sample_data_files("data/sample_files.txt")

f = cdms2.open(os.path.join(cdat_info.get_sampledata_path(),"ta_ncep_87-6-88-4.nc"))

ta = f("ta", level=slice(0,6))
ta.getTime().units = "days since 1949-1-1"
ta2 = f("ta", level=slice(6,None))
ta2.shape
ta2.getTime().units = "days since 1949-1-1"
ta1 = cdms2.open("ta1.nc","w")
cdms2.setNetcdfDeflateLevelFlag(0)
cdms2.setNetcdfDeflateFlag(0)
cdms2.setNetcdfShuffleFlag(0)
ta1 = cdms2.open("ta1.nc","w")
ta1.write(ta[:4],id='ta')
ta1.close()
ta1 = cdms2.open("ta1b.nc","w")
ta1.write(ta[4:],id='ta')
ta1.close()
ta1 = cdms2.open("ta2.nc","w")
ta1.write(ta2[:4],id='ta')
ta1.close()
ta1 = cdms2.open("ta2b.nc","w")
ta1.write(ta2[4:],id='ta')
ta1.close()
