#ifndef ROOTFILE_H
#define ROOTFILE_H

#include <vector>
#include <memory>
#include <optional>
#include "TDirectory.h"
#include "TTree.h"
#include "TLeaf.h"
#include "TH1.h"
#include "TFile.h"

enum class NodeType { DIRECTORY, TTREE, TLEAF, HIST, UNKNOWN };
class RootFile {
public:
    RootFile() = default;
    ~RootFile();
    // Loads and labels a TFile directory structure. Must be called before
    // everything else
    void load(std::string filename);

    // TObject in ROOT file
    class Node final {
    public:
        Node();
        Node(NodeType nt, int idx, Node* mot, int nest);

        // Open/close directory. Toggles content visibility of folders and trees
        void toggleOpenOnClick();

        NodeType type; // TObject category
        int index = -1; // Logical pointer to storage
        Node* mother = nullptr; // So leaves know their tree
        std::vector<std::unique_ptr<Node>> nodes;

        enum OpenState {DIR_OPEN = 0b10, LISTED = 0b01, DEFAULT=0b0};
        std::uint8_t openState = DEFAULT;
        int nesting = 0; // for printing
        bool showInSearch = false; // When search active set to true if match

    private:
        void recurseOpen(bool open);
    } root_node;

    using MenuItem = std::tuple<std::string, Node*>;
    // Return n-th listed RootFile element
    std::optional<MenuItem> getEntry(int, bool searchMode=false);
    std::vector<MenuItem> displayList;

    // Name (title)
    std::string toString(Node*);
    
    // Return number of elements to be displayed in gui
    int menuLength(bool searchMode);

    // Object address storage
    std::vector<TDirectory*> m_directories;
    std::vector<TTree*> m_trees;
    std::vector<TLeaf*> m_leaves;
    std::vector<TH1D*> m_histos_th1d;
    std::vector<TObject*> m_unclassified;

private:
    void traverseTFile(std::string& filename);
    void traverseTFile(TDirectory*, RootFile::Node*, int depth=0);
    void readBranches(RootFile::Node*, TTree*, int depth);
    void populateMenu(Node*, int nesting=0);
    void populateMenu();
    void openObviousDirectory(Node*);

    std::unique_ptr<TFile> m_tfile;
};

#endif // ROOTFILE_H
