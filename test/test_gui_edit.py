# -*- Python -*-

import os
import pyautogui
import subprocess
import time
import unittest
from PIL import Image

pyautogui.PAUSE = 0.2

def tuple_compare(one,two,delta):
    """Compare two RGB triples to determine if they differ less than delta"""
    if abs(one[0]-two[0]) < delta and \
       abs(one[1]-two[1]) < delta and \
       abs(one[2]-two[2]) < delta:
        return True
    else:
        return False

def check_image(fname, color, delta):
    """
    Check if a square of four pixels of an image have the expected color
    with a given delta.  Delete the image if the pixels match.
    """
    with Image.open(fname) as im:
        im = im.convert("RGB")
        px = im.load()
        # check format, size, and that it is all black
        if tuple_compare(px[100,100], color, delta) and \
           tuple_compare(px[100,500], color, delta) and \
           tuple_compare(px[500,100], color, delta) and \
           tuple_compare(px[500,500], color, delta):
            os.remove(fname)
            return True
        else:
            print(px[100,100], px[100,500], px[500,100], px[500,500], color)
            return False

class GUIEditorChecks(unittest.TestCase):
    """This set of tests exercises some basic editor tasks"""

    # handle for LAMMPS-GUI process
    gui=None

    def setUp(self):
        """Launch LAMMPS-GUI"""
        self.gui=subprocess.Popen(os.environ['LAMMPS_GUI'])
        time.sleep(1.0)
        # move mouse inside the editor widget and get focus
        pyautogui.click(button='left', x=100, y=100)

    def tearDown(self):
        """Stop LAMMPS-GUI"""
        if not self.gui.poll():
            self.gui.terminate()

    def testExitShortcut(self):
        """Exit LAMMPS-GUI immediately via keyboard shortcut"""
        subprocess.check_output("shooter shot1.png", shell=True, stderr=subprocess.STDOUT)
        pyautogui.hotkey('ctrl','q')
        subprocess.check_output("shooter shot2.png", shell=True, stderr=subprocess.STDOUT)
        self.assertEqual(self.gui.poll(), 0)
        self.assertTrue(check_image('shot1.png', (255,255,255), 10))
        self.assertTrue(check_image('shot2.png', (0,0,0), 1))

    def testExitMenu(self):
        """Exit LAMMPS-GUI immediately via the menu"""
        subprocess.check_output("shooter shot1.png", shell=True, stderr=subprocess.STDOUT)
        pyautogui.hotkey('alt','f')
        pyautogui.keyDown('q')
        pyautogui.keyUp('q')
        subprocess.check_output("shooter shot2.png", shell=True, stderr=subprocess.STDOUT)
        self.assertEqual(self.gui.poll(), 0)
        self.assertTrue(check_image('shot1.png', (255,255,255), 10))
        self.assertTrue(check_image('shot2.png', (0,0,0), 1))

##############################
if __name__ == "__main__":
    unittest.main(verbosity=2)
