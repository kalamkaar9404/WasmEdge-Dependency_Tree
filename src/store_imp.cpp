#include "wasmedge_m.h"
#include <mutex>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <string_view>
#include <shared_mutex>
#include <iostream>
#include <algorithm>

using namespace std;
using namespace WasmEdge;

namespace Runtime {
class StoreManager {
public:
    Expect<void> registerModule(const string& Name, const vector<string>& Imports) {
        unique_lock<shared_mutex> Lock(GraphMutex);
        
        if (Modules.find(Name)!= Modules.end()) {
            cerr << "Module " << Name << " already exists.\n";
            return Unexpect(ErrCode::UnknownError);
        }
        Modules.insert(Name);
        cout << " Registered instance: " << Name << "\n";

        // Link Imports (Populate Graph)
        for (const auto& Provider : Imports) {
            if (Modules.find(Provider)!= Modules.end()) {
                linkModules_Internal(Name, Provider);
            } else {
                cout << " " << Name << " imports " << Provider << " but it is missing.\n";
            }
        }
        return {};
    }

    Expect<void> removeModule(string_view Name, bool Cascade = false) {
        return Cascade? removeModuleCascading(Name) : removeModuleStrict(Name);
    }

    void printGraph() {
        shared_lock<shared_mutex> Lock(GraphMutex);
        cout << "\nDependency Graph State\n";
        cout << "Active Modules: ";
        for(const auto& m : Modules) cout << m << " ";
        cout << "\n";
        
        cout << "Reference Map\n";
        for (const auto& [Provider, Consumers] : ModRefMap) {
            cout << "  " << Provider << " <--- { ";
            for (const auto& C : Consumers) cout << C << " ";
            cout << "}\n";
        }
        cout << "\n\n";
    }

private:
    set<string> Modules; 
    map<string, set<string>> ModDepMap; // Forward: Consumer -> Providers
    map<string, set<string>> ModRefMap; // Reverse: Provider -> Consumers
    mutable shared_mutex GraphMutex;

    void linkModules_Internal(const string& Consumer, const string& Provider) {
        ModDepMap[Consumer].insert(Provider);
        ModRefMap[Provider].insert(Consumer);
        cout << "Linked: " << Consumer << " depends on " << Provider << "\n";
    }

    void unlinkModules(const string& Consumer) {
        unique_lock<shared_mutex> Lock(GraphMutex);
        
        // 1. Stop referencing providers
        if (auto It = ModDepMap.find(Consumer); It!= ModDepMap.end()) {
            for (const auto& Provider : It->second) {
                ModRefMap[Provider].erase(Consumer);
            }
            ModDepMap.erase(It);
        }
        
        // 2. Clean up self
        ModRefMap.erase(Consumer); 
    }

    Expect<void> removeModuleStrict(string_view Name) {
        string ModName(Name);
        
        {
            shared_lock<shared_mutex> Lock(GraphMutex);
            auto It = ModRefMap.find(ModName);
            if (It!= ModRefMap.end() &&!It->second.empty()) {
                cerr << " Cannot delete " << ModName << ": In use by others.\n";
                return Unexpect(ErrCode::ModuleInUse);
            }
        }

        unlinkModules(ModName);
        
        {
            unique_lock<shared_mutex> Lock(GraphMutex);
            if (Modules.erase(ModName) == 0) {
                return Unexpect(ErrCode::WrongInstanceAddress);
            }
        }
        
        cout << " Successfully deleted: " << ModName << "\n";
        return {};
    }

    void collectCascadingCandidates(const string& Target, vector<string>& Order, set<string>& Visited) {
        if (Visited.count(Target)) return;
        Visited.insert(Target);

        set<string> Dependents;
        {
            shared_lock<shared_mutex> Lock(GraphMutex);
            if (auto It = ModRefMap.find(Target); It!= ModRefMap.end()) {
                Dependents = It->second;
            }
        }

        for (const auto& Dep : Dependents) {
            collectCascadingCandidates(Dep, Order, Visited);
        }

        Order.push_back(Target);
    }

    Expect<void> removeModuleCascading(string_view Name) {
        vector<string> DeleteOrder;
        set<string> Visited;
        string RootName(Name);

        collectCascadingCandidates(RootName, DeleteOrder, Visited);

        for (const auto& Target : DeleteOrder) {
            cout << "Auto-removing dependent: " << Target << "\n";
            auto Res = removeModuleStrict(Target);
            if (!Res) return Unexpect(Res.error());
        }
        return {};
    }
};

}