(module
  (import "math_lib" "add" (func $add (param i32 i32) (result i32)))
  (func (export "run") (result i32) (call $add (i32.const 1) (i32.const 2)))
)