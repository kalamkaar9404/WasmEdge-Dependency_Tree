#ifndef WASMEDGE_MOCKS_H
#define WASMEDGE_MOCKS_H
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace WasmEdge {
enum class ErrCode {
    Success = 0,
    ModuleInUse,
    WrongInstanceAddress, 
    UnknownError
};

template <typename T>
class Expect {
    std::variant<T, ErrCode> Data;
public:
    Expect() : Data(T()) {} 
    Expect(T Val) : Data(Val) {}
    Expect(ErrCode Err) : Data(Err) {}

    bool has_value() const { return std::holds_alternative<T>(Data); }
    T value() const { return std::get<T>(Data); }
    ErrCode error() const { return std::get<ErrCode>(Data); }
    operator bool() const { return has_value(); }
};

template <>
class Expect<void> {
    ErrCode Code;
public:
    Expect() : Code(ErrCode::Success) {}
    Expect(ErrCode C) : Code(C) {}
    bool has_value() const { return Code == ErrCode::Success; }
    ErrCode error() const { return Code; }
    operator bool() const { return has_value(); }
};

inline Expect<void> Unexpect(ErrCode Code) { return Expect<void>(Code); }

}
#endif