## Performance

I got these results (after following [#how-to-run](#how-to-run))
on my machine:
``` bash
$ echo "0" | time Lama/_build/default/src/Driver.exe -runtime Lama/runtime -i Lama/performance/Sort.lama
 > Lama/_build/default/src/Driver.exe -runtime Lama/runtime -i   342,74s user 1,73s system 99% cpu 5:44,57 total
```

```bash
$ (cd Lama/performance && ../_build/default/src/Driver.exe -runtime ../runtime -b Sort.lama) 
&& time build/lama-interpreter Lama/performance/Sort.bc
build/lama-interpreter Lama/performance/Sort.bc  114,02s user 3,75s system 99% cpu 1:57,79 total
```

| `lamac -i` | My interpreter |
| --- | --- |
| ~342 seconds | ~114 seconds |

If I am not mistaken,
`make -C Lama all` builds
`lamac` with `--profile=release`,
so both programs are compared in Release
configurations.

## Implementation details

The implementation details regarding callstack
are described through comments in `frame.hpp`.

## How to Run

1. First, pull Lama submodule to
    be able to build the runtime:
    ```bash
    git submodule update --init
    ```

1. Follow [Lama](https://github.com/PLTools/Lama/)'s
    installation instructions up to
    (inclusively) the point of creating `lama`
    switch for `opam`. Set the switch:

    ```bash
    eval $(opam env --switch=lama --set-switch)
    ```

1. Build Lama:
    ```bash
    make -C Lama all
    ```
    The only thing that is needed from Lama is its runtime.
    `all` is a surefire way of 
    building it. Possibly, there exists
    a finer rule for building the runtime.

1. Generate a `Makefile` to build the interpreter
    using CMake:
    ```bash
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/g++-11 -B build -G "Unix Makefiles"
    ```
    You may change the `CMAKE_CXX_COMPILER` variable
    to specify your desired compiler.

1. Finally, build the interpreter:
    ```bash
    make -C build
    ```
    The interpreter's executable will be located
    at `build/lama-interpreter`
    (assuming you specified
    `build` as the build directory when executing
    CMake).
