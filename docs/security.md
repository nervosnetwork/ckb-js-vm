

### Modification on original QuickJS
The original version of quickjs is from: 
https://bellard.org/quickjs/quickjs-2021-03-27.tar.xz

with following modifications in `quickjs` folder:
1. The qjs.c is rewrote because it becomes a script on CKB
2. The following files are copied from original quickjs with very small modifications:
    - cutils.c
    - cutils.h
    - libbf.c
    - libbf.h
    - libregexp-opcode.h
    - libregexp.c
    - libregexp.h
    - libunicode-table.h
    - libunicode.c
    - libunicode.h
    - list.h
    - quickjs-atom.h
    - quickjs-opcode.h
    - quickjs.h
    - quickhs.c
    
    The major modifications:
    1. header file including
    2. A lot of binding functions are removed
    3. Macros for other platforms
    4. Replace `alloca` function

3. The following files are removed from original quickjs:
    - quickjs-libc.c
    - quickjs-libc.h

4. The following files are added:
    - ckb_module.c
    - ckb_module.h
    - mocked.c
    - mocked.h
    - std_module.c
    - std_module.h
