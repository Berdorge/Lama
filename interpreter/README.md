## Performance

I got these results (after following [#how-to-run](#how-to-run))
on my machine:
```bash
$ echo "0" | time _build/default/src/Driver.exe -runtime runtime -i performance/Sort.lama
 > _build/default/src/Driver.exe -runtime runtime -i performance/Sort.lama  342,74s user 1,73s system 99% cpu 5:44,57 total
```

```bash
$ echo "0" | time _build/default/src/Driver.exe -runtime runtime -s performance/Sort.lama
 > _build/default/src/Driver.exe -runtime runtime -s performance/Sort.lama  111,20s user 1,83s system 99% cpu 1:53,05 total
```

```bash
$ (cd performance && ../_build/default/src/Driver.exe -runtime ../runtime -b Sort.lama) && time interpreter/build/lama-interpreter performance/Sort.bc
Verifier took 5 us
interpreter/build/lama-interpreter performance/Sort.bc  103,91s user 3,93s system 99% cpu 1:47,85 total
```

Previous results (before implementing the verifier):
```bash
$ (cd performance && ../_build/default/src/Driver.exe -runtime ../runtime -b Sort.lama) && time interpreter/build/lama-interpreter performance/Sort.bc
interpreter/build/lama-interpreter performance/Sort.bc  114,02s user 3,75s system 99% cpu 1:57,79 total
```

| `lamac -i` | `lamac -s` | My interpreter | My interpreter + verifier |
| --- | --- | --- | --- |
| ~344 seconds | ~113 seconds | ~117 seconds | ~108 seconds |

If I am not mistaken,
`make -C Lama all` builds
`lamac` with `--profile=release`,
so both programs are compared in Release
configurations.

## How to Run

1. Follow [Lama](https://github.com/PLTools/Lama/)'s
    installation instructions up to
    (inclusively) the point of creating `lama`
    switch for `opam`. Set the switch:

    ```bash
    eval $(opam env --switch=lama --set-switch)
    ```

1. Build Lama:
    ```bash
    make all
    ```
    The only thing that is needed from Lama is its runtime.
    `all` is a surefire way of 
    building it. Possibly, there exists
    a finer rule for building the runtime.

1. Generate a `Makefile` to build the interpreter
    using CMake:
    ```bash
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/g++-11 -S interpreter -B interpreter/build -G "Unix Makefiles"
    ```
    You may change the `CMAKE_CXX_COMPILER` variable
    to specify your desired compiler.

1. Finally, build the interpreter:
    ```bash
    make -C interpreter/build
    ```
    The interpreter's executable will be located
    at `interpreter/build/lama-interpreter`
    (assuming you specified
    `interpreter/build` as the build directory when executing
    CMake).
