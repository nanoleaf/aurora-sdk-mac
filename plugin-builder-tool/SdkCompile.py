import os
import subprocess
import tkMessageBox
import glob

version_marker = "4538474992"

def check_if_sdk_file_built(plugin_dir):
    makefile_dir = plugin_dir + "/Debug/"
    os.chdir(makefile_dir)
    lib_name = ""
    for filename in glob.glob("*.so"):
        lib_name = filename
        break
    lib_path = makefile_dir + lib_name
    return os.path.exists(lib_path)

def sdk_compile(plugin_dir):
    makefile_dir = plugin_dir + "/Debug/"
    print(makefile_dir)
    make_clean_process = subprocess.Popen(["make", "clean"], cwd=makefile_dir, stdout=subprocess.PIPE)
    while True:
        line = make_clean_process.stdout.readline()
        if line != "":
            print (line)
        else:
            break
    make_all_process = subprocess.Popen(["make", "all"], cwd=makefile_dir, stdout=subprocess.PIPE)
    while True:
        line = make_all_process.stdout.readline()
        if line != "":
            print (line)
        else:
            break

    plugin_built = check_if_sdk_file_built(plugin_dir)
    if not plugin_built:
        tkMessageBox.showinfo("Build result", "Build Failed")
    else:
        tkMessageBox.showinfo("Build result", "Build Successful")
