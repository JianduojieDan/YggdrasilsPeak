import sys,time
sys.path.insert(0,r"E:/UE5.8/UE_5.8/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python")
import remote_execution as r
code=open(sys.argv[1]).read()
e=r.RemoteExecution();e.start()
for i in range(50):
    if e.remote_nodes: break
    time.sleep(0.2)
e.open_command_connection(e.remote_nodes[0]['node_id'])
res=e.run_command(code,unattended=True,exec_mode=r.MODE_EXEC_FILE)
print(res.get('success'), str(res.get('result'))[-600:]); [print(o['output']) for o in res.get('output',[])]
e.stop()
