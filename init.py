import os
import zipfile
import subprocess

def unzip(source_filename, dest_dir):
    with zipfile.ZipFile(source_filename) as zf:
        zf.extractall(dest_dir)

def runCmd(arg):
    cmd = subprocess.Popen(arg)
    runcode = None
    while runcode is None:
        runcode = cmd.poll()

def submodule():
    print("git submodule init")
    runCmd(['git', 'submodule', 'init'])
    print("git submodule update")
    runCmd(['git', 'submodule', 'update'])

def zip3rdparty():
    path = os.path.dirname(os.path.realpath(__file__))
    path = os.path.join(path + "/3rdparty")

    for file in os.listdir(path):
        if file.endswith(".zip"):
            print("unzip:", file)
            unzip(os.path.join(path + "/" + file), path + "/unpacked")

def main():
    print("start init 3rdparty lib")
    zip3rdparty()

if __name__ == "__main__":
    main()
