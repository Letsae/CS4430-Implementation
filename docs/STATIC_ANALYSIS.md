# Static Analysis Report

## clang-tidy

### Command
```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
run-clang-tidy -p build src/
```

### Findings
*Run static analysis to populate this section*

## cppcheck

### Command
```bash
cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem src/
```

### Findings
*Run static analysis to populate this section*

## Summary
*Static analysis to be performed before final release*
