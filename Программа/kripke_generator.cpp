#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include "include/json.hpp"

using json = nlohmann::json;

struct ModelKripke {
    std::vector<std::string> states;
    std::vector<std::string> initial_states;
    std::vector<std::pair<std::string, std::string>> transitions;
    std::unordered_map<std::string, std::vector<std::string>> state_predicates;
    std::vector<std::string> specifications; // <-- НОВОЕ ПОЛЕ
};

ModelKripke parseModel(const std::string& filename) {
    ModelKripke model;
    std::ifstream file(filename);
    json data = json::parse(file);

    for (const auto& state : data["states"]) {
        model.states.push_back(state);
    }

    for (const auto& init : data["initial_states"]) {
        model.initial_states.push_back(init);
    }

    for (const auto& trans : data["transitions"]) {
        model.transitions.emplace_back(trans[0], trans[1]);
    }

    for (const auto& sp : data["state_predicates"]) {
        std::string state = sp["state"];
        std::vector<std::string> predicates;
        for (const auto& pred : sp["predicates"]) {
            if (!pred.empty()) {
                predicates.push_back(pred);
            }
        }
        model.state_predicates[state] = predicates;
    }

    // НОВОЕ: читаем спецификации из JSON (если поле есть)
    if (data.contains("specifications")) {
        for (const auto& spec : data["specifications"]) {
            model.specifications.push_back(spec);
        }
    }

    return model;
}

std::string generateSMV(const ModelKripke& model) {
    std::string smv;

    smv += "MODULE main\n";
    smv += "VAR\n";
    smv += "\tstate_ : {";

    for (size_t i = 0; i < model.states.size(); ++i) {
        smv += model.states[i];
        if (i != model.states.size() - 1) smv += ", ";
    }
    smv += "};\n";

    // Собираем все уникальные предикаты
    std::set<std::string> all_predicates;
    for (const auto& [state, preds] : model.state_predicates) {
        for (const auto& p : preds) {
            if (!p.empty()) {
                all_predicates.insert(p);
            }
        }
    }

    for (const auto& p : all_predicates) {
        smv += "\t" + p + " : boolean;\n";
    }

    smv += "ASSIGN\n";
    smv += "\tinit(state_) := " + model.initial_states[0] + ";\n";
    smv += "\tnext(state_) := case\n";

    std::unordered_map<std::string, std::set<std::string>> transitions_map;
    for (const auto& [from, to] : model.transitions) {
        transitions_map[from].insert(to);
    }

    for (const auto& state : model.states) {
        if (transitions_map.find(state) != transitions_map.end()) {
            smv += "\t\t(state_ = " + state + ") : {";
            size_t count = 0;
            for (const auto& target : transitions_map[state]) {
                smv += target;
                if (++count != transitions_map[state].size()) smv += ", ";
            }
            smv += "};\n";
        }
    }

    smv += "\t\tTRUE: state_;\n";
    smv += "\tesac;\n";
    smv += "------------------------------------------------------------\n";

    // DEFINE: для каждого предиката перечисляем состояния, где он истинен
    for (const auto& predicate : all_predicates) {
        smv += "\t" + predicate + " := case\n\t\t\t";
        bool first_condition = true;
        for (const auto& [state, preds] : model.state_predicates) {
            if (std::find(preds.begin(), preds.end(), predicate) != preds.end()) {
                if (!first_condition) smv += " | ";
                smv += "(state_ = " + state + ")";
                first_condition = false;
            }
        }
        if (first_condition) {
            smv += "FALSE";
        }
        smv += " : TRUE;\n\t\t\tTRUE: FALSE;\n\t esac;\n";
    }

    // SPEC: НОВОЕ — берём из модели вместо захардкоженных строк
    smv += "\n";
    if (!model.specifications.empty()) {
        for (const auto& spec : model.specifications) {
            smv += "SPEC\n  " + spec + "\n\n";
        }
    }

    return smv;
}

int main(int argc, char* argv[]) {
    // НОВОЕ: имя файла можно передать аргументом, по умолчанию "model.json"
    std::string input_file = "model.json";
    std::string output_file = "output.smv";

    if (argc >= 2) input_file  = argv[1];
    if (argc >= 3) output_file = argv[2];

    ModelKripke model = parseModel(input_file);
    std::ofstream out(output_file);
    out << generateSMV(model);

    std::cout << "SMV file generated: " << output_file << "\n";
    std::cout << "States: " << model.states.size() << "\n";
    std::cout << "Transitions: " << model.transitions.size() << "\n";
    std::cout << "Specifications: " << model.specifications.size() << "\n";

    return 0;
}
