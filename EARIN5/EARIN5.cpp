// EARIN5.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <nlohmann/json.hpp>

std::vector<std::string> splitString(std::string in, char splitter) {

    std::vector<std::string> outVector;
    std::string cut="";
    in.erase(remove(in.begin(), in.end(), ' '), in.end());

    unsigned int subStart = 0;
    unsigned int subLen = 0;

    //"T,F,T"
    //"rose,blue"

    for (char c : in) {
        if (c == splitter) {
            cut = in.substr(subStart, subLen);
            subStart += subLen+1;
            subLen = 0;

            outVector.push_back(cut);
        }
        else subLen++;
    }

    cut = in.substr(subStart);
    outVector.push_back(cut);

    return outVector;
}

struct Probability {
    std::vector<std::string> keys;
    double value;

    Probability(std::string keys, double value) {
        this->value = value;
        this->keys = splitString(keys, ',');
    }

    void Print() {
        for (std::string s : keys) {
            std::cout << s << " ";
        }
        std::cout << value << "\n";
    }
};


struct Node {
    std::string nodeName;
    std::vector <std::string> parents;
    std::vector <Probability> probabilities;
    bool isLeaf;

    Node(const nlohmann::json& j,std::string key) {
        this->nodeName = key;
        this->isLeaf = true;
        j.at("parents").get_to(this->parents);
        LoadProbabilities(j.at("probabilities"));
    }

    void LoadProbabilities(const nlohmann::json& j) {
        for (auto& i : j.items()) {
            probabilities.push_back(Probability(i.key(), i.value()));
        }
    }

    bool CheckValidity() {
        bool pair = true;
        double sum;
        for (Probability p : probabilities) {
            if (p.value == NULL) {
                std::cout << nodeName <<": UNDEFINED PROBABILITY!\n";
                return false;
            }
            if (pair) {
                sum = p.value;
                pair = false;
            }
            else {
                pair = true;
                sum += p.value;

                if (sum != 1.0) {
                    std::cout << nodeName << ":PROBABILITY DOESNT ADD TO 1!\n";
                    return false;
                }
            }
        }
        return true;
    }

    void SetLeaf(bool state) {
        this->isLeaf = state;
    }

    void Print() {
        std::cout << nodeName << " " << isLeaf << "\n" << "parents : ";
        for (std::string s : parents) {
            std::cout << s << " ";
        }
        std::cout << "\n";
        for (Probability p : probabilities) {
            p.Print();
        }
    }

};

struct Network {
    std::vector <std::string> nodeNames;
    std::vector <Node> nodes;

    Network(const nlohmann::json& j) {
        j.at("nodes").get_to(this->nodeNames);
        for (auto& i : j.at("relations").items()) {
            nodes.push_back(Node(i.value(), i.key()));
        }

        CheckValidity();
    }

    bool CheckValidity() {
        //check cycles
        SetLeaves(this->nodes,this->nodeNames);
        if (!CheckCycles()) return false;
        //check nodes
        for (Node n : nodes) {
            if (!n.CheckValidity()) return false;
        }
        return true;
    }

    bool CheckCycles() {
        std::vector <Node> copyNodes = this->nodes;
        std::vector <std::string> copyNames = this->nodeNames;
        bool leafs = false;
        int leaf_pos = 0;

        do {
            leafs = false;
            leaf_pos = 0;
            for (Node n : copyNodes) {
                if (n.isLeaf) {
                    leafs = true;
                    break;
                }
                leaf_pos++;
            }

            if (!leafs) return false; //Graph has nodes left but none are leafs, therefore it is cyclic
            
            copyNames.erase(copyNames.begin() + FindNode(copyNodes,copyNodes.at(leaf_pos).nodeName));
            copyNodes.erase(copyNodes.begin() + leaf_pos); //remove leaf
            
            SetLeaves(copyNodes,copyNames);

        }while (!copyNodes.empty());

        return true;
    }

    void SetLeaves(std::vector <Node> &nodes, std::vector <std::string> names) {
        int i;

        for (Node &n : nodes) {
            n.SetLeaf(true);
            if (n.parents.empty()) continue;

            for (std::string parent : n.parents) {
                if (std::find(names.begin(), names.end(), parent) == names.end()) continue;
                i = FindNode(nodes, parent);
                nodes.at(i).SetLeaf(false);
            }
        }
    }

    int FindNode(std::vector <Node> nodes, std::string nodeName) {
        int iterator = 0;
        for (Node n : nodes) {
            if (n.nodeName == nodeName) return iterator;
            iterator++;
        }

        return iterator;
    }

    void Print() {
        for (std::string s : nodeNames) {
            std::cout << s << " ";
        }
        std::cout << "\n";
        for (Node n : nodes) {
            n.Print();
        }
    }

};


int main()
{
    std::ifstream in("C:/Users/JS/source/repos/EARIN5/Test.json");
    nlohmann::json j;
    in >> j;
    

    Network bNet(j);
    bNet.Print();
    std::cout << bNet.CheckValidity() << "\n";

    //nlohmann::json j3 = j2.at(0);
    //std::cout << j3.dump() << "\n";
    //std::string j2String = j2.dump();
    //std::vector <std::string> j2vect = splitString(j2String,',')

    //Probability p1(j["probabilities"].at(1));
    //Network net = j.get<Network>();



}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
