# -*- Python -*-

import os, sys
import subprocess
import time

# launch virtual frame buffer
subprocess.check_output("rm -f shot.png", shell=True, stderr=subprocess.STDOUT)
vfb = subprocess.Popen(["Xvfb", ":1", "-screen", "0", "1024x768x24"])
os.environ["DISPLAY"] = ":1.0"
time.sleep(0.5)

if (len(sys.argv) < 2):
    vfb.terminate()
    sys.exit("Error: missing command line arguments")

subprocess.check_output(format("%s shot.png" % sys.argv[1]), shell=True, stderr=subprocess.STDOUT)
time.sleep(0.5)
vfb.terminate()

if not os.path.exists("shot.png"):
    sys.exit("Error: screenshot not created")
os.remove("shot.png")
