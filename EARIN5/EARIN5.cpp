// EARIN5.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <time.h>
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
    std::vector <std::string> parentsNames;
    std::vector <std::string> potentialValues;
    std::vector <Probability> probabilities;
    std::vector <Node*> parents;
    std::vector <Node*> children;
    std::string nodeValue;
    bool isLeaf;
    int visits;

    Node(const nlohmann::json& j,std::string key) {
        this->nodeName = key;
        this->isLeaf = true;
        j.at("parents").get_to(this->parentsNames);
        LoadProbabilities(j.at("probabilities"));
        LoadPotentialValues();
        nodeValue = "";
        visits = 0;
    }

    void LoadProbabilities(const nlohmann::json& j) {
        for (auto& i : j.items()) {
            probabilities.push_back(Probability(i.key(), i.value()));
        }
    }

    void LoadPotentialValues() {
        for (Probability p : probabilities) {
            //load last key
            potentialValues.push_back(p.keys.at(p.keys.size() - 1));
        }

        sort(potentialValues.begin(), potentialValues.end());
        potentialValues.erase(std::unique(potentialValues.begin(), potentialValues.end()), potentialValues.end());

    }

    void AddParent(Node* node){
        this->parents.push_back(node);
    }

    void RemoveParent(Node* node){
        std::vector<Node*>::iterator p;
        p = find(this->parents.begin(),this->parents.end(), node);

        this->parents.erase(p);
    }

    void AddChild(Node* node){
        this->children.push_back(node);
    } 

    void RemoveChild(Node* node){
        //std::vector<Node*>::iterator p;
        //p = find(this->children.begin(), this->children.end(), node);

        int pos = 0;
        for (Node* n : this->children) {
            if (n->nodeName == node->nodeName) break;
            pos++;
        }

        this->children.erase(this->children.begin()+pos);
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

    double GetProbForCurrentParents(std::string potentialValue) {
        std::vector <std::string> parentValues;
        for (Node* parent : parents) {
            parentValues.push_back(parent->nodeValue);
        }
        parentValues.push_back(potentialValue);

        bool match = true;
        //check if match
        for (Probability prob : probabilities) {
            for (int i = 0; i < parentValues.size(); i++) {
                if (prob.keys.at(i) != parentValues[i]) break;

                if (i == parentValues.size() - 1) return prob.value;

            }
        }
    }

    void SetRandomValue() {
        int tmp = rand() % 10000;
        double prob = this->probabilities.at(0).value;
        int keys_len = this->probabilities.at(0).keys.size();
        if ((double)(tmp / 10000) > prob) {
            this->nodeValue = this->probabilities.at(0).keys.at(keys_len - 1);
        }
        else {
            this->nodeValue = this->probabilities.at(1).keys.at(keys_len - 1);
        }
    }

    void SetLeaf(bool state) {
        this->isLeaf = state;
    }

    void UpdateLeaf() {
        //If it has no children then it is a leaf
        if (this->children.empty()) this->isLeaf = true;
        else this->isLeaf = false;
    }

    void SetNodeValue(std::string newValue) {
        this->nodeValue = newValue;
    }

    void Print() {
        std::cout << nodeName << " " << isLeaf << "\n" << "parents : ";
        for (std::string s : parentsNames) {
            std::cout << s << " ";
        }
        std::cout << "\n";
        for (Probability p : probabilities) {
            p.Print();
        }
    }

    std::string PrintName() {
        return this->nodeName;
    }
};

struct Network {
    std::vector <std::string> nodeNames;
    std::vector <Node> nodes;
    bool valid;

    Network(const nlohmann::json& j) {
        j.at("nodes").get_to(this->nodeNames);
        for (auto& i : j.at("relations").items()) {
            nodes.push_back(Node(i.value(), i.key()));
        }

        this->valid = CheckValidity();
    }

    bool CheckValidity() {
        //check cycles
        
        if (!CheckCycles()) return false;

        SetParentsChildren(nodes);
        //check nodes
        for (Node n : nodes) {
            if (!n.CheckValidity()) return false;
        }
        return true;
    }

    bool IsValid() {
        return this->valid;
    }

