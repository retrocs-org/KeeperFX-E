#!/usr/bin/env python3
"""Headless KeeperFX host supervisor. Standard library, GPL-2.0-or-later.
One synchronized AI host + up to three human players. Does not claim a fifth slot.
"""
import argparse,base64,fcntl,json,os,re,secrets,signal,socket,subprocess,sys,threading,time,urllib.request
from pathlib import Path

def request(base,route,data=None,token=''):
    headers={'Content-Type':'application/json'}
    if token:headers['Authorization']='Bearer '+token
    req=urllib.request.Request(base+route,data=json.dumps(data).encode() if data is not None else None,headers=headers)
    with urllib.request.urlopen(req,timeout=5) as r:return json.load(r)

def rpc(port):
    with socket.create_connection(('127.0.0.1',port),timeout=2) as c:
        c.sendall(b'{"action":"get_network_info","ack":1}\n');raw=b''
        while b'\n' not in raw and len(raw)<65536:raw+=c.recv(4096)
        value=json.loads(raw.split(b'\n',1)[0]);return value.get('data',{}) if value.get('success') else {}

def validate(config):
    for field in ('engine_binary','user_data','hub_url','name','endpoint','rotation'):assert field in config,'Missing '+field
    engine=Path(config['engine_binary']).resolve();profile=Path(config['user_data']).resolve();assert engine.is_file() and os.access(engine,os.X_OK),'Engine is not executable';assert (profile/'GameData/keeperfx.cfg').is_file(),'Prepare a separate dedicated profile first';assert '.app/' not in str(profile),'User data must be outside the application';assert 1<=config.get('humans',1)<=3,'Dedicated mode supports 1–3 humans plus its AI';assert config['rotation'] and len(config['rotation'])<=32,'Choose 1–32 maps'
    for item in config['rotation']:assert re.fullmatch('[A-Za-z0-9_-]{1,63}',item['campaign']) and type(item['level'])==int and 0<item['level']<=99999,'Invalid map'
    assert config['hub_url'].startswith(('http://127.0.0.1:','http://localhost:','https://')),'Use local HTTP or HTTPS';return engine,profile

