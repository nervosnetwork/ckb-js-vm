# Introduction

CKB-VM is a virtual machine based on the RISC-V instruction set that executes on-chain script on CKB, providing
developers with maximum flexibility and power while maintaining security and performance. This approach allows for
seamless integration of new cryptographic primitives without hard forks and supports development in any programming
language that can target RISC-V, enabling developers to use familiar tools and existing libraries rather than building
everything from scratch.

Currently, the main languages used for on-chain script development are
[C](https://github.com/nervosnetwork/ckb-c-stdlib) and [Rust](https://github.com/nervosnetwork/ckb-std). While C is
accessible to learn, it lacks high-level abstractions, making it challenging to write code that is free from undefined
behaviors and security vulnerabilities. Rust offers stronger safety guarantees and modern language features, but its
steep learning curve limits widespread adoption among developers.

With the project [ckb-js-vm](https://github.com/nervosnetwork/ckb-js-vm), we aim to enable on-chain script development
using TypeScript, one of the world's most popular programming languages. This approach significantly lowers the barrier
to entry for blockchain developers, allowing them to leverage the vast JavaScript/TypeScript ecosystem, including its
rich libraries, tools, and community support. By providing a unified language for both development and testing,
ckb-js-vm creates a streamlined, one-stop solution that eliminates context-switching between languages and accelerates
the development cycle for CKB on-chain script programming.

## Early Attempt

In the early years, we [demonstrated](https://xuejie.space/2019_07_13_introduction_to_ckb_script_programming_script_basics/)
the possibility of using JavaScript on CKB-VM through [Duktape](https://duktape.org/), a compact JavaScript engine with
readable code and comprehensive documentation. However, this approach suffered from significant performance limitations,
with on-chain scripts consuming between 100-1000 M cycles, making it impractical for production environments. These
early experiments, while promising conceptually, highlighted the need for a more efficient JavaScript execution solution.

## QuickJS, A Fast JavaScript Engine

[QuickJS](https://bellard.org/quickjs/) is a lightweight JavaScript engine developed by legendary programmer
[Fabrice Bellard](https://en.wikipedia.org/wiki/Fabrice_Bellard). It provides impressive performance without relying
on JIT compilation, making it ideal for CKB-VM which implements [W^X](https://en.wikipedia.org/wiki/W%5EX) security
features that prevent JIT-based optimizations. With QuickJS, we've achieved a significant performance breakthrough,
reducing execution costs to under 50M cycles for typical JavaScript operations—a dramatic improvement over previous
approaches.

CKB imposes a 500KB size limitation for binaries deployed to a single cell, making QuickJS's focus on code size
efficiency particularly valuable for on-chain scripts. The code size for essential features are quite small:
- The complete regular expression library requires only ~15 KiB (x86 code), excluding Unicode support
- The full Unicode library adds approximately 45 KiB (x86 code)
- BigInt, BigFloat, and BigDecimal functionality (via the libbf library) requires about 90 KiB (x86 code)

## Total Solution
With a JavaScript engine, it is still very difficult to develop a full featured on-chain script. We also provide
following tools and libraries:
- libraries in TypeScript
- building tools
- framework for unit tests
We will introduce these features later.

## Comprehensive Development Ecosystem

While integrating a JavaScript engine is a significant advancement, developing production-ready on-chain scripts
requires a complete ecosystem of tools and libraries. The ckb-js-vm project delivers a comprehensive solution that
includes:

- A rich set of TypeScript libraries providing CKB-specific abstractions
- Streamlined build tooling that optimizes code size and performance
- A robust testing framework enabling comprehensive unit testing
- Scaffold tools that generate project templates and boilerplate code for rapid development

This end-to-end approach ensures developers can focus on writing business logic rather than wrestling with
infrastructure concerns. The subsequent sections of this documentation will explore each component of this ecosystem
in detail, demonstrating how they work together to create a seamless development experience.

