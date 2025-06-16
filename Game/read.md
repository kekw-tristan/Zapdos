# Zapdos

**Zapdos** is a DirectX12 engine designed for high-performance graphics and game development.

## How to Run

1. **Open terminal in the project folder**
2. Run the following command to generate Visual Studio 2022 project files:
   ```bash
   premake5 vs2022
   ```
3. Open the generated `Zapdos.sln` in Visual Studio 2022.
4. Build the solution and run the project.

## Naming Conventions

To ensure code consistency, Zapdos follows these naming conventions:

| Pattern        | Meaning                                 |
|----------------|-----------------------------------------|
| `c_variable`   | `constexpr` variable                    |
| `m_variable`   | Member variable in a class (not struct) |
| `s_variable`   | `static` variable                       |
| `cClassName`   | Class name                              |
| `sStructName`  | Struct name                             |
| `pVariable`    | Pointer variable                        |