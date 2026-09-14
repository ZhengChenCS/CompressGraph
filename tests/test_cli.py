"""Standard-library-only integration test for binary format and legacy filter."""
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

exe, filter_exe = map(lambda p: str(Path(p).resolve()), sys.argv[1:3])
def save(path, values):
    path.write_bytes(struct.pack('<'+'i'*len(values), *values))
def read(path):
    b = path.read_bytes()
    return struct.unpack('<'+'i'*(len(b)//4), b)
def verify(directory, expected):
    ptr, col, info = [read(directory/name) for name in ('csr_vlist.bin','csr_elist.bin','info.bin')]
    n, rules = info
    assert n == len(expected) and len(ptr) == n+rules+1
    for row in range(n):
        stack=list(reversed(col[ptr[row]:ptr[row+1]])); expanded=[]
        while stack:
            symbol=stack.pop()
            if symbol<n: expanded.append(symbol)
            else: stack.extend(reversed(col[ptr[symbol]:ptr[symbol+1]]))
            assert len(expanded)<=len(expected[row])
        assert expanded == expected[row]
with tempfile.TemporaryDirectory() as temp:
    root=Path(temp);v=root/'v.bin';e=root/'e.bin';out=root/'compressed'
    expected=[list(range(64)) for _ in range(64)]
    save(v,[64*i for i in range(65)]);save(e,[x for row in expected for x in row])
    cmd=[exe,str(v),str(e),str(out),'--threads','4','--repeat','2','--verify']
    result=json.loads(subprocess.check_output(cmd,text=True))
    assert result['verified'] and result['rules']>0 and len(result['times'])==2
    verify(out,expected)
    original=(out/'csr_elist.bin').read_bytes()
    assert subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE).returncode != 0
    assert (out/'csr_elist.bin').read_bytes()==original
    filtered=root/'filtered';filtered.mkdir()
    subprocess.run([filter_exe,str(out/'csr_vlist.bin'),str(out/'csr_elist.bin'),str(out/'info.bin'),'16'],cwd=filtered,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    verify(filtered,expected)
    if len(sys.argv)>3:
        serial=str(Path(sys.argv[3]).resolve())
        outputs=[]
        for label,options in [('default',[]),('two',['2']),('sixteen',['16'])]:
            dest=root/label;dest.mkdir()
            subprocess.run([serial,str(v),str(e)]+options,cwd=dest,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            verify(dest,expected);outputs.append(dest)
        for name in ['csr_vlist.bin','csr_elist.bin','info.bin']:
            assert (outputs[0]/name).read_bytes()==(outputs[1]/name).read_bytes()
    v.write_bytes(b'x')
    assert subprocess.run([exe,str(v),str(e),str(root/'bad')],stdout=subprocess.PIPE,stderr=subprocess.PIPE).returncode != 0
    save(v,[0]);save(e,[])
    subprocess.run([exe,str(v),str(e),str(root/'empty'),'--verify'],check=True,stdout=subprocess.PIPE)
    verify(root/'empty',[])
print('CLI format, legacy filter, exactness, empty input and error handling passed')
