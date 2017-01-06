/* file: unidupe.cc
 * ----------------
 * Main program that takes three paths as arguments:
 *  - path in 1 (p1)
 *  - path in 2 (p2)
 *  - path out (pout)
 * Will create tree representations of the files and directories in
 * p1 and p2, then create pout, a representation of p1 and p2 merged.
 * Duplicate files as determined by contents and by pathname from the
 * the root (p1 or p2) are identified. In the directory of the most
 * recent file, a hidden folder is created which will contain all other
 * duplicates. The merge preserves all files, directories, and unifies
 * duplicates.
 *
 * After being shown the representations in the terminal, the user is
 * prompted whether to generate the merged folder at pout. If the user
 * chooses yes, folders are created in pout and files are copies from p1
 * and p2 to generate the merged tree with files history folders in the
 * directory of the most recent duplicate.
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

#include "FsTree.h"
#include "FsNode.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <list>

using namespace std;

int main(int argc, char** argv) {
   cout << "\t\t--== UniFs ==--\t\t" << endl;

   // TODO Change internals so more than two directories can be merged in
   // one call by repeatedly merging the previous result of a merge with
   // the next directory.

   // Get input paths from args.
   if (argc != 4) {
      cerr << "Error: Expected 3 arguments." << endl;
      cerr << "\tUsage: unifs pathin1 pathin2 pathout" << endl;
      return -1;
   }
   string path1 = argv[1];
   string path2 = argv[2];
   string pathout = argv[3];

   // Build trees and file hash table.
   unordered_multimap<string, FsNode> fileStore;
   list<FsNode> folderStore;
   FsTree ft1;
   ft1.build(path1, fileStore, folderStore);
   cout << "=== Tree 1 ===" << endl << ft1 << endl;
   FsTree ft2;
   ft2.build(path2, fileStore, folderStore);
   cout << "=== Tree 2 ===" << endl << ft2 << endl;

   // Compute transformation of input FSs for unified FS.
   FsTree ftJoint(ft1, ft2, pathout, fileStore);

   // Output proposed solution.
   cout << ftJoint << endl;

   // Implement proposed solution.
   char resp = '\0';
   while (resp != 'n' && resp != 'Y') {
      cout << "Do you wish to proceed with transformation? (Y, n): ";
      cin >> resp;
   }

   if (resp == 'Y') ftJoint.execTform();

   return 0;
}
