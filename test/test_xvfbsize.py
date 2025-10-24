# -*- Python -*-

import os, sys
import subprocess
import time
import pyautogui

width, height = pyautogui.size()
time.sleep(0.2)

if width != 1024 or height != 768:
    sys.exit(format("Error: incorrect screen size %dx%d, Expected: 1024x768" % (width, height)))

