#include "RootFile.h"

RootFile::Node::Node() : type(NodeType::UNKNOWN), index(-1) { }
RootFile::Node::Node(NodeType nt, int idx, Node* mot, int nest) 
    : type(nt), index(idx), mother(mot), nesting(nest) { }

void RootFile::updateDisplayList(Node* mothernode, int nesting) {
    for (auto& node : mothernode->nodes) {
        switch (node->type) {
            case NodeType::DIRECTORY:
                displayList.emplace_back(node->type, m_directories[node->index]->GetName(), node.get());
                updateDisplayList(node.get(), nesting+1);
                break;
            case NodeType::TTREE:
                displayList.emplace_back(node->type, m_trees[node->index]->GetName(), node.get());
                updateDisplayList(node.get(), nesting+1);
                break;
            case NodeType::TLEAF:
                displayList.emplace_back(node->type, m_leaves[node->index]->GetName(), node.get());
                break;
            case NodeType::HIST:
                displayList.emplace_back(node->type, m_histos_th1d[node->index]->GetName(), node.get());
                break;
            case NodeType::UNKNOWN:
                displayList.emplace_back(node->type, m_unclassified[node->index]->GetName(), node.get());
                break;
            default: break;
        }
    }
}

void RootFile::updateDisplayList() {
    // Make flat file structure list for quick redraw
    displayList.clear();
    displayList.emplace_back(root_node.type, m_directories[root_node.index]->GetName(), &root_node);
    updateDisplayList(&root_node);
}

std::optional<RootFile::MenuItem> RootFile::getEntry(int i) {
    int entry = 0;
    for (size_t j = 0; j < displayList.size(); ++j) {
        if (std::get<2>(displayList[j])->open) {
            if (entry == i) {
                return displayList[j];
            }
            entry++;
        }
    }
    return std::nullopt;
}

int RootFile::menuLength() {
    int length = 0;
    for (const auto& [a, b, node] : displayList) {
        if (node->open) {
            length++;
        }
    }
    return length;
}

void RootFile::Node::setOpen(bool state, int recurse) {
    if (recurse > 0) {
        open = state; // Dont hide element we are clicking on
    }
    if (type == NodeType::DIRECTORY || type == NodeType::TTREE) {
        directory_open = state;
        for (auto& node : nodes) {
            node->setOpen(state, recurse + 1);
        }
    }
    else {
        open = state;
    }
}

std::string RootFile::toString(Node* node) {
    std::string descr;
    std::string name;
    std::string title;
    std::string classname;
    auto make_name = [&name, &title, &classname](TObject* obj){
        name = obj->GetName();
        title = obj->GetTitle();
        classname = obj->ClassName();
        if (name != title) {
            return std::format("({}) {} \"{}\"", classname, name, title);
        }
        return std::format("({}) {}", classname, name, title);
    };
    switch (node->type) {
        case NodeType::DIRECTORY: descr = make_name(m_directories[node->index]);  break;
        case NodeType::TTREE:     descr = make_name(m_trees[node->index]);        break;
        case NodeType::TLEAF:     descr = make_name(m_leaves[node->index]);       break;
        case NodeType::HIST:      descr = make_name(m_histos_th1d[node->index]);  break;
        case NodeType::UNKNOWN:   descr = make_name(m_unclassified[node->index]); break;
    }

    return descr;
}
