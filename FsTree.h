/* file: FsTree.h
 * --------------
 * Tree of FsNodes representing a directory's contents. A representation
 * can be built from a path or from two existing trees. If built from two
 * existing trees, the terminal 'cp' and 'mkdir' commands to generate the
 * resulting tree on the file system are queued and can be run with
 * functionality implemented here.
 *
 * -----------------------------------------------------------------
 *  MIT License
 *
 *  Copyright (c) 2017 dansternik (Dominique Piens)
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#pragma once
#include "EditStep.h"
#include "FsNode.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <ostream>
#include <list>

class FsTree {
  public:
   FsTree() : root(nullptr), kMaxProc(10) {}
   // Builds a representation of the two input trees merged. Will modify
   // nodes in the two existing trees.
   FsTree(FsTree& ft1, FsTree& ft2, std::string pathout,
         std::unordered_multimap<std::string, FsNode>& fileStore);
   // Builds a representation of folder at rootpath.
   void build(std::string rootpath,
                std::unordered_multimap<std::string, FsNode>& fileStore,
                std::list<FsNode>& folderStore);
   // Executes cp and mkdir commands in sqeuence to build the tree
   // built as a result of the constructor which takes two trees as
   // inputs.
   void execTform();
   FsNode* getRoot() { return root; }
   friend std::ostream& operator<<(std::ostream& os, const FsTree& ft);

  private:
   // Encapsulates FsNode* and adds a compare function so a priority queue
   // is sorted with the most recently changed files at the front.
   struct FsNodePtr;
   // Helper for FsTree::build that explores rootpath and recurses on
   // its folders, creating nodes in folderStore and filStore.
   void explore(std::string rootpath,
                std::unordered_multimap<std::string, FsNode>& fileStore,
                std::list<FsNode>& folderStore, FsNode* parent);
   // Given a node at the top of a hierarchy (start of path), adds every
   // subordinate node to a priority queue which orders by file recency.
   void traverseSubs(FsNode* nd, std::priority_queue<FsNodePtr>& pq);
   // Given a node in a duplicate subordination hierarchy, selects the most
   // recent duplicate, subordinates others to it, and creates EditSteps to
   // copy every duplicate in a hidden folder in the same directory as the
   // most recent duplicate.
   void makeFileHist(FsNode* src);
   // Helper for constructor taking two trees as inputs. Folds contents of nd2
   // into nd1. After it returns, nd1 should have all the contents of nd1 and
   // nd2.
   void mergeDirs(FsNode* nd1, FsNode* nd2, std::unordered_set<FsNode*>& sups);
   // Helper for execTform, stores the next EditStep to execute in step.
   bool getNextStep(EditStep& step);

   FsNode* root;
   // Where new nodes resulting from merging two trees are stored.
   std::list<FsNode> plannedNode;
   std::queue<EditStep> editSteps;
   const unsigned int kMaxProc; // Max child processes to run in execTform.
};

