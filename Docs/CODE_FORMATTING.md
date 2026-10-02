# Source Code Formatting

The v1.0.0 source tree uses one consistent C formatting policy.

## Rules

- Indentation: 4 spaces.
- Tabs: not used in C/H files.
- Brace style: Allman.
- Maximum target line length: 100 characters.
- One declaration or logical statement per readable line where practical.
- One blank line between logical sections.
- Public structures list one member per line.
- Function parameters may wrap vertically when a call becomes difficult to read.
- Files use LF line endings and always end with a newline.

## Automatic formatting

The repository includes:

- `.clang-format`
- `.editorconfig`

If `clang-format` is installed:

```bash
find App Drivers Middleware Docs -type f \( -name "*.c" -o -name "*.h" \) \
    -exec clang-format -i {} +
```

In STM32CubeIDE/Eclipse, keep indentation set to spaces with width 4.