    bool CheckCycles() {
        std::vector <Node> copyNodes = this->nodes;
        SetParentsChildren(copyNodes);
        Node *leaf;
        std::vector <std::string> copyNames = this->nodeNames;

        bool leafs = false;
        int leaf_pos = 0;

        do {

            //find a leaf, if there are none then return false
            for (Node n : copyNodes) {

                if (n.isLeaf) {
                    leafs = true;
                    break;
                }
                leaf_pos++;
            }
            if (!leafs) {
                std::cout << "Graph is cyclic!\n";
                return false;
            }
            //remove the leaf
            //Remove all references in parent nodes
            leaf = &copyNodes.at(leaf_pos);
            for (Node* n : leaf->parents) {
                //n->RemoveChild(leaf);//!!!!!!!!!!!!!!
                n->children.clear();
            }


            copyNodes.erase(copyNodes.begin()+leaf_pos);

            //Only 1 node left, it must be a leaf, graph must be acyclic
            if (copyNodes.size() == 1) return true;

            //return to starting state
            SetParentsChildren(copyNodes);
            //SetLeaves(copyNodes);
            leafs = false;
            leaf_pos = 0;

        }while (!copyNodes.empty());

        return true;
    }

    void SetParentsChildren(std::vector <Node> &nodes) {
        
        std::vector<std::string>::iterator p;

        //clear nodes
        for (Node& n : nodes) {
            n.children.clear();
            n.parents.clear();

        }

        //For each node, find each parent and set the current node as the parent child, set parent for current node
        for (Node &n : nodes) {

            if (n.parentsNames.empty()) continue;

            for (Node &parent : nodes) {
                p = find(n.parentsNames.begin(), n.parentsNames.end(), parent.nodeName);

                //nodeName same as parent.nodeName is present in n.parentsNames
                if (p != n.parentsNames.end()) {
                    parent.AddChild(&n);
                    n.AddParent(&parent);
                }
                else {
                    continue;
                }
            }
        }
        SetLeaves(nodes);
    }
    
    void SetLeaves(std::vector <Node> &nodes) {
        for (Node &n : nodes) {
           
            n.UpdateLeaf();

        }
    }

