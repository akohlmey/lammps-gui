# -*- Python -*-

import os, sys
import subprocess
import time

subprocess.check_output("shooter shot.png", shell=True, stderr=subprocess.STDOUT)
time.sleep(0.2)

if not os.path.exists("shot.png"):
    sys.exit("Error: screenshot not created")
os.remove("shot.png")
