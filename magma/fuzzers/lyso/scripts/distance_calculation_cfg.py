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

bb_distance = []

def merge_distance_files(cfg_cg_path):
        for dist in cfg_cg_path.glob("*.distances.txt"):
            with dist.open("r") as f:
                for line in f.readlines():
                    s = line.strip().split(',')
                    source_bb = s[0]
                    target_bb = s[1]
                    distance = s[2]
                    bb_distance.append([[int(target_bb), int(source_bb)], float(distance)])

def sort_distance(output):
    
    bb_distance.sort()
    with output.open('w') as f:
        for item in bb_distance:
            f.write(str(item[0][0]) + "," + str(item[0][1]) + "," + str(item[1]) + "\n")


def exec_distance_cfg(dot, out, bbnames, fnames, cg_distance, cg_callsites, fun2bb_distance):

    prog = DIST_PY
    cmd = [prog,
           "-d", dot,
           "-o", out,
           "-b", bbnames,
           "-f", fnames,
           "-c", cg_distance,
           "-s", cg_callsites,
           "-t", fun2bb_distance]
    #pipe = subprocess.PIPE
    r = subprocess.run(cmd, check=True)
    return r





def calculating_distances_cfg(args):

    dot_files = args.temporary_directory / DOT_DIR_NAME
    bbcalls = args.temporary_directory / "BBcalls.txt"
    bbnames = args.temporary_directory / "BBnames.txt"
    fnames = args.temporary_directory / "Fnames.txt"
    bbtargets = args.temporary_directory / "BBtargets.txt"
    callgraph = dot_files / CALLGRAPH_NAME
    callgraph_distance = args.temporary_directory / "callgraph.distance.txt"
    fun2bb_distance = args.temporary_directory / "fun2bb.txt"    
 
    with callgraph.open("r") as f:
        callgraph_dot = f.read()

    #Helper function
    def calculate_cfg_distance_from_file(cfg: Path):
        if cfg.stat().st_size == 0: return
        name = cfg.name.split('.')[-2]
        if name not in callgraph_dot: return
        outname = name + ".distances.txt"
        outpath = cfg.parent / outname
        exec_distance_cfg(cfg, outpath, bbnames, fnames, callgraph_distance, bbcalls, fun2bb_distance)

    print("Computing distance for control-flow graphs (this might take a while)")
    with ThreadPoolExecutor(max_workers=mp.cpu_count()) as executor:
         results = executor.map(calculate_cfg_distance_from_file, dot_files.glob("cfg.*.dot"))
         
    try:
         for r in results: pass
    except subprocess.CalledProcessError as err:
           print("An exception occurred:\n")
           print(err.stderr.decode())
           sys.exit(1)

    print("Done computing distance for CFG and merge the distance files..")
    merge_distance_files(dot_files)
    sort_distance(args.temporary_directory / "distance.cfg.txt")

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

    DIST_PY = args.script_directory / "distance_cfg.py"

    calculating_distances_cfg(args)


