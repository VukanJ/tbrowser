#ifndef ROOTFILE_H
#define ROOTFILE_H

#include <vector>
#include <memory>
#include "TDirectory.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TH1D.h"

enum class NodeType { DIRECTORY, TTREE, TLEAF, HIST, UNKNOWN };
class RootFile {
    public:
        struct Node {
            Node();
            Node(NodeType nt, int idx, Node* mot, int nest);
            void setOpen(bool, int recurse=0);

            NodeType type;
            int index = -1; // Logical pointer to storage
            Node* mother; // So leaves know their tree
            std::vector<std::unique_ptr<Node>> nodes;

            bool directory_open = false;  // By default, open all directories
            bool open = false;
            int nesting = 0;
        } root_node;

        std::string toString(Node*);
        void updateDisplayList(Node*, int nesting=0);
        void updateDisplayList();
        int menuLength();

        std::vector<TDirectory*> m_directories;
        std::vector<TTree*> m_trees;
        std::vector<TLeaf*> m_leaves;
        std::vector<TH1D*> m_histos_th1d;
        std::vector<TObject*> m_unclassified;

        using MenuItem = std::tuple<NodeType, std::string, Node*>;
        std::optional<MenuItem> getEntry(int);
        std::vector<MenuItem> displayList;

};

#endif // ROOTFILE_H
