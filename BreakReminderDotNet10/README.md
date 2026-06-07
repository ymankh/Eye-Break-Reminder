# Repository layout

- `dotnet/` — WinUI 3 break reminder app, single-file publish config, and WiX installer
- `c/` — C starter project with `src/`, `include/`, `tests/`, and `CMakeLists.txt`

## Working with the .NET project

```powershell
cd dotnet
dotnet build
```

Project instructions live in `dotnet/README.md`.

## Working with the C project

```powershell
cd c
cmake -S . -B build
cmake --build build
```
