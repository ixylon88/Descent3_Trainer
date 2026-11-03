#!/usr/bin/env python3
"""SPDX-License-Identifier: GPL-3.0-or-later
Check actual PE headers and imports; this is not a Windows 98 execution test.
"""
import struct
import sys
from pathlib import Path

ALLOW = {
    'comctl32.dll': {'InitCommonControls'},
    'comdlg32.dll': {'CommDlgExtendedError','GetOpenFileNameA','GetSaveFileNameA'},
    'gdi32.dll': {'CreateFontA','DeleteObject','GetStockObject'},
    'kernel32.dll': {'CloseHandle','CreateFileA','DeleteFileA','ExitProcess','FlushFileBuffers',
        'GetCommandLineA','GetFileAttributesA','GetFileSize','GetFullPathNameA','GetLocalTime',
        'GetModuleHandleA','GetProcessHeap','GetTempFileNameA','HeapAlloc','HeapFree',
        'MoveFileA','ReadFile','SetFilePointer','WriteFile','lstrcmpiA'},
    'user32.dll': {'AdjustWindowRectEx','CreateWindowExA','DefWindowProcA','DestroyWindow',
        'DispatchMessageA','EnableWindow','GetMessageA','GetSystemMetrics','GetWindowTextA',
        'IsDialogMessageA','LoadCursorA','LoadIconA','MessageBoxA','PostQuitMessage',
        'RegisterClassA','SendMessageA','SetWindowTextA','ShowWindow','SystemParametersInfoA',
        'TranslateMessage','UpdateWindow','wsprintfA'},
}

def audit(path):
    data = Path(path).read_bytes()
    u16 = lambda p: struct.unpack_from('<H',data,p)[0]
    u32 = lambda p: struct.unpack_from('<I',data,p)[0]
    assert data[:2] == b'MZ'
    pe = u32(0x3c)
    assert data[pe:pe+4] == b'PE\0\0'
    assert u16(pe+4) == 0x14c, 'not 32-bit x86'
    opt = pe+24
    assert u16(opt) == 0x10b, 'not PE32'
    assert (u16(opt+40),u16(opt+42)) == (4,0), 'unexpected OS version'
    assert (u16(opt+48),u16(opt+50)) == (4,0), 'unexpected subsystem version'
    assert u16(opt+68) == 2, 'not a GUI application'
    assert u16(opt+70) == 0, 'unexpected modern DLL characteristics'
    for index,name in [(9,'TLS'),(10,'load configuration'),(14,'.NET CLR')]:
        assert u32(opt+96+index*8) == 0, 'unexpected '+name
    sections=[]
    for i in range(u16(pe+6)):
        p=opt+u16(pe+20)+40*i
        sections.append((u32(p+12),max(u32(p+8),u32(p+16)),u32(p+20)))
    def offset(rva):
        for start,size,filepos in sections:
            if start<=rva<start+size:return filepos+rva-start
        raise AssertionError('RVA outside sections')
    def z(p):
        return data[p:data.index(0,p)].decode('ascii')
    imports={}
    p=offset(u32(opt+104))
    while u32(p+12):
        dll=z(offset(u32(p+12))).lower()
        assert dll in ALLOW, 'unexpected runtime dependency '+dll
        q=offset(u32(p) or u32(p+16))
        functions=[]
        while u32(q):
            value=u32(q)
            assert not value&0x80000000, 'ordinal import requires separate review'
            function=z(offset(value)+2)
            assert function in ALLOW[dll], 'unexpected import '+dll+'!'+function
            functions.append(function);q+=4
        imports[dll]=functions;p+=20
    assert set(imports)==set(ALLOW)
    print('PASS: PE32/x86, Windows GUI, OS 4.0, subsystem 4.0')
    print('PASS: no CLR, TLS initialization, modern load configuration or CRT DLL')
    for dll,functions in imports.items():print('PASS:',dll,':',', '.join(functions))
    print('PASS: all',sum(map(len,imports.values())),'imports match the classic API allowlist')
    print('NOTE: this static audit does not prove runtime behavior on Windows 98.')

if __name__=='__main__':audit(sys.argv[1] if len(sys.argv)>1 else 'D3EDIT98.EXE')
