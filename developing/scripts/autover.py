"""
  [AutoVer] by K-Nana
  Say goodbye to incorrect versioning.
  By setting it to run automatically, you can insert the date written in the program into a variable.
  MIT License https://opensource.org/license/mit
"""

#Import("env")
import tempfile
import subprocess

VER_MARKER = "; // [version]\n"
ASSIGNMENT = "= "
TARGET = "./src/main.cpp"

def main():
  version = subprocess.check_output(["python3", "./developing/scripts/vergenerator.py"]).decode().strip()
  targetFile = open(TARGET, mode='r', encoding="utf-8")
  temp = tempfile.TemporaryFile(mode='w+', encoding="utf-8")
  temp.write(targetFile.read())
  temp.seek(0)
  targetFile.close()
  targetFile = open(TARGET, mode='w', encoding="utf-8")
  for line in temp:
    if line.endswith(VER_MARKER):
      subtracted = line[:-len(VER_MARKER)]
      splitted = subtracted.split(ASSIGNMENT)
      targetFile.write(splitted[0]+ASSIGNMENT+version+VER_MARKER)
    else:
      targetFile.write(line)

if __name__ == "SCons.Script":
  main()