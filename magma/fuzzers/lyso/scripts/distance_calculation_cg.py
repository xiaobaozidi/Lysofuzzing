#!/usr/bin/env python3

import argparse
import sys 
import subprocess
from concurrent.futures import ThreadPoolExecutor
import multiprocessing as mp
from argparse import ArgumentTypeError as ArgTypeErr
from pathlib import Path


DOT_DIR_NAME = "dot-files"
CALLGRAPH_NAME = "callgraph.dot"

def calculating_distances_cg(args):

    dot_files = args.temporary_directory / DOT_DIR_NAME
    fnames = args.temporary_directory / "Fnames.txt"
    ftargets = args.temporary_directory / "Ftargets.txt"
    btargets = args.temporary_directory / "Btargets.txt"
    cg_callsite = args.temporary_directory / "BBcalls.txt"
    callgraph = dot_files / CALLGRAPH_NAME
    callgraph_distance = args.temporary_directory / "callgraph.distance.txt"
    rev_callgraph_distance = args.temporary_directory / "rev_callgraph.distance.txt"
    fun2bb = args.temporary_directory / "fun2bb.txt"    
   
    print("Computing distance for callgraph\n")
    try:
        r = exec_distance_cg(callgraph, ftargets, btargets, callgraph_distance, rev_callgraph_distance, 
                             fun2bb, fnames, cg_callsite, args.temporary_directory)

    except subprocess.CalledProcessError as err:
        print("An exception occurred:\n")
        #print(err.stderr.decode())
        sys.exit(1)

    if not callgraph_distance.exists():
        print("callgraph distance calculation fails\n")
          


def exec_distance_cg(dot, ftargets, btargets, callgraph_distance, rev_callgraph_distance, fun2bb, fnames, cg_callsite, tmp_dir):
    
    prog = DIST_PY
    cmd = [prog,
           "-d", dot,
           "-f", ftargets,
           "-b", btargets,
           "-w", callgraph_distance,
           "-r", rev_callgraph_distance,
           "-o", fun2bb,
           "-n", fnames,
           "-s", cg_callsite,
           tmp_dir]
    print(cmd) 
    #pipe = subprocess.PIPE
    r = subprocess.run(cmd, check=True)
    return r


# -- Argparse --
def is_path_to_dir(path):
    """Returns Path object when path is an existing directory"""
    p = Path(path)
    if not p.exists():
        raise ArgTypeErr("path doesn't exist")
    if not p.is_dir():
        raise ArgTypeErr("not a directory")
    return p
# ----


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument("script_directory", metavar="script-directory",
                        type=is_path_to_dir,
                        help="Root directory of script")

    parser.add_argument("temporary_directory", metavar="temporary-directory",
                        type=is_path_to_dir,
                        help="Directory where dot files and target files are "
                             "located")

    args = parser.parse_args();
    
    DIST_PY = args.script_directory / "distance_cg.py"

    calculating_distances_cg(args)

