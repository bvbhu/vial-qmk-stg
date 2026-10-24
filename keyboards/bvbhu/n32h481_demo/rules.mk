# n32h481_demo uses a custom matrix scan (CUSTOM_MATRIX = lite):
# the slide switch on KEY1/KEY2 is read as a 1-key matrix (KEY2 driven
# low, KEY1 pulled up) and the hall sensor as a second key.
# The keyboard-named source n32h481_demo.c (matrix + rgb_matrix driver)
# is picked up automatically by the build.
CUSTOM_MATRIX = lite
