/* file: FsNode.cc
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

#include "FsNode.h"
#include <ostream>
#include <sstream>
#include <string>

using namespace std;

FsNode::FsNode(std::string n, FsNode* p, std::string t) :
   num_files(0), type(t), name(n), parent(p), isSub(false), topSup(nullptr),
   dstParent(nullptr), is_created(false) {
      setParent(p);
}

void FsNode::setParent(FsNode* p) {
   parent = p;
   if (p != nullptr) path = p->path + "/" + name;
}

string FsNode::toString(string prefix) {
   stringstream ss;
   ss << prefix + name + "\n";
   for (pair<string, FsNode*> n : children) {
      ss << n.second->toString(prefix + "  ");
   }
   return ss.str();
}

void FsNode::makeSub(FsNode* sup) {
   sup->subordinates.push_back(this);
   isSub = true;
   topSup = (sup->isSub) ? sup->topSup : sup;
}

