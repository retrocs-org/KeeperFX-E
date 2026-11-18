#!/usr/bin/env python3
"""Generate the release description from the current version's changelog."""
import argparse
from pathlib import Path
from release import ROOT, changelog_notes, validate_release_notes, ReleaseError

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--write', action='store_true')
    action.add_argument('--check', action='store_true')
    args = parser.parse_args()
    version = (ROOT/'VERSION').read_text().strip()
    text = changelog_notes(ROOT, version)+'\n'
    target = ROOT/'docs/RELEASE-NOTES.md'
    if args.write:
        if target.is_symlink():
            raise ReleaseError('Refusing linked release notes')
        target.write_text(text)
    else:
        validate_release_notes(ROOT, version, target.read_text())
    print('Release notes '+('written.' if args.write else 'verified.'))

if __name__ == '__main__':
    main()
