Import("env")
print("Replace MKSPIFFSTOOL with mklittlefs.exe")
env.Replace (MKSPIFFSTOOL = "mklittlefs.exe")

# Acquire mklittlefs.exe from here: https://github.com/earlephilhower/mklittlefs/releases
# Place in root of project directory.