## Adding a New Component in ESP-IDF

To add a custom component to your ESP-IDF project, follow these steps:

### 1. Component Directory Structure

Organize your component as follows:

```

components/
└── your_component/
├── CMakeLists.txt
├── include/
│   └── your_header.hpp
└── src/
└── your_source.cpp

````

- Place all public headers in the `include/` directory.
- Place source files in the `src/` directory.

### 2. Component CMakeLists.txt

Register the component and specify source files and include directories:

```cmake
idf_component_register(
    SRCS "src/your_source.cpp"
    INCLUDE_DIRS "include"
)
````

### 3. Main Application CMakeLists.txt

In your `main/CMakeLists.txt`, link the component by adding it as a requirement:

```cmake
idf_component_register(
    SRCS "main.cpp"
    REQUIRES your_component
)
```

This ensures that the compiler can find the component's headers and link against its object files.

### 4. Including Component Headers

In your source files (e.g., `main.cpp`), include component headers with quotes and without relative paths:

```cpp
#include "your_header.hpp"
```

---

By following these steps, your component will be correctly built and linked with the main application.
