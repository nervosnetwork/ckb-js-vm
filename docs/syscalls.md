# CKB Syscalls Bindings

## Partial Loading

The functions here may take a variable length of arguments. There is a [partial
loading](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#partial-loading)
for functions. Functions below with partial loading supported may be
additionally passed to the argument length and offset. The behavior of these JS
functions is classified as follows:

- When both the 'length' and 'offset' parameters are omitted, the default
  behavior is to load the entire data. For instance, invoking the function
  `ckb.load_witness(0, ckb.SOURCE_INPUT)` would result in the loading of the
  complete witness data associated with the input cell at index 0.

- When the 'offset' parameter is omitted, and a 'length' of zero is explicitly
  provided, the function will return the length of the entire data rather than
  the data itself. For instance, invoking `ckb.load_witness(0, ckb.SOURCE_INPUT,
  0)` will return the length of the witness, not the actual witness data.

- When the 'offset' parameter is omitted, and a non-zero 'length' is provided,
  the function will return the initial data consisting of the specified number
  of bytes. For example, invoking `ckb.load_witness(0, ckb.SOURCE_INPUT, 10)` will
  retrieve and return the initial 10 bytes of the witness data.

- When both the 'length' and 'offset' parameters are provided, with the 'length'
  set to zero, the function will return the data length starting from the
  specified offset. For instance, using the function call `ckb.load_witness(0,
  ckb.SOURCE_INPUT, 0, 10)` would retrieve the data length starting from offset
  10.

- When both the 'length' and 'offset' parameters are provided, with the 'length'
  set to a non-zero value, the function will return the data starting from the
  specified offset up to the specified length. For example, using the function
  call `ckb.load_witness(0, ckb.SOURCE_INPUT, 2, 10)` would retrieve 2 bytes of
  data starting from offset 10.

## Error Handling

All of the functions are designed to raise exceptions in the event of an error.

## More Examples

[See CKB syscall test cases](../tests/ckb_js_tests/test_data/syscall.js).

## Functions

When partial loading support is enabled, the description of the 'length' and
'offset' arguments may be omitted. Optionally, you can still pass 'length' and
'offset' when using functions such as load_witness. Here is an example:
```js
let buf = ckb.load_witness(index, source, length, offset);
```
Length and offset can be omitted when reading the full data.

All bindings in this document can be found from [source file](../quickjs/ckb_module.c).

#### `ckb.exit`
Description: exit the ckb-vm execution

Example:
```js
ckb.exit(code)
```

Arguments: code (exit code)

Return value(s): none

Side effects: exit the ckb-vm execution

See also: [`ckb_exit` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#exit)

#### `ckb.mount`
Description: load the cell data and mount the file system specified by `source`
and `index`.

Example:
```js
ckb.mount(source, index)
```

Arguments: source (the source of the cell to load), index (the index of the cell
to load within all cells with source `source`)

Return value(s): None

Side effects: the files within the file system will be available to use if no error happened

See also: [Simple File System and JavaScript Module](./fs.md)

#### `ckb.load_tx_hash`
Description: load the transaction hash

Example:
```js
let buf = ckb.load_tx_hash()
```

Arguments: none

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains the transaction hash)

See also: [`ckb_load_tx_hash` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-transaction-hash)

#### `ckb.load_script_hash`
Description: load the hash of current script

Example:
```js
let buf = ckb.load_script_hash();
```

Arguments: none

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains the script hash)

See also: [`ckb_load_script_hash` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-script-hash)

#### `ckb.load_script`
Description: load current script

Example:

```js
let buf = ckb.load_script();
```

Arguments: none

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains script)

See also: [`ckb_load_script` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-script)

#### `ckb.load_transaction`
Description: load current transaction

Example:
```js
let buf = ckb.load_transaction();
```
Arguments: none

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains current transaction)

See also: [`ckb_load_transaction` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-transaction)

#### `ckb.load_cell`
Description: load cell

Example:
```js
let buf = ckb.load_cell(index, source);
```

Arguments: index (the index of the cell), source (the source of the cell)

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains cell)

See also: [`ckb_load_cell` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-cell)

#### `ckb.load_input`
Description: load input cell

Example:
```js
let buf = ckb.load_input(index, source);
```

Arguments: index (the index of the cell), source (the source of the cell)

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains the input cell)

See also: [`ckb_load_input` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-input)

#### `ckb.load_header`
Description: load cell header

Example:

```js
let buf = ckb.load_header(index, source);
```

Arguments: index (the index of the cell), source (the source of the cell)

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains the header)

See also: [`ckb_load_header` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-header)

#### `ckb.load_witness`
Description: load the witness

Example:
```js
let buf = ckb.load_witness(index, source);
```

Arguments: index (the index of the cell), source (the source of the cell)

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains the witness)

See also: [`ckb_load_witness` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-witness)

#### `ckb.load_cell_data`
Description: load cell data

Example:
```js
let buf = ckb.load_cell_data(index, source);
```

Arguments: index (the index of the cell), source (the source of the cell)

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains the cell data)

See also: [`ckb_load_cell_data` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-cell-data)

#### `ckb.load_cell_by_field`
Description: load cell data field

Example:

```js
let buf = ckb.load_cell_by_field(index, source, field);
```

Arguments: index (the index of the cell), source (the source of the cell), field (the field to load)

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains the cell data field)

