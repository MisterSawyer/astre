# Astre

[Docs](https://mistersawyer.github.io/astre/)

[![Build Astre engine](https://github.com/MisterSawyer/astre/actions/workflows/test-windows-debug.yml/badge.svg?branch=main)](https://github.com/MisterSawyer/astre/actions/workflows/test-windows-debug.yml)

---

## Dependencies

This project requires the following libraries:

- [**Asio**](https://think-async.com/Asio/)
- [**spdlog**](https://github.com/gabime/spdlog)
- [**GLM**](https://github.com/g-truc/glm)
- [**GLEW**](https://glew.sourceforge.net/)
- [**Protobuf**](https://protobuf.dev/)
- [**LUA**](https://github.com/walterschell/Lua)
- [**LUA API Sol2**](https://github.com/ThePhD/sol2)

</br>

---

## Configuring

To configure the project using CMake, run the following command:

```sh
cmake -S . -B build -A x64 -DCMAKE_INSTALL_PREFIX=install -DASTRE_BUILD_TESTS=ON 
```

- `-S .` : Specifies the source directory.
- `-B build` : Specifies the build directory.
- `-A x64` : Sets the target architecture to 64-bit
- `-DASTRE_BUILD_TESTS=ON` : Enables tests.
- `-DCMAKE_INSTALL_PREFIX=install` : Specifies install directory.
- `--graphviz=build/graph.dot` : Specifies the path to the graphviz file.

</br>

---

## Building

To build the project in **Debug Mode**, use:

```sh
cmake --build build --config Debug
```

This will compile the project with debugging enabled.

To build the project in **Release Mode with Debug info enabled**, use:

```sh
cmake --build build --config RelWithDebInfo
```

To build the project in **Release Mode**, use:

```sh
cmake --build build --config Release
```

</br>

---

## Installing

To install project in **Debug Mode**, use:

```sh
cmake --build build --config Debug --target install
```

This will install the project with debugging enabled.

To install project in **Release Mode**, use:

```sh
cmake --build build --config Release --target install
```

</br>

---

## Executables

|name|path|
|---|---|
|Engine Tests|`<INSTALL_PREFIX>/bin/AstreEngineTests.exe`|
|Game|`<INSTALL_PREFIX>/bin/AstreGame.exe`|
|Editor|`<INSTALL_PREFIX>/bin/AstreEditor.exe`|
