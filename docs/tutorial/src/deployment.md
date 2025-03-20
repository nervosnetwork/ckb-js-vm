# Reproducible Build and Deployment

When deploying an on-chain script, it's essential to build it from scratch. A key requirement during this process is
ensuring that different builds of the same source code produce identical binaries - this is known as a "reproducible
build."

You can achieve this with ckb-js-vm using the following command:

  ```bash
  bash reproducible_build.sh
  ```

## Deployment

The script has been deployed on the testnet with these parameters:

| Parameter   | Value                                                                |
| ----------- | -------------------------------------------------------------------- |
| `code_hash` | `0x3e9b6bead927bef62fcb56f0c79f4fbd1b739f32dd222beac10d346f2918bed7` |
| `hash_type` | `type`                                                               |
| `tx_hash`   | `0xf594e7deb3bdec611b20bdf7814acf7779ddf061a8f207b1f3261242b8dc4494` |
| `index`     | `0x0`                                                                |
| `dep_type`  | `code`                                                               |

The corresponding SHA256 checksum in [checksums.txt](https://github.com/nervosnetwork/ckb-js-vm/blob/main/checksums.txt)
is: `898260099d49ef84a1fb10f4aae7dae8ef5ec5b34d2edae29a78981fd084f1ed`
