import os
import sys
import shutil
import platform
import subprocess

def get_path():
    return os.path.dirname(os.path.realpath(__file__))

def runCmd(arg):
    cmd = subprocess.Popen(arg)
    runcode = None
    while runcode is None:
        runcode = cmd.poll()

def gen():
    print ("gen")
    #clean()
    path = get_path()
    if not os.path.exists(os.path.join(path, "build")):
        os.mkdir(os.path.join(path, "build"))
    os.chdir(os.path.join(path, "build"))
    generate_out = "Unix Makefiles"
    if any(platform.win32_ver()):
        generate_out = "Visual Studio 12 2013"
    runCmd(['cmake', '..', '-G', generate_out])
    os.chdir(path)

def compilation():
    print ("compilation")
    runCmd(['cmake', '--build', './build'])

def build():
    print ("build") 
    clean()   
    gen()
    compilation()

def clean():
    print ("clean")
    path = get_path()
    dirs = ["bin", "build", "lib"]
    for name in dirs:
        shutil.rmtree(os.path.join(path, name), ignore_errors = True)  
    
def help():
    print ("build.py [--gen, --build(default), --clean")

def main():
    action = build

    if len(sys.argv) == 2:
        opt = sys.argv[1]   
        if opt == "--gen":
            action = gen
        elif opt == "--clean":
            action = clean
        elif opt == "--build":
            action = build  
        else:
           action = help
    elif len(sys.argv) > 2:  
        action = help

    action()

if __name__ == "__main__":
    main()