class Host:
    def __init__(self,config):self.config=config;self.engine,self.profile=validate(config);self.stop=False;self.next=False;self.process=None;self.index=0;self.status={};self.lock=threading.Lock()
    def command(self,line):
        try:
            command=json.loads(line).get('command')
            if command=='stop':self.stop=True
            elif command=='next':self.next=True
            elif command=='status':print(json.dumps(self.status),flush=True)
            else:print('Unknown command; use status, next or stop',file=sys.stderr)
        except (ValueError,AttributeError):print('Expected a JSON command',file=sys.stderr)
    def finish_child(self):
        if self.process and self.process.poll() is None:
            self.process.terminate()
            try:self.process.wait(10)
            except subprocess.TimeoutExpired:self.process.kill();self.process.wait(5)
    def run(self):
        state=self.profile/'dedicated';state.mkdir(mode=0o700,exist_ok=True)
        with (state/'host.lock').open('a') as lock:
            os.chmod(state/'host.lock',0o600)
            try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
            except BlockingIOError:raise RuntimeError('This dedicated profile already has a running host')
            self._run_locked()
    def _run_locked(self):
        cfg=self.config;base=cfg['hub_url'].rstrip('/');key=os.environ.get('KFX_HUB_ADMIN_KEY','');assert len(key)>=24,'Set KFX_HUB_ADMIN_KEY to this local service’s administrator key';state=self.profile/'dedicated';state.mkdir(mode=0o700,exist_ok=True)
        stopfile=state/'stop.request';nextfile=state/'next.request';stopfile.unlink(missing_ok=True);nextfile.unlink(missing_ok=True)
        while not self.stop:
            item=cfg['rotation'][self.index%len(cfg['rotation'])];self.index+=1;self.next=False
            with socket.socket() as sock:sock.bind(('127.0.0.1',0));api_port=sock.getsockname()[1]
            runtime=self.profile/'GameData';gamecfg=runtime/'keeperfx.cfg';text=gamecfg.read_text();text=re.sub(r'(?m)^API_ENABLED=.*$','API_ENABLED=TRUE',text);text=re.sub(r'(?m)^API_PORT=.*$',f'API_PORT={api_port}',text);gamecfg.write_text(text)
            room=request(base,'/api/servers/register',{'name':cfg['name'],'endpoint':cfg['endpoint'],'engine':cfg.get('engine_version','1.4.0.0'),'kind':'dedicated','admin_key':key,'map':item['campaign']+':'+str(item['level']),'rotation':[v['campaign']+':'+str(v['level']) for v in cfg['rotation']],'password':bool(os.environ.get('KFX_SERVER_PASSWORD'))})
            env=dict(os.environ,KFX_USER_DATA_DIR=str(self.profile),KFX_HEADLESS='1',KFX_LOCAL_NETWORK_ONLY='1',SDL_VIDEODRIVER='dummy',SDL_RENDER_DRIVER='software',SDL_AUDIODRIVER='dummy',ALSOFT_DRIVERS='null',KFX_HUB_URL=base,KFX_HUB_SERVER_ID=room['id'],KFX_HUB_SERVER_SECRET=room['secret'])
            if cfg.get('app_resources'):env['KFX_APP_RESOURCES']=cfg['app_resources']
            replay=state/(room['id']+'.pck');port=int(cfg.get('port',5556));args=[str(runtime/'keeperfx'),'-nointro','-server',str(port),'-campaign',item['campaign'],'-level',str(item['level']),'-waitusers',str(cfg.get('humans',1)+1),'-nosound','-nick',cfg.get('ai_name','ServerAI'),'-packetsave',str(replay),'-log','dedicated.log']
            log=open(state/'engine-output.log','ab');self.process=subprocess.Popen(args,executable=str(self.engine),cwd=runtime,env=env,stdout=log,stderr=subprocess.STDOUT);offset=0;had_game=False;failures=0;verified=False;stream_truncated=False
            print('Headless lobby ready:',cfg['name'],'map',item['campaign'],item['level'],flush=True)
            try:
                while not self.stop and not self.next and self.process.poll() is None:
                    try:
                        if stopfile.exists():self.stop=True;stopfile.unlink();break
                        if nextfile.exists():self.next=True;nextfile.unlink();break
                        status=rpc(api_port);self.status={**status,'server_id':room['id'],'map':item};(state/'status.json').write_text(json.dumps(self.status));failures=0
                        if status.get('turn',0)>0 and status.get('active'):had_game=True
                        if cfg.get('spectators',False) and replay.exists() and not stream_truncated:
                            with replay.open('rb') as f:f.seek(offset);raw=f.read(32768)
                            if raw:answer=request(base,'/api/servers/'+room['id']+'/stream',{'offset':offset,'data':base64.b64encode(raw).decode()},room['secret']);offset=answer['offset'];stream_truncated=bool(answer.get('truncated'))
                        humans=[v for v in status.get('users',[]) if v.get('peer')!=0]
                        ended=had_game and (not status.get('active') or humans and all(v.get('outcome') in (1,2) for v in humans))
                        if ended:
                            identified=[{'user':v['account'],'result':'won' if v['outcome']==1 else 'lost'} for v in humans if v.get('account') and v.get('outcome') in (1,2)]
                            if identified and all(v.get('verified_finish') for v in humans):
                                try:request(base,'/api/servers/'+room['id']+'/result',{'match':room['match_id'],'results':identified,'cooperative':bool(cfg.get('cooperative',False))},room['secret']);verified=True
                                except Exception:pass # Unverified/disconnected games never become ranked wins.
                            break
                    except (OSError,ValueError):
                        failures+=1
                        if failures>30:raise RuntimeError('Headless engine status unavailable for one minute')
                    time.sleep(1)
            finally:
                self.finish_child();log.close()
                if cfg.get('spectators',False) and replay.exists() and not stream_truncated:
                    with replay.open('rb') as f:
                        f.seek(offset)
                        while True:
                            raw=f.read(32768);finished=not raw
                            try:answer=request(base,'/api/servers/'+room['id']+'/stream',{'offset':offset,'data':base64.b64encode(raw).decode(),'finished':finished},room['secret']);offset=answer['offset'];stream_truncated=bool(answer.get('truncated'))
                            except Exception:break
                            if finished:break
                try:request(base,'/api/servers/'+room['id']+'/close',{},room['secret'])
                except Exception:pass
            if self.process.returncode and not self.stop and not self.next:raise RuntimeError('Engine exited unexpectedly; inspect dedicated/engine-output.log')

def main():
    p=argparse.ArgumentParser();p.add_argument('config',type=Path);p.add_argument('--check',action='store_true');a=p.parse_args();cfg=json.loads(a.config.read_text());validate(cfg)
    if a.check:print('Dedicated configuration is valid');return
    host=Host(cfg);signal.signal(signal.SIGTERM,lambda *_:setattr(host,'stop',True));signal.signal(signal.SIGINT,lambda *_:setattr(host,'stop',True))
    def commands():
        for line in sys.stdin:host.command(line)
    threading.Thread(target=commands,daemon=True).start()
    try:host.run()
    finally:host.finish_child()
if __name__=='__main__':main()
