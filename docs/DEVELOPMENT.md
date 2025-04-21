# Development Guide

## Development Environment Setup

### Prerequisites

1. **Operating System**
   - Linux (Ubuntu 20.04+ recommended)
   - macOS (10.15+)
   - Windows (10+ with WSL2)

2. **Required Tools**
   - C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
   - CMake 3.10+
   - Git
   - OpenMP
   - nlohmann/json
   - Google Test (for testing)

3. **Optional Tools**
   - Doxygen (for documentation)
   - clang-format (for code formatting)
   - cppcheck (for static analysis)
   - valgrind (for memory checking)

### Installation Steps

1. **Linux (Ubuntu)**
```bash
# Install build tools
sudo apt update
sudo apt install -y build-essential cmake git

# Install dependencies
sudo apt install -y libomp-dev nlohmann-json3-dev

# Install optional tools
sudo apt install -y doxygen clang-format cppcheck valgrind
```

2. **macOS**
```bash
# Install Homebrew if not installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build tools
brew install cmake git

# Install dependencies
brew install libomp nlohmann-json

# Install optional tools
brew install doxygen clang-format cppcheck valgrind
```

3. **Windows (WSL2)**
```bash
# Follow Ubuntu instructions above
# Or use Visual Studio with C++ workload
```

### Project Setup

1. **Clone Repository**
```bash
git clone https://github.com/doomhammerhell/tradingbot.git
cd tradingbot
```

2. **Create Build Directory**
```bash
mkdir build
cd build
```

3. **Configure Project**
```bash
cmake -DENABLE_TESTING=ON -DENABLE_DOCS=ON ..
```

4. **Build Project**
```bash
make -j$(nproc)
```

## Coding Standards

### General Guidelines

1. **Naming Conventions**
   - Classes: PascalCase
   - Functions: camelCase
   - Variables: snake_case
   - Constants: UPPER_SNAKE_CASE
   - Private members: snake_case_ with trailing underscore

2. **File Organization**
   - Header files: .hpp
   - Source files: .cpp
   - Test files: _test.cpp
   - One class per file
   - Related classes in same directory

3. **Comments**
   - Use Doxygen style
   - Document public interfaces
   - Explain complex logic
   - Keep comments up to date

### Code Style

1. **Formatting**
```cpp
// Class definition
class MyClass {
public:
    // Public members first
    MyClass();
    ~MyClass();

    void publicMethod();

private:
    // Private members last
    void privateMethod();
    int private_member_;
};

// Function definition
ReturnType ClassName::methodName(
    ParameterType1 param1,
    ParameterType2 param2
) {
    // Implementation
}
```

2. **Indentation**
   - Use 4 spaces
   - No tabs
   - Align braces
   - Align parameters

3. **Line Length**
   - Maximum 100 characters
   - Break long lines
   - Align continuation

### Best Practices

1. **Memory Management**
   - Use smart pointers
   - Avoid raw pointers
   - Check for nullptr
   - Handle exceptions

2. **Error Handling**
   - Use exceptions
   - Provide error messages
   - Clean up resources
   - Log errors

3. **Performance**
   - Avoid copies
   - Use move semantics
   - Pre-allocate memory
   - Profile code

## Testing

### Unit Tests

1. **Test Structure**
```cpp
#include <gtest/gtest.h>
#include "MyClass.hpp"

class MyClassTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }

    void TearDown() override {
        // Cleanup code
    }

    MyClass instance_;
};

TEST_F(MyClassTest, TestMethod) {
    // Test code
    EXPECT_EQ(expected, actual);
}
```

2. **Test Categories**
   - Functionality tests
   - Edge cases
   - Error handling
   - Performance tests

3. **Running Tests**
```bash
# Run all tests
ctest

# Run specific test
./tests/my_test --gtest_filter=MyClassTest.TestMethod

# Run with coverage
ctest -T coverage
```

### Integration Tests

1. **Test Structure**
```cpp
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup system
    }

    void TearDown() override {
        // Cleanup system
    }
};

TEST_F(IntegrationTest, SystemTest) {
    // Test system integration
}
```

2. **Test Categories**
   - Component integration
   - System behavior
   - Data flow
   - Error propagation

## Documentation

### Code Documentation

1. **Class Documentation**
```cpp
/**
 * @brief Brief description of the class
 * 
 * Detailed description of the class and its purpose.
 * 
 * @tparam T Template parameter description
 */
template<typename T>
class MyClass {
    // ...
};
```

2. **Function Documentation**
```cpp
/**
 * @brief Brief description of the function
 * 
 * Detailed description of the function and its behavior.
 * 
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return Description of return value
 * @throw ExceptionType Description of when this exception is thrown
 */
ReturnType functionName(ParameterType1 param1, ParameterType2 param2);
```

3. **Generate Documentation**
```bash
# Generate Doxygen documentation
doxygen Doxyfile

# View documentation
open docs/html/index.html
```

### API Documentation

1. **Documentation Structure**
   - Overview
   - Installation
   - Usage examples
   - API reference
   - Troubleshooting

2. **Example Documentation**
```markdown
# API Reference

## Class MyClass

### Description
Brief description of the class.

### Constructor
```cpp
MyClass(ParameterType param);
```

### Methods
- `methodName()`: Description
- `anotherMethod()`: Description

### Example
```cpp
MyClass instance(param);
instance.methodName();
```
```

## Version Control

### Git Workflow

1. **Branching Strategy**
   - main: Production code
   - develop: Development branch
   - feature/*: New features
   - bugfix/*: Bug fixes
   - release/*: Release preparation

2. **Commit Messages**
```
type(scope): description

Detailed description of changes.

- Bullet point 1
- Bullet point 2
```

3. **Pull Requests**
   - Create from feature branch
   - Include description
   - Add reviewers
   - Pass CI checks

### Code Review

1. **Review Checklist**
   - Code style
   - Functionality
   - Performance
   - Documentation
   - Tests

2. **Review Process**
   - Self-review first
   - Request review
   - Address comments
   - Merge when approved

## Release Process

### Versioning

1. **Semantic Versioning**
   - MAJOR.MINOR.PATCH
   - MAJOR: Breaking changes
   - MINOR: New features
   - PATCH: Bug fixes

2. **Version Tags**
```bash
# Create tag
git tag -a v1.0.0 -m "Release 1.0.0"
git push origin v1.0.0
```

### Release Steps

1. **Preparation**
   - Update version
   - Update changelog
   - Run tests
   - Check documentation

2. **Release**
   - Create release branch
   - Build packages
   - Run integration tests
   - Create release notes

3. **Deployment**
   - Deploy to production
   - Monitor performance
   - Handle issues
   - Update documentation

## Troubleshooting

### Common Issues

1. **Build Issues**
   - Check dependencies
   - Verify compiler version
   - Check CMake version
   - Review build logs

2. **Runtime Issues**
   - Check logs
   - Verify configuration
   - Test with debug build
   - Use valgrind

3. **Performance Issues**
   - Profile code
   - Check memory usage
   - Review algorithms
   - Optimize bottlenecks

### Debugging

1. **Debug Build**
```bash
# Configure debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build with debug symbols
make -j$(nproc)
```

2. **GDB Usage**
```bash
# Start debugger
gdb ./tradingbot

# Set breakpoint
break MyClass::methodName

# Run program
run

# Step through code
next
step

# Print variables
print variable_name

# Backtrace
bt
```

3. **Valgrind Usage**
```bash
# Check memory leaks
valgrind --leak-check=full ./tradingbot

# Check thread issues
valgrind --tool=helgrind ./tradingbot
``` 