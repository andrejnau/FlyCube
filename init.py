import os
import subprocess

def run_cmd(arg):
    cmd = subprocess.Popen(arg)
    runcode = None
    while runcode is None:
        runcode = cmd.poll()

def submodule():
    print("git submodule init")
    run_cmd(['git', 'submodule', 'init'])
    print("git submodule update")
    run_cmd(['git', 'submodule', 'update'])

def main():
    print("start init 3rdparty")
    submodule()

if __name__ == "__main__":
    main()
