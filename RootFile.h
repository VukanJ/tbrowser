#ifndef ROOTFILE_H
#define ROOTFILE_H

#include <vector>
#include <memory>
#include "TDirectory.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TH1.h"
#include "TFile.h"

enum class NodeType { DIRECTORY, TTREE, TLEAF, HIST, UNKNOWN };
class RootFile {
public:
    ~RootFile();
    void load(std::string filename);

    class Node final {
    public:
        Node();
        Node(NodeType nt, int idx, Node* mot, int nest);
        void toggleOpenOnClick();

        NodeType type;
        int index = -1; // Logical pointer to storage
        Node* mother; // So leaves know their tree
        std::vector<std::unique_ptr<Node>> nodes;

        enum OpenState {DIR_OPEN = 0b10, LISTED = 0b01, DEFAULT=0b0};
        std::uint8_t openState = DEFAULT;
        int nesting = 0;
    private:
        void recurseOpen(bool open);
    } root_node;

    using MenuItem = std::tuple<std::string, Node*>;
    std::optional<MenuItem> getEntry(int);
    std::vector<MenuItem> displayList;

    std::string toString(Node*);
    int menuLength();

    std::vector<TDirectory*> m_directories;
    std::vector<TTree*> m_trees;
    std::vector<TLeaf*> m_leaves;
    std::vector<TH1D*> m_histos_th1d;
    std::vector<TObject*> m_unclassified;

private:
    std::unique_ptr<TFile> m_tfile;
    void traverseTFile(TDirectory*, RootFile::Node*, int depth=0);
    void traverseTFile(std::string& filename);
    void readBranches(RootFile::Node*, TTree*, int depth);

    void populateMenu(Node*, int nesting=0);
    void populateMenu();
};

#endif // ROOTFILE_H
