# Usage: python -I uepy.py <file.py>  -> runs file in the running editor, prints output
import sys, time
sys.path.append(r'E:\UE5.8\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python')
import remote_execution as re_
cfg = re_.RemoteExecutionConfig()
r = re_.RemoteExecution(cfg); r.start()
for _ in range(50):
    if r.remote_nodes: break
    time.sleep(0.2)
if not r.remote_nodes:
    print('NO EDITOR NODE FOUND'); r.stop(); sys.exit(1)
r.open_command_connection(r.remote_nodes[0]['node_id'])
code = open(sys.argv[1], encoding='utf-8').read()
res = r.run_command(code, unattended=True, exec_mode=re_.MODE_EXEC_FILE)
for o in res.get('output', []): print(o.get('output', '').rstrip())
print('RESULT', res.get('success'), res.get('result'))
r.stop()
