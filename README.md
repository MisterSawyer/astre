# 🚀 Astre

## 📚 Documentation

---

## 📦 Dependencies

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

## ⚙️ Configuring the project

To configure the project using CMake, run the following command:

```sh
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_INSTALL_PREFIX=install -DASTRE_BUILD_TESTS=ON 
```

- `-S .` : Specifies the source directory.
- `-B build` : Specifies the build directory.
- `-G "Visual Studio 17 2022"` : Use Visual Studio generator
- `-A x64` : Sets the target architecture to 64-bit
- `-DASTRE_BUILD_TESTS=ON` : Enables tests.
- `-DCMAKE_INSTALL_PREFIX=install` : Specifies install directory.

</br>

---

## 🛠️ Building the project

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

## 🛠️ Installing the Project

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

## Example for VSCode tasks

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "CMake: Configure (VS2022 x64)",
            "type": "shell",
            "command": "cmake",
            "args": [
                "-S",
                ".",
                "-B",
                "build",
                "-G",
                "Visual Studio 17 2022",
                "-A",
                "x64",
                "-DCMAKE_INSTALL_PREFIX=install"
            ],
            "group": "build",
            "problemMatcher": [],
            "detail": "Configure the project using CMake and Visual Studio 2022 (x64)"
        },
        {
            "label": "CMake: Build & Install (Debug)",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--build",
                "build",
                "--config",
                "Debug",
                "--target",
                "install",
                "-j",
                "4"
            ],
            "dependsOn": [
                "CMake: Configure (VS2022 x64)"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": [],
            "detail": "Build and install the project using Debug configuration"
        }
    ]
  }
```

---

## 🖥️ Start the game executable

```sh
./install/bin/AstreGame.exe
```

## 🖥️ Start the editor executable

```sh
./install/bin/AstreEditor.exe
```
