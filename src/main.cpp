#include <iostream>
#include <vector>
#include <string>
#include <wasmedge/wasmedge.h>
#include "wasmedge_m.h"
#include "store_imp.cpp"

using namespace std;
using namespace WasmEdge;
using namespace Runtime;

vector<string> GetModuleDependencies(const char* Path) {
    WasmEdge_LoaderContext *LoaderCxt = WasmEdge_LoaderCreate(NULL);
    WasmEdge_ASTModuleContext *ASTCxt = NULL;
    WasmEdge_Result Res = WasmEdge_LoaderParseFromFile(LoaderCxt, &ASTCxt, Path);
    
    vector<string> Deps;
    if (WasmEdge_ResultOK(Res)) {
        uint32_t ImpCount = WasmEdge_ASTModuleListImportsLength(ASTCxt);
        const WasmEdge_ImportTypeContext **ImpList = new const WasmEdge_ImportTypeContext*[ImpCount];
        WasmEdge_ASTModuleListImports(ASTCxt, ImpList, ImpCount);
        
        for (uint32_t i = 0; i < ImpCount; ++i) {
            WasmEdge_String ModName = WasmEdge_ImportTypeGetModuleName(ImpList[i]);
            string Name(ModName.Buf, ModName.Length);
            // Add unique dependencies only
            if (find(Deps.begin(), Deps.end(), Name) == Deps.end()) {
                Deps.push_back(Name);
            }
        }
        delete ImpList;
    }
    WasmEdge_ASTModuleDelete(ASTCxt);
    WasmEdge_LoaderDelete(LoaderCxt);
    return Deps;
}

void RunDirectDeletionExample() {
    cout << "Direct Deletion\n";
    StoreManager Manager;
    
    // Registering a provider and a consumer extracted from real WASM files [2]
    Manager.registerModule("math_lib", {});
    auto ConsumerDeps = GetModuleDependencies("modules/consumer.wasm");
    Manager.registerModule("app_module", ConsumerDeps);

    Manager.printGraph();
    auto Res = Manager.removeModule("math_lib", false);
    if (!Res && Res.error() == ErrCode::ModuleInUse) {
        cout << " Deletion blocked: Provider still in use.\n";
    }
}

void RunCascadedDeletionExample() {
    cout << "\nCascaded Deletion\n";
    StoreManager Manager;
    Manager.registerModule("base_plugin", {});
    Manager.registerModule("extension_a", {"base_plugin"});
    Manager.registerModule("extension_b", {"extension_a"});

    Manager.printGraph();
    if (Manager.removeModule("base_plugin", true)) {
        cout << " Cleanup successful: All dependents removed.\n";
    }
}

int main() {
    RunDirectDeletionExample();
    RunCascadedDeletionExample();
    return 0;
}