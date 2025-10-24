# -*- Python -*-

import pyautogui
import unittest
pyautogui.PAUSE = 0.2


class PyAutoGUIChecks(unittest.TestCase):

    def testScreenSize(self):
        """Confirm that the Xvfb size is 1024x768 as expected"""
        width, height = pyautogui.size()
        self.assertEqual(width, 1024)
        self.assertEqual(height, 768)

    def testMousePosition(self):
        """Confirm that mouse position can be detected and the mouse moved"""

        # by default the mouse is in the middle of the screen
        x, y = pyautogui.position()
        width, height = pyautogui.size()
        self.assertEqual(x, width/2)
        self.assertEqual(y, height/2)

        # now move mouse to an absolute position
        pyautogui.moveTo(100,100)
        x, y = pyautogui.position()
        self.assertEqual(x, 100)
        self.assertEqual(y, 100)

        # now move mouse by a relative amount
        pyautogui.moveRel(250,125)
        x, y = pyautogui.position()
        self.assertEqual(x, 350)
        self.assertEqual(y, 225)

##############################
if __name__ == "__main__":
    unittest.main(verbosity=2)