    int FindNodeIterator(std::vector <Node> nodes, std::string nodeName) {
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

    void PrintBlanket(std::string nodeName) {

        std::vector <std::string> blanketNames = GetBlanket(nodeName);

        for (std::string s : blanketNames) {
            std::cout << s << " ";
        }

    }

    std::vector <std::string> GetBlanket(std::string nodeName) {

        int nodePos = FindNodeIterator(this->nodes, nodeName);
        std::vector <std::string> blanketNames;

        Node node = this->nodes.at(nodePos);
        //print parents
        for (Node* n : node.parents) {
            blanketNames.push_back(n->PrintName());
        }

        //print children
        for (Node* n : node.children) {
            blanketNames.push_back(n->PrintName());
            //print parents of children of node in question
            for (Node* m : n->parents) {
                blanketNames.push_back(m->PrintName());
            }
        }

        sort(blanketNames.begin(), blanketNames.end());
        blanketNames.erase(std::unique(blanketNames.begin(), blanketNames.end()), blanketNames.end());

        nodePos = 0;
        //Remove itself from the blanket
        for (std::string s : blanketNames) {
            if (s == nodeName) {
                blanketNames.erase(blanketNames.begin() + nodePos);
                break;
            }
            else {
                nodePos++;
            }
        }

        return blanketNames;

    }

    //evidence, querry
    //call MCMC("NodeName=T,NodeName=F","NodeName")
    void MCMC(std::string evidence, std::string querry,int iterations) {

        std::vector <std::string> evidenceNames;
        std::vector <std::string> BlanketNames;
        std::vector <std::string> evidenceNodes = splitString(evidence, ',');

        //Holds current values of each node in each iteration
        std::vector <std::string> nodeValHistory;

        //Fill values
            //Get names and values in evidence
         //each holds "NodeName=T"
        
        for (std::string eNode : evidenceNodes) {
            std::vector <std::string> singleNodeEvidence = splitString(eNode, '=');
            this->nodes.at(FindNodeIterator(this->nodes, singleNodeEvidence[0])).nodeValue = singleNodeEvidence[1];
            evidenceNames.push_back(singleNodeEvidence[0]);
        }

        /*
        for (Node n : this->nodes) {
            if (n.nodeValue != "") continue;
            
            n.SetRandomValue();
        }
        */

        for (int a = 0; a < this->nodes.size(); a++) {
            if (this->nodes.at(a).nodeValue != "") continue;
            this->nodes.at(a).SetRandomValue();
        }

        int index;
        bool present = false;
        double probP;
        double probC=1;
        double probTotal;
        std::vector <std::string> potentialValues;
        std::vector <std::string> childPotentialValues;

        std::vector <double> parentsProbs;
        std::vector <double> childrenProbs;
        std::vector <double> probs;

        std::string walkingNodeName;
        std::string savedValue;

        //Random Walking
        for (int i = 0; i < iterations; i++) {
            present = false;
            //Draw random node from network not in evidence
            do {
                index = rand() % this->nodes.size();

                //check if node is present in evidence
                if (std::count(evidenceNames.begin(), evidenceNames.end(), this->nodes.at(index).nodeName)) present = true;
                else present = false;

            } while(present);

            walkingNodeName = this->nodes.at(index).nodeName;

            //Calculate P for blanket
                //Calculate Blanket
            BlanketNames = GetBlanket(walkingNodeName);
                //For each possible value of node P(X=xj | MB(X))
            potentialValues = this->nodes.at(index).potentialValues;
            savedValue = this->nodes.at(index).nodeValue;
            probs.clear();
            parentsProbs.clear();
            childrenProbs.clear();
            for (std::string potVal : potentialValues) {

                probC = 1;
                probP = 0;
                

                //Parents = aP(X=xj|Parents(X)) -> P(X = xj | current values of X's parents)
                probP = this->nodes.at(index).GetProbForCurrentParents(potVal);
                parentsProbs.push_back(probP);
                //GetNodeProbForGivenParents
                //set X = xj
                this->nodes.at(index).SetNodeValue(potVal);



                for (Node* c : this->nodes.at(index).children) {
                    childPotentialValues = c->potentialValues;

                    probC *= c->GetProbForCurrentParents(c->nodeValue);
                }

                //push probP and probC onto vector
                childrenProbs.push_back(probC);
                this->nodes.at(index).SetNodeValue(savedValue);
            }

            double sumParents=0;
            for (double p : parentsProbs) {
                sumParents += p;
            }
            double alpha = (1.0) / sumParents;
            double tmp;
            //alpha = 1/ sum(parentProbs)
            //P(X=xj | MB(X)) = alpha * parentProb[j] * childrenProb[j]
            //for each possible xj calculate P(X=xj | blanket)
            for (int j = 0; j < parentsProbs.size(); j++) {
                tmp = (alpha * parentsProbs[j] * childrenProbs[j]);
                probs.push_back(tmp);
            }
            //Set value
            double sumProbs = 0;
            for (double p : probs) {
                sumProbs += p;
            }
            int randInt = rand() % 100000;
            double randVal = (randInt*sumProbs) / 100000;
            double offset = 0.0;
            int iter = 0;
            for (double p : probs) {
                offset += p;
                if (randVal < offset) {
                    //Set selected node value to appropriate value
                    this->nodes.at(index).SetNodeValue(potentialValues[iter]);
                    break;
                }
                iter++;
            }
            //std::cout << this->nodes.at(index).nodeName << " " << potentialValues[iter] << "\n";
            //Increase counter
            this->nodes.at(index).visits++;


            //Save current node values
            std::string currentValues = "";
            for (Node n : this->nodes) {
                currentValues += n.nodeValue + ",";
            }
            nodeValHistory.push_back(currentValues);

        }

        //Unpack and calculate probs from history
        std::vector <int> occurences;
        std::vector <std::string> splitHistory;
        int qIndex = FindNodeIterator(this->nodes,querry);
        std::string part;
        int correctIndex;
        for (int c = 0; c < this->nodes.at(qIndex).potentialValues.size(); c++) {
            occurences.push_back(0);
        }

        for (std::string hval : nodeValHistory) {
            //Split the string into array/vector
            splitHistory.clear();
            splitHistory = splitString(hval, ',');

            part = splitHistory[qIndex];
            correctIndex = 0;
            for (std::string s : this->nodes.at(qIndex).potentialValues) {
                if (s == part) {
                    occurences[correctIndex]++;
                }
                correctIndex++;
            }

        }

        double x;
        std::cout << this->nodes.at(qIndex).nodeName << " : ";
        for (int d = 0; d < occurences.size();d++) {
            x = (double)occurences[d] / (double)iterations;
            std::cout << " " << this->nodes.at(qIndex).potentialValues[d] << ":" << occurences[d] << "/" << iterations << "=" << x;
        }

    }
};


int main()
{
    srand(time(NULL));
    std::ifstream in("C:/Users/JS/source/repos/EARIN5/Test.json");
    nlohmann::json j;
    in >> j;
    

    Network bNet(j);
    if (!bNet.IsValid()) {
        
        std::cout << "Network is invalid...\n";
        return 0;

    }
    //bNet.PrintBlanket("burglary");
    bNet.MCMC("burglary=T,alarm=T", "earthquake",10000);

    //std::cout << bNet.CheckValidity() << "\n";

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
