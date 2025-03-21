#include "RootFile.h"
#include "TKey.h"
#include "definitions.h"
#include <memory>


RootFile::Node::Node() : type(NodeType::UNKNOWN) { }

RootFile::Node::Node(NodeType nt, int idx, Node* mot, int nest) 
    : type(nt), index(idx), mother(mot), nesting(nest) { }

RootFile::~RootFile() {
    if (m_tfile != nullptr) {
        if (m_tfile->IsOpen()) {
            m_tfile->Close();
        }
    }
}

void RootFile::load(std::string filename) {
    traverseTFile(filename);
    populateMenu();
}

void RootFile::populateMenu() {
    // Make flat file structure list for quick redraw
    displayList.clear();
    displayList.emplace_back(m_directories[root_node.index]->GetName(), &root_node);
    populateMenu(&root_node);
}

void RootFile::populateMenu(Node* mothernode, int nesting) {
    for (auto& node : mothernode->nodes) {
        switch (node->type) {
            case NodeType::DIRECTORY:
                displayList.emplace_back(m_directories[node->index]->GetName(), node.get());
                populateMenu(node.get(), nesting + 1);
                break;
            case NodeType::TTREE:
                displayList.emplace_back(m_trees[node->index]->GetName(), node.get());
                populateMenu(node.get(), nesting + 1);
                break;
            case NodeType::TLEAF:
                displayList.emplace_back(m_leaves[node->index]->GetName(), node.get());
                break;
            case NodeType::HIST:
                displayList.emplace_back(m_histos_th1d[node->index]->GetName(), node.get());
                break;
            case NodeType::UNKNOWN:
                displayList.emplace_back(m_unclassified[node->index]->GetName(), node.get());
                break;
            default: break;
        }
    }
}

std::optional<RootFile::MenuItem> RootFile::getEntry(int i, bool searchMode) {
    // Return reference to i-th open entry
    int entry = 0;
    for (int j = 0; auto& [_, node] : displayList) {
        bool criterion = searchMode ? node->showInSearch : node->openState & RootFile::Node::LISTED;
        if (criterion) {
            if (entry == i) {
                return displayList[j];
            }
            entry++;
        }
        j++;
    }
    return std::nullopt;
}

int RootFile::menuLength(bool searchMode) {
    // Return number of open entries
    int length = 0;
    if (searchMode) {
        for (const auto& [_, node] : displayList) {
            if (node->showInSearch) {
                length++;
            }
        }
    }
    else {
        for (const auto& [_, node] : displayList) {
            if (node->openState & RootFile::Node::LISTED) {
                length++;
            }
        }
    }
    return length;
}

void RootFile::Node::toggleOpenOnClick() {
    if (type == NodeType::DIRECTORY || type == NodeType::TTREE) {
        openState ^= DIR_OPEN;
        recurseOpen(openState & DIR_OPEN);
    }
}

void RootFile::Node::recurseOpen(bool open) {
    for (auto& child : nodes) {
        if (open) {
            child->openState |= LISTED;
        }
        else {
            child->openState ^= LISTED;
        }

        if ((child->type == NodeType::DIRECTORY || child->type == NodeType::TTREE) && child->openState & DIR_OPEN) {
            // If subfolder is opened and now listed, list its contents as well
            child->recurseOpen(open);
        }
    }
}

std::string RootFile::toString(Node* node) {
    std::string descr;
    std::string name;
    std::string title;
    std::string classname;
    auto make_name = [this, &name, &title, &classname, &node](TObject* obj){
        name = obj->GetName();
        title = obj->GetTitle();
        classname = obj->ClassName();
        
        Long64_t nEntries = -1;
        if (node->type == NodeType::TTREE) {
            nEntries = m_trees[node->index]->GetEntriesFast();
        }

        std::string info;

        info = fmtstring("({}) {}", classname, name);
        if (!title.empty() && name != title) {
            info += fmtstring(" \"{}\"", title);
        }
        if (nEntries > -1) {
            info += fmtstring(", Entries: {}", nEntries);
        }
        return info;
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

void RootFile::traverseTFile(TDirectory* dir, RootFile::Node* node, int depth) {
    TList* keys = dir->GetListOfKeys();
    if (keys == nullptr) {
        return;
    }

    for (int i = 0; i < keys->GetSize(); ++i) {
        TKey* key = dynamic_cast<TKey*>(keys->At(i));
        TObject* obj = key->ReadObj();

        if (strcmp(key->GetClassName(), "TTree") == 0) {
            auto* tree = dynamic_cast<TTree*>(obj);
            m_trees.push_back(tree);
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::TTREE, m_trees.size() - 1, node, depth));
            readBranches(node->nodes.back().get(), tree, depth + 1);
        }
        else if (strcmp(key->GetClassName(), "TH1D") == 0) {
            m_histos_th1d.push_back(dynamic_cast<TH1D*>(obj));
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::HIST, m_histos_th1d.size() - 1, node, depth));
        }
        else if (obj->IsA()->InheritsFrom(TDirectory::Class())) {
            m_directories.push_back(dynamic_cast<TDirectory*>(obj));
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::DIRECTORY, m_directories.size() - 1, node, depth));
            traverseTFile(dynamic_cast<TDirectory*>(obj), node->nodes.back().get(), depth + 1);
        }
        else {
            m_unclassified.push_back(obj);
            node->nodes.emplace_back(std::make_unique<RootFile::Node>(NodeType::UNKNOWN, m_unclassified.size() - 1, node, depth));
        }
    }
}

void RootFile::traverseTFile(std::string& filename) {
    m_tfile = std::unique_ptr<TFile>(TFile::Open(filename.c_str(), "READ"));
    if (!m_tfile || m_tfile->IsZombie()) {
        throw std::runtime_error("Not a root file or broken file");
    }

    m_directories.push_back(m_tfile.get());
    root_node.index = 0;
    root_node.type = NodeType::DIRECTORY;
    traverseTFile(m_tfile.get(), &root_node);

    // Open stuff
    root_node.openState |= RootFile::Node::LISTED;
    root_node.toggleOpenOnClick();
    openObviousDirectory(&root_node);
}

void RootFile::readBranches(RootFile::Node* node, TTree* tree, int depth) {
    for (auto* leaf : *tree->GetListOfLeaves()) {
        m_leaves.push_back(dynamic_cast<TLeaf*>(leaf));
        node->nodes.emplace_back(std::make_unique<RootFile::Node>(
                    NodeType::TLEAF, m_leaves.size() - 1, node, depth));
    }
}

void RootFile::openObviousDirectory(Node* node) {
    int dirCount = 0;
    Node* gotoDir = nullptr;
    for (const auto& child : node->nodes) {
        if (child->type == NodeType::DIRECTORY || child->type == NodeType::TTREE) {
            dirCount++;
            gotoDir = child.get();
        }
    }

    if (dirCount == 1) {
        gotoDir->toggleOpenOnClick();
        openObviousDirectory(gotoDir);
    }
}
