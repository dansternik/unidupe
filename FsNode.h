/* file: FsNode.h
 * --------------
 * File or directory node for FsTree. Has functionality to build tree
 * representation from a path, and to generate file or folder on the
 * file system.
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
#include <string>
#include <vector>
#include <unordered_map>
#include <ostream>
#include <time.h>

class FsNode {
  public:
   FsNode() : num_files(0), parent(nullptr), isSub(false), is_created(false) {}
   // Init list + calls setParent
   FsNode(std::string n, FsNode* p, std::string t);
   // Sets parent and path
   void setParent(FsNode* p);
   void setDstParent(FsNode* p) { if (p != nullptr) dstParent = p; }
   // Recursively print self and children nodes with offset.
   std::string toString(std::string prefix);
   // Makes node subordinate to sup.
   void makeSub(FsNode* sup);

   // Present for possible improvement which treats large files differently.
   size_t size; 
   size_t num_files; // for folders.
   struct timespec date_changed;
   std::string type;
   std::string name;
   std::string path; // Used by EditStep to prepare shell commands.
   FsNode* parent;
   std::unordered_map<std::string, FsNode*> children;

   // Used to merge trees.
   bool isSub;
   std::vector<FsNode*> subordinates;
   FsNode* topSup; // Node known to be at the top of subordination hierarchy.
   FsNode* dstParent; // For files, to get path to copy to.
   bool is_created;
};

