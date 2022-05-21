<!doctype html public "-//w3c//dtd html 4.0 transitional//en">

# QuickJS Javascript Engine

## News

*   2021-03-27:

    *   New release ([Changelog](Changelog))

*   2020-11-08:

    *   New release ([Changelog](Changelog))

*   2020-09-06:

    *   New release ([Changelog](Changelog))
    *   Official GitHub mirror at [https://github.com/bellard/quickjs](https://github.com/bellard/quickjs)

## Introduction

QuickJS is a small and embeddable Javascript engine. It supports the
[ES2020](https://tc39.github.io/ecma262/) specification
including modules, asynchronous generators, proxies and BigInt.

It optionally supports mathematical extensions such as big decimal
floating point numbers (BigDecimal), big binary floating point numbers
(BigFloat) and operator overloading.

Main Features:

*   Small and easily embeddable: just a few C files, no external
dependency, 210 KiB of x86 code for a simple <tt>hello world</tt>
program.
*   Fast interpreter with very low startup time: runs the 75000 tests
of the [ECMAScript Test
Suite](https://github.com/tc39/test262) in about 100 seconds on a single core of a desktop PC. The
complete life cycle of a runtime instance completes in less than 300
microseconds.
*   Almost
  complete [ES2020](https://tc39.github.io/ecma262/)
  support including modules, asynchronous generators and full Annex B
  support (legacy web compatibility).
*   Passes nearly 100% of the ECMAScript Test Suite tests when selecting the ES2020 features. A summary is available at [Test262 Report](https://test262.report/).
*   Can compile Javascript sources to executables with no external dependency.
*   Garbage collection using reference counting (to reduce memory usage
  and have deterministic behavior) with cycle removal.
*   Mathematical extensions: BigDecimal, BigFloat, operator overloading, bigint mode, math mode.
*   Command line interpreter with contextual colorization implemented in Javascript.
*   Small built-in standard library with C library wrappers.

## [Benchmark](bench.html)

## Online Demo

An online demonstration of the QuickJS engine with its mathematical
extensions is available
at [numcalc.com](http://numcalc.com). It was compiled from
C to WASM/asm.js with Emscripten.

<tt>qjs</tt> and <tt>qjscalc</tt> can be run in [JSLinux](https://bellard.org/jslinux/vm.html?url=alpine-x86.cfg).

## Documentation

QuickJS documentation: [HTML version](quickjs.html),
[PDF version](quickjs.pdf).

Specification of the JS Bignum Extensions: [HTML
version](jsbignum.html), [PDF version](jsbignum.pdf).

## Download

*   QuickJS source code: [quickjs-2021-03-27.tar.xz](quickjs-2021-03-27.tar.xz)
*   QuickJS extras (contain the unicode files needed to rebuild the unicode tables and the bench-v8 benchmark): [quickjs-extras-2021-03-27.tar.xz](quickjs-extras-2021-03-27.tar.xz)

*   Official [GitHub mirror](https://github.com/bellard/quickjs).

*   Binary releases are available in [jsvu](https://github.com/GoogleChromeLabs/jsvu), [esvu](https://github.com/devsnek/esvu) and [here](binary_releases).

*   Typescript compiler compiled with QuickJS: [quickjs-typescript-4.0.0-linux-x86.tar.xz](quickjs-typescript-4.0.0-linux-x86.tar.xz)

*   Babel compiler compiled with QuickJS: [quickjs-babel-linux-x86.tar.xz](quickjs-babel-linux-x86.tar.xz)

## Sub-projects

QuickJS embeds the following C libraries which can be used in other
projects:

*   **libregexp**: small and fast regexp library fully compliant with the Javascript ES 2019 specification.
*   **libunicode**: small unicode library supporting case
conversion, unicode normalization, unicode script queries, unicode
general category queries and all unicode binary properties.
*   **libbf**: small library implementing arbitrary precision
    IEEE 754 floating point operations and transcendental functions with
    exact rounding. It is maintained as a [separate project](/libbf).

## Links

*   [QuickJS Development](https://www.freelists.org/list/quickjs-devel) mailing list
*   [Small Javascript programs](pi.html) to compute
    one billion digits of pi.

## Licensing

QuickJS is released under
the [MIT license](https://opensource.org/licenses/MIT).

Unless otherwise specified, the QuickJS sources are copyright Fabrice
Bellard and Charlie Gordon.

* * *

Fabrice Bellard - [https://bellard.org/](https://bellard.org)