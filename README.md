# replace-color
Replace a single color in a bunch of RGBA images

Usage:
```
replace-color [OPTION...]

-d, --directory arg        The directory where to search for input files
-s, --filename-suffix arg  How the input file names should end
-f, --from-color arg       Which RGBA color to change; for example, try
                           0xffff00ff for yellow
-t, --to-color arg         Which RGBA color to change to; for example, try
                           0xffff0080 for yellow with alpha
```

For example:
```
replace-color -d=/path/to/images -s=.png -f=0xffff00ff -t=0xffff0080
```