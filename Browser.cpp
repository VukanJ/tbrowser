#include "Browser.h"
#include <iostream>
#include <format>
#include <ncurses.h>

bool resize_flag = false;

FileBrowser::FileBrowser() { }

FileBrowser::~FileBrowser() { }

void FileBrowser::populate(std::string filename) {
    // Get Pointers to directories, trees, and histograms
    m_filename = filename;
    m_tfile = std::unique_ptr<TFile>(TFile::Open(m_filename.c_str(), "READ"));
    if (!m_tfile || m_tfile->IsZombie()) {
        std::cerr << "File cannot be opened\n";
        exit(EXIT_FAILURE);
    }

    for (auto* KEY : *m_tfile->GetListOfKeys()) {
        TKey* key = (TKey*)KEY;
        if (strcmp(key->GetClassName(), "TTree") == 0) {
            m_trees.push_back(static_cast<TTree*>(key->ReadObj()));
        }
        else if (strcmp(key->GetClassName(), "TH1D") == 0) {
            m_histos.push_back(static_cast<TH1D*>(key->ReadObj()));
        }
        else if (strcmp(key->GetClassName(), "TH2D") == 0) {
        }
    }
    // Read branch names
    for (TTree* tree : m_trees) {
        for (auto leaf : *tree->GetListOfLeaves()) {
            m_leaves.push_back(static_cast<TLeaf*>(leaf));
        }
    }
}

void FileBrowser::printFiles(int lines, int cols, int x, int y) {
    mvprintw(x, y, ""); // TODO print tree
    for (size_t i = 0; i < m_leaves.size(); ++i) {
        auto name = std::string(" ") + m_leaves[i]->GetName();
        if (selected == i) {
            attron(A_REVERSE);
            mvprintw(y + i+1, x, name.c_str());
            attroff(A_REVERSE);
        }
        else {
            mvprintw(y + i+1, x, name.c_str());
        }
    }
}