See also: [`ckb_load_cell_by_field` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-cell-by-field)

#### `ckb.load_input_by_field`
Description: load input field

Example:
```js
let buf = ckb.load_input_by_field(index, source, field);
```

Arguments: index (the index of the cell), source (the source of the cell), field (the field to load)

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains the input field)

See also: [`ckb_load_input_by_field` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-input-by-field)

#### `ckb.load_header_by_field`
Description: load header by field

Example:
```js
let buffer = ckb.load_header_by_field(index, source, field);
```

Arguments: index (the index of the cell), source (the source of the cell), field (the field to load)

Partial loading supported: yes

Return value(s): buf (An ArrayBuffer that contains the header field)

See also: [`ckb_load_header_by_field` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0009-vm-syscalls/0009-vm-syscalls.md#load-header-by-field)

#### ckb.current_cycles
Description: get current cycles

Example:
```js
let cycles = ckb.current_cycles();
```
Arguments: none

Return value(s): current cycles

See also: [`ckb_current_cycles` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0034-vm-syscalls-2/0034-vm-syscalls-2.md#current-cycles)

#### ckb.vm_version
Description: get current vm version

Example:
```js
let version = ckb.vm_version();
```

Arguments: none

Return value(s): current cycles

See also: [`ckb_vm_version` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0034-vm-syscalls-2/0034-vm-syscalls-2.md#vm-version)

#### ckb.exec_cell
Description: runs an executable file from specified cell data in the context of
an already existing machine, replacing the previous executable. 

Example:
```js
let exit_code = ckb.exec_cell(code_hash, hash_type, offset, length, arg1, arg2, arg3);
```

Arguments: code_hash/hash_type(denote a cell to load JS code), offset(JS code
offset), length(JS code length), arg1,arg2,...(arguments passed to new
executable)

Return values(s): exit code

See also: [`ckb_exec` syscall](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0034-vm-syscalls-2/0034-vm-syscalls-2.md#exec)

#### ckb.spawn_cell
Description: runs an executable file from specified cell data in a new
context, without changing previous executable. 

Example:
```js
let spawn_args = {
    content_length: 1024,
    memory_limit: 8,
    offset: 0,
    length: 0,
};
let value = ckb.spawn_cell(code_hash, hash_type, spawn_args, arg1, arg2, arg3);
let content = value.content;
let exit_code = value.exit_code;
```

Arguments: 
- code_hash/hash_type(denote a cell to load JS code)
- spawn_args, extra spawn arguments
    * content_length, optional, specify content length, default to 0
    * memory_limit, optional, specify memory limit (1~8), default to 8
    * offset, optional, JS code offset, default to 0
    * length, optional, JS code length, default to read all

- arg1,arg2,...(arguments passed to new executable)

Return values(s): exit code and content

See also: [`ckb_spawn` syscall](https://github.com/nervosnetwork/rfcs/pull/418/files)



#### ckb.set_content
Description: set content. It can be fetched by `spawn` syscall.

Example:
```js
let buf = Uint8Array([1,2,3]);
ckb.set_content(buf);
```

Arguments: content(an ArrayBuffer contains content)

Return value(s): none

See also: [`set_content` syscall](https://github.com/nervosnetwork/rfcs/pull/418/files)

#### ckb.get_memory_limit
Description: Get the maximum available memory for the current script.

Example:
```js
let size = ckb.get_memory_limit();
```

Arguments: none

Return value(s): memory size in bytes

See also: [`ckb_get_memory_limit` syscall](https://github.com/nervosnetwork/rfcs/pull/418/files)

#### ckb.current_memory
Description: Get the current memory usage. The result is the sum of the memory
usage of the parent script and the child script.

Example:
```js
let size = ckb.current_memory();
```

Arguments: none

Return value(s): memory size in bytes

See also: [`ckb_current_memory` syscall](https://github.com/nervosnetwork/rfcs/pull/418/files)


## Exported Constants

Most constants here are directly taken from [ckb_consts.h](https://github.com/nervosnetwork/ckb-system-scripts/blob/master/c/ckb_consts.h): 

```
ckb.SOURCE_INPUT
ckb.SOURCE_OUTPUT
ckb.SOURCE_CELL_DEP
ckb.SOURCE_HEADER_DEP
ckb.SOURCE_GROUP_INPUT
ckb.SOURCE_GROUP_OUTPUT

ckb.CELL_FIELD_CAPACITY
ckb.CELL_FIELD_DATA_HASH
ckb.CELL_FIELD_LOCK
ckb.CELL_FIELD_LOCK_HASH
ckb.CELL_FIELD_TYPE
ckb.CELL_FIELD_TYPE_HASH
ckb.CELL_FIELD_OCCUPIED_CAPACITY

ckb.INPUT_FIELD_OUT_POINT
ckb.INPUT_FIELD_SINCE

ckb.HEADER_FIELD_EPOCH_NUMBER
ckb.HEADER_FIELD_EPOCH_START_BLOCK_NUMBER
ckb.HEADER_FIELD_EPOCH_LENGTH

ckb.SCRIPT_HASH_TYPE_DATA
ckb.SCRIPT_HASH_TYPE_TYPE
ckb.SCRIPT_HASH_TYPE_DATA1
ckb.SCRIPT_HASH_TYPE_DATA2
```
