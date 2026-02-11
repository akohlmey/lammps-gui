## -*- Python -*- ######################################################################
## LAMMPS-GUI - A Graphical Tool to Learn and Explore the LAMMPS MD Simulation Software
##
## Copyright (c) 2023, 2024, 2025, 2026  Axel Kohlmeyer
##
## Documentation: https://lammps-gui.lammps.org/
## Contact: akohlmey@gmail.com
##
## This software is distributed under the GNU General Public License version 2 or later.
########################################################################################

import os
import subprocess
import pyautogui
import unittest
from PIL import Image
pyautogui.PAUSE = 0.2

class ScreenshotChecks(unittest.TestCase):
    """Tests for the 'shooter' screenshot wrapper"""

    def setUp(self):
        """Clean up leftover files from previous runs"""
        if os.path.exists("shot.png"):
            os.remove("shot.png")

    def tearDown(self):
        """Clean up after test completion"""
        if os.path.exists("shot.png"):
            os.remove("shot.png")

    def testCreateImage(self):
        """Test if 'shooter' can run and if it will create an all-black PNG image file"""
        subprocess.check_output("shooter shot.png", shell=True, stderr=subprocess.STDOUT)
        width, height = pyautogui.size()
        self.assertTrue(os.path.exists("shot.png"))
        with Image.open("shot.png") as im:
            self.assertEqual(im.format, "PNG")
            self.assertEqual(im.size, (width,height))

            im = im.convert("RGB")
            px = im.load()
            # check format, size, and that it is all black
            self.assertEqual(px[100,100], (0,0,0))
            self.assertEqual(px[100,500], (0,0,0))
            self.assertEqual(px[500,100], (0,0,0))
            self.assertEqual(px[500,500], (0,0,0))

##############################
if __name__ == "__main__":
    unittest.main(verbosity=2)
