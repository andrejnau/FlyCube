import os
import subprocess
import sys

def runCmd(arg):
    cmd = subprocess.Popen(arg)
    runcode = None
    while runcode is None:
        runcode = cmd.poll()

def apply_compress_for_dir(compressonator, tex_path, out_dir):
    files = [f for f in os.listdir(tex_path) if os.path.isfile(os.path.join(tex_path, f))]
    for f in files:
        if not f.lower().endswith(('.png', '.jpg', '.jpeg')):
            continue
        print(f)
        extension = os.path.splitext(f)[1]
        nef_file = f.replace(extension, ".dds")
        #runCmd([compressonator, "-fd", "BC7", "-miplevels", "100", os.path.join(tex_path, f),  os.path.join(out_dir, nef_file)])
        runCmd([compressonator, "-format", "dds", "-define", "dds:mipmaps=100", os.path.join(tex_path, f),  os.path.join(out_dir, nef_file)])
        if tex_path == out_dir:
            os.remove(os.path.join(tex_path, f))

def main():
    outdir = sys.argv[2]
    if len(sys.argv) > 3:
        outdir =  sys.argv[3]
    apply_compress_for_dir(sys.argv[1], sys.argv[2], outdir)

if __name__ == "__main__":
    main()
