# LFX Mentorship 2026 (Term 1): Module Instance Dependency Tree in WASM Store

![LFX Mentorship](https://img.shields.io/badge/LFX-Mentorship%202026-blue)
![Issue](https://img.shields.io/badge/Issue-%234514-green)
![Status](https://img.shields.io/badge/Status-In%20Progress-orange)

##  Project Overview

**Project:** Module instance dependency tree in WASM store  
**Organization:** WasmEdge Runtime  
**Issue:** [#4514](https://github.com/WasmEdge/WasmEdge/issues/4514)  

### The Problem
In the WasmEdge runtime, module instances are stored and managed dynamically. A critical issue arises when a module instance (let's call it `Module B`) is deleted while another module (`Module A`) still imports and relies on it.

Currently, the store allows `Module B` to be deleted without checking for active dependents. This leads to **dangling pointers** and **segmentation faults (crashes)** when `Module A` subsequently tries to access the deleted memory.

### The Solution
This project implements a **Module Instance Dependency Tree** within the `Store` class. This system tracks relationships between modules (Who imports whom?) and enforces memory safety during deletion.

The solution introduces two modes of deletion:
1.  **Strict Mode:** Prevents deletion if *any* other module depends on the target.
2.  **Cascading Mode:** Automatically identifies and removes the entire tree of dependent modules in a safe, bottom-up order.

---

##  Technical Implementation

The implementation works by maintaining a Directed Acyclic Graph (DAG) of module dependencies. The lifecycle is managed in five phases:

### Phase 1: Dependency Graph Construction (Linking)
When a new module is registered via `registerModule`, the system inspects its imports.
- **ModDepMap (Forward Edge):** Records what the new module needs.
- **ModRefMap (Reverse Edge):** Records that existing modules are now "in use" by the new module.

### Phase 2: Deletion Routing
The `removeModule` API acts as a router based on user intent:
- If `cascade = false`: Routes to **Strict Mode**.
- If `cascade = true`: Routes to **Cascading Mode**.

### Phase 3: Safe Order Calculation (Cascading Only)
If cascading is selected, a Depth-First Search (DFS) is performed to collect all dependents. A **Post-Order Traversal** ensures "children" (dependent modules) are scheduled for deletion *before* their "parents" (dependencies).

### Phase 4: Safety Verification (The Guard)
Before deletion, `removeModuleStrict` checks the `ModRefMap`.
- If the module has incoming edges (active dependents), it returns `ErrCode::ModuleInUse`.

### Phase 5: Unlink and Memory Reclamation
Finally, `unlinkModules` removes the edges from the graph, and `Modules.erase` frees the memory safely.

---

## Building and Testing

### Prerequisites
- C++ Compiler (GCC/Clang) supporting C++17 or later
- CMake 3.12+
- WasmEdge dependencies (LLVM, etc.)

### Build Instructions
```bash
git clone [https://github.com/](https://github.com/)[YOUR_USERNAME]/WasmEdge.git
cd WasmEdge
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
