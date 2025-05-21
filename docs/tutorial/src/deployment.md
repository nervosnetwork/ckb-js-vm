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
| `tx_hash`   | `0x9f6558e91efa7580bfe97830d11cd94ca5d614bbf4a10b36f3a5b9d092749353` |
| `index`     | `0x0`                                                                |
| `dep_type`  | `code`                                                               |

The corresponding SHA256 checksum in [checksums.txt](https://github.com/nervosnetwork/ckb-js-vm/blob/main/checksums.txt)
is: `32d1db56b9d6f3188c1defe94fbfaa16159b46652a2c9e45e9eb167f0e083cd2`
