# cscan help message

```
cdscan -h
```

returns

```
Usage:
    cdscan [options] <files>

    Scan a list of files producing a CDMS dataset in XML representation. See Notes below
    for a more complete explanation.

Arguments:

    <files> is a list of file paths to scan. The files can be listed in any order, and may
    be in multiple directories.  A file may also be a CDML dataset (.xml or .cdml), in
    which case the dataset(s) and files are combined into a new dataset.

Options:

    -a alias_file: change variable names to the aliases defined in an alias file.
                   Each line of the alias file consists of two blank separated
                   fields: variable_id alias. 'variable_id' is the ID of the variable
                   in the file, and 'alias' is the name that will be substituted for
                   it in the output dataset. Only variables with entries in the alias_file
                   are renamed.

    -c calendar:   either "gregorian", "proleptic_gregorian", "julian", "noleap", or "360_day". Default:
                   "gregorian". This option should be used with caution, as it will
                   override any calendar information in the files.

    -d dataset_id: dataset identifier. Default: "none"

    -e newattr:    Add or modify attributes of a file, variable, or
           axis. The form of 'newattr' is either:

           'var.attr = value' to modify a variable or attribute, or
           '.attr = value' to modify a global (file) attribute.

           In either case, 'value' may be quoted to preserve spaces
           or force the attribute to be treated as a string. If
           'value' is not quoted and the first character is a
           digit, it is converted to integer or
           floating-point. This option does not modify the input
           datafiles. See notes and examples below.

    --exclude var,var,...
                   Exclude specified variables. The argument
                   is a comma-separated list of variables containing no blanks.
                   In contrast to --exclude-file, this skips the variables regardless
                   of the file(s) in which they are contained, but processes other
                   variables in the files.
                   Also see --include.

    --exclude-file pattern
                   Exclude files with a basename matching the regular expression pattern.
                   In contrast to --exclude, this skips the file entirely. Multiple patterns
                   may be listed by separating with vertical bars (e.g. abc|def ). Note
                   that the match is to the initial part of the basename. For example, the
                   pattern 'st' matches any basename starting with 'st'.

    -f file_list:  file containing a list of absolute data file names, one per
                   line. <files> arguments are ignored.

    --forecast     generate a description of a forecast dataset.
                   This is not compatible with the -i, -r, -t, or -l options.
                   A file can contain data for exactly one forecast; its
                   forecast_reference_time (aka run time, analysis time, starting time,
                   generating time, tau=0 time) is specified by the nbdate,nbsec variables.
                   Each file's time axis will be interpreted as the forecast_period (aka
                   tau, the interval from the forecast_reference_time to the current time)
                   regardless of its units, standard_name, or other attributes.

    -h:            print a help message.

    -i time_delta: scan time as a 'linear' dimension. This is useful if the time dimension
                   is very long. The argument is the time delta, a float or integer.  For
                   example, if the time delta is 6 hours, and the reference units are
                   "hours since xxxx", set the interval delta to 6.  The default value is
                   the difference of the first two timepoints.

    --ignore-open-error:
                   Ignore open errors. Print a warning and continue.

    --include var,var,...
                   Only include specified variables in the output. The argument
                   is a comma-separated list of variables containing no blanks.
                   Also see --exclude.

    --include-file pattern
                   Only include files with a basename matching the regular expression pattern.
                   In contrast to --include, this skips files entirely if they do not
                   match the pattern. Multiple patterns
                   may be listed by separating with vertical bars (e.g. abc|def ). Note
                   that the match is to the initial part of the basename. For example, the
                   pattern 'st' matches any basename starting with 'st'.

    -j:        scan time as a vector dimension. Time values are listed
           individually. Turns off the -i option.

    -l levels:     list of levels, comma-separated. Only specify if files are partitioned by
                   levels.

    -m levelid:    name of the vertical level dimension. The default is the name of the
                   vertical level dimension

    --notrim-lat:  Don't trim latitude values (in degrees) to the range [-90..90]. By default
           latitude values are trimmed.

    -p template:   Compatibility with pre-V3.0 datasets. 'cdimport -h' describes template strings.

    -q:            quiet mode

    -r time_units: time units of the form "<units> since yyyy-mm-dd hh:mi:ss", where
                   <units> is one of "year", "month", "day", "hour", "minute", "second".
                   Trailing fields may be omitted. The default is the units of the first
                   time dimension found.

    -s suffix_file: Append a suffix to variable names, depending on the directory
                   containing the data file.  This can be used to distinguish variables
                   having the same name but generated by different models or ensemble
                   runs. 'suffix_file' is the name of a file describing a mapping between
                   directories and suffixes.  Each line consists of two blank-separated
                   fields: 'directory' 'suffix'. Each file path is compared to the
                   directories in the suffix file. If the file path is in that directory
                   or a subdirectory, the corresponding suffix is appended to the variable
                   IDs in the file. If more than one such directory is found, the first
                   directory found is used. If no match is made, the variable ids are not
                   altered.  Regular expressions can be used: see the example in the Notes
                   section.

    -t timeid:     id of the partitioned time dimension. The default is the name of the time
                   dimension.

    --time-linear tzero,delta,units[,calendar]
                   Override the time dimensions(s) with a linear time dimension. The arguments are
                   a comma-separated list:

                   tzero is the initial time point, a floating-point value.
                   delta is the time delta, floating-point.
                   units are time units as specified in the [-r] option.
                   calendar is optional, and is specified as in the [-c] option. If omitted, it
                     defaults to the value specified by [-c], otherwise as specified in the file.

                   Example: --time-linear '0,1,months since 1980,noleap'

                   Note (6) compares this option with [-i] and [-r]

    --var-locate 'var,file_pattern':
                   Only scan a variable if the basename of the file matches the pattern. This
                   may be used to resolve duplicate variable errors. var and file_pattern are
                   separated by a comma, with no blanks.

                   var is the name of the variable
                   file_pattern is a regular expression following the Python re module syntax.e

                   Example: to scan variable ps from files starting with the string 'ps_':
                     --var-locate 'ps,ps_.*'

    -x xmlfile:    XML filename. By default, output is written to standard output.

Example:

    cdscan -c noleap -d test -x test.xml [uv]*.nc
    cdscan -d pcmdi_6h -i 0.25 -r 'days since 1979-1-1' *6h*.ctl

Notes:

    (1) The files can be in netCDF, GrADS/GRIB, HDF, or DRS format, and can be listed in
    any order. Most commonly, the files are the result of a single experiment, and the
    'partitioned' dimension is time. The time dimension of a variable is the coordinate
    variable having a name that starts with 'time' or having an attribute "axis='T'". If
    this is not the case, specify the time dimension with the -t option. The time
    dimension should be in the form supported by cdtime. If this is not the case (or to
    override them) use the -r option.

    (2) The basic form of the command is 'cdscan <files>'. By default, the time values are
    listed explicitly in the output XML. This can cause a problem if the time dimension is
    very long, say for 6-hourly data. To handle this the form 'cdscan -i delta <files>'
    may be used. This generates a compact time representation of the form <start, length,
    delta>. An exception is raised if the time dimension for a given file is not linear.

    (3) Another form of the command is 'cdscan -l lev1,lev2,..,levn <files>'. This asserts
    that the dataset is partitioned in both time and vertical level dimensions. The level
    dimension of a variable is the dimension having a name that starts with "lev", or
    having an attribute "axis=Z". If this is not the case, set the level name with the -m
    option.

    (4) An example of a suffix file:

    /exp/pr/ncar-a  _ncar-a
    /exp/pr/ecm-a   _ecm-a
    /exp/ta/ncar-a  _ncar-a
    /exp/ta/ecm-a   _ecm-a

    For all files in directory /exp/pr/ncar-a or a subdirectory, the corresponding
    variable ids will be appended with the suffix '_ncar-a'.  Regular expressions can be
    used, as defined in the Python 're' module. For example, The previous example can be
    replaced with the single line:

    /exp/[^/]*/([^/]*) _\g<1>

    Note the use of parentheses to delimit a group. The syntax \g<n> refers to the n-th
    group matched in the regular expression, with the first group being n=1. The string
    [^/]* matches any sequence of characters other than a forward slash.

    (5) Adding or modifying attributes with the -e option:

    time.units = "days since 1979-1-1"

    sets the units of all variables/axes to "Days since 1979-1-1". Note
    that since this is done before any other processing is done, it allows
    overriding of non-COARDS time units.

    .newattr=newvalue

    Set the global file attribute 'newattr' to 'newvalue'.

    (6) The [--time-linear] option overrides the time values in the file(s). The resulting
    dimension does not have any gaps. In contrast, the [-i], [-r] options use the specified
    time units (from [-r]), and calendar from [-c] if specified, to convert the file times
    to the new units. The resulting linear dimension may have gaps.

    In either case, the files are ordered by the time values in the files.

    The [--time-linear] option should be used with caution, as it is applied to all the time
    dimensions found.
```