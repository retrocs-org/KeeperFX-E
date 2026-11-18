#!/usr/bin/env python3
"""Export code and project UI assets without Git history or imported game content."""
import argparse
import shutil
from pathlib import Path
from release_support import source_files, ReleaseError
from source_policy import excluded
from release import ROOT, module

def export(root, destination):
    destination = destination.resolve()
    # A standalone folder cannot accidentally inherit the original Git history.
    if destination == root.resolve() or root.resolve() in destination.parents or destination.exists():
        raise ReleaseError('Choose a new folder outside the original checkout')
    files = {name: data for name, data in source_files(root).items()
             if not excluded(name) and 'gitlink' not in data}
    module(root,'check-sensitive').scan(root, files)
    destination.mkdir(parents=True)
    try:
        for name in files:
            target = destination/name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(root/name, target)
    except Exception:
        shutil.rmtree(destination)
        raise
    return len(files)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    print('Exported '+str(export(ROOT,args.output))+' public source files. No Git history or uploads.')

if __name__ == '__main__':
    main()
