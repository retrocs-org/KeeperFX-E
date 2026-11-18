from pathlib import Path
import os,tempfile,subprocess
root=Path(__file__).resolve().parents[2]
exe=Path(os.environ.get('KFX_TEST_BOOTSTRAP',Path('/Applications/KeeperFX-E.app/Contents/MacOS/KeeperFXE')))
with tempfile.TemporaryDirectory(prefix='kfx-import-fixture-') as folder:
 folder=Path(folder);prefs=folder/'Profile';resources=folder/'Resources';game=resources/'game';game.mkdir(parents=True)
 (game/'keeperfx.cfg').write_text('RENDERER=SOFTWARE\n');(game/'marker.dat').write_text('version one');(resources/'asset-version.txt').write_text('1')
 disc=folder/'Disc'/'KEEPER'
 for group,names in {'DATA':['bluepal.dat','bluepall.dat','dogpal.pal','hitpall.dat','lightng.pal','redpal.col','redpall.dat','slab0-0.dat','slab0-1.dat','vampal.pal','whitepal.col'],'SOUND':['atmos1.sbk','atmos2.sbk','bullfrog.sbk']}.items():
  (disc/group).mkdir(parents=True)
  for name in names:(disc/group/name.upper()).write_bytes(b'fixture')
 env=dict(os.environ,KFX_USER_DATA_DIR=str(prefs),KFX_RESOURCES_DIR=str(resources))
 def call(*args,ok=True):
  r=subprocess.run([str(exe),*map(str,args)],env=env,capture_output=True,text=True)
  assert (r.returncode==0)==ok,(r.returncode,r.stderr)
 call('--import',folder/'Disc');call('--prepare-only')
 runtime=prefs/'GameData';(runtime/'keeperfx.cfg').write_text('RENDERER=OPENGL\n');(runtime/'cache.dat').write_bytes(b'keep')
 (resources/'asset-version.txt').write_text('2');(game/'marker.dat').write_text('version two')
 call('--prepare-only')
 assert (runtime/'marker.dat').read_text()=='version two'
 assert (runtime/'keeperfx.cfg').read_text()=='RENDERER=OPENGL\n'
 assert (runtime/'cache.dat').read_bytes()==b'keep'
 (disc/'DATA/SLAB0-0.DAT').write_bytes(b'');call('--import',folder/'Disc',ok=False)
 assert (prefs/'OriginalData/data/slab0-0.dat').read_bytes()==b'fixture'
 print('PASS: uppercase mounted-CD layout, updates refresh assets and preserve settings/cache, zero-length file rejected without losing existing import')
