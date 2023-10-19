

### Modification on original QuickJS
To facilitate effective auditing, here we list the changes upon original
QuickJS. The original version of QuickJS is from:
https://bellard.org/quickjs/quickjs-2021-03-27.tar.xz

With following modifications in `quickjs` folder:
1. The qjs.c is rewrote because it becomes a script on CKB
2. The following files are copied from original QuickJS with very small modifications:
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

3. The following files are removed from original QuickJS:
    - quickjs-libc.c
    - quickjs-libc.h

4. The following files are added:
    - ckb_module.c
    - ckb_module.h
    - mocked.c
    - mocked.h
    - std_module.c
    - std_module.h


### Dynamic Library Issue
It is possible to load RISC-V binaries as dynamic libraries on ckb-vm. However,
using ckb-js-vm as a dynamic library is not recommended due to security issues.

If ckb-js-vm is used as a dynamic library, a host script can load it and execute
JavaScript code. However, this setup can potentially be exploited by malicious
JavaScript code, leading to memory overflow issues in QuickJS. It is important
to note that in this scenario, the host script and the dynamic library share the
same memory space, which means that the host script may be compromised,
resulting in unexpected behavior or skipped checks.

To mitigate this security risk, it is strongly advised to use ckb-js-vm with the
`spawn` or `exec` methods. This allows ckb-js-vm to run in a separate ckb-vm
instance, preventing malicious JavaScript code from accessing or modifying
anything in the host script.


