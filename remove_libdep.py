# Automatically remove tinywire from RTCLib as a lib dependency.

# String to look out for: depends=TinyWireM
# File in which string is located: library.properties

import os

# To make this work regardless of environment, we simply navigate into the first subdir within .\.pio\libdeps
bDir = '.\.pio\libdeps\\'
bDir = bDir + os.listdir(bDir)[0]
bDir = bDir + "\RTClib\\"

# Open target file and save all its contents into an array.
# Omit all strings matching with a[] while we're at it.
targetFile = "library.properties"
f = open(targetFile,'r')
a = ['depends=TinyWireM']
lst = []
for line in f:
    for word in a:
        if word in line:
            line = line.replace(word,'')
    lst.append(line)
f.close()

# Write lst[] into targetFile again.
f = open(targetFile,'w')
for line in lst:
    f.write(line)
f.close()