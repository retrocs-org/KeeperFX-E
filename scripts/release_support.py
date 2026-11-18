"""Release inventory helpers adapted from Beam68 (MIT)."""
import hashlib,json,os,subprocess
from pathlib import Path
class ReleaseError(RuntimeError):
    pass

SKIP_DIRS = {'.git', '.svn', 'build', 'dist', 'outputs', 'work', '__pycache__',
             '.pytest_cache'}
SKIP_SUFFIXES = {'.o', '.pyc', '.swp', '.zip', '.lha', '.uaem', '.log'}
GENERATED = {'BUILD-PROVENANCE.json', 'RELEASE-BUILD.json', '.DS_Store'}

def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()

def canonical(data):
    return (json.dumps(data, sort_keys=True, indent=2) + '\n').encode()

def atomic_json(path, data):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_symlink() or path.parent.is_symlink():
        raise ReleaseError('Refusing linked release metadata')
    temp = path.with_name(path.name + '.new')
    with temp.open('xb') as stream:
        stream.write(canonical(data))
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temp, path)


def source_files(root, git_env=None):
    # Include tracked and new public source, respecting the project's generated-output ignores.
    result=subprocess.run(['git','ls-files','-co','--exclude-standard','-z'],cwd=root,capture_output=True,check=True,env=git_env)
    staged=subprocess.run(['git','ls-files','--stage','-z'],cwd=root,capture_output=True,text=True,check=True,env=git_env).stdout
    links={entry.split('\t',1)[1]:entry.split()[1] for entry in staged.split('\0') if entry.startswith('160000 ')}
    files={}
    for name in sorted(set(result.stdout.decode().split('\0'))-{''}):
        p=root/name
        if p.name=='.DS_Store':continue
        if name in links:
            files[name]={'gitlink':links[name]};continue
        if p.is_symlink() or not p.is_file():raise ReleaseError('Missing or linked release input: '+name)
        if any(part in {'.git','outputs','.local','third_party','__pycache__'} for part in p.relative_to(root).parts):continue
        if p.name.startswith('.env') or p.suffix.lower() in {'.pem','.key','.p12','.pfx'} or p.name in {'.netrc','.git-credentials','id_rsa','id_ed25519','credentials','release-config.json'}:
            if name!='examples/release-config.json':raise ReleaseError('Private release input: '+name)
        files[name]={'sha256':sha(p),'executable':bool(p.stat().st_mode&0o111)}
    for name in ['VERSION','macos/BUILD_NUMBER','LICENSE','CMakeLists.txt','src/main.cpp','scripts/release.py','macos/package-app.sh']:
        if name not in files:raise ReleaseError('Missing release input: '+name)
    return files

def fingerprint(files):return hashlib.sha256(canonical(files)).hexdigest()
