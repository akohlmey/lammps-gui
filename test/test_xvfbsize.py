# -*- Python -*-

import os, sys
import subprocess
import time

# launch virtual frame buffer
vfb = subprocess.Popen(["Xvfb", ":2", "-screen", "0", "1024x768x24"])
os.environ["DISPLAY"] = ":2.0"
time.sleep(0.5)

import pyautogui
width, height = pyautogui.size()

time.sleep(0.5)
vfb.terminate()

if width != 1024 or height != 768:
    sys.exit(format("Error: incorrect screen size %dx%d, Expected: 1024x768" % (width, height)))

