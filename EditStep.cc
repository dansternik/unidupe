/* file: EditStep.cc
 * ----------------
 *  From FsNodes, contains command to generate file on file system
 *  using cp or mkdir in terminal.
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

#include "EditStep.h"
#include <stdexcept>
#include <cstring>
using namespace std;

EditStep::EditStep(string o, FsNode* s = nullptr, FsNode* d = nullptr) : op(o) {
   if (d == nullptr)
      throw invalid_argument("Null pointer as destination.");
   if (op != "mkdir" && op != "cp")
      throw invalid_argument("EditStep must be of type mkdir or cp.");

   size_t arg0len = op.size()+1;
   com[0] = new char[arg0len];
   op.copy(com[0], arg0len - 1);
   com[0][arg0len-1] = '\0';
   if (op == "mkdir") {
      size_t arg1len = d->path.size()+1;
      com[1] = new char[arg1len];
      d->path.copy(com[1], arg1len - 1);
      com[1][arg1len-1] = '\0';

      com[2] = com[3] = nullptr;
      acting = d;
   } else if (op == "cp") {
      string opt = "--backup=numbered";
      size_t arg1len = opt.size()+1;
      com[1] = new char[arg1len];
      opt.copy(com[1], arg1len - 1);
      com[1][arg1len-1] = '\0';

      if (s == nullptr)
         throw invalid_argument("EditStep: Null pointer as source.");

      size_t arg2len = s->path.size()+1;
      com[2] = new char[arg2len];
      s->path.copy(com[2], arg2len - 1);
      com[2][arg2len-1] = '\0';
      
      size_t arg3len = d->path.size()+1;
      com[3] = new char[arg3len];
      d->path.copy(com[3], arg3len - 1);
      com[3][arg3len-1] = '\0';

      acting = s;
   }
   com[4] = nullptr;
}

/* // Breaks program when uncommented for some reason.
EditStep::~EditStep() {
   for (unsigned int i = 0; i < UNIFS_MAX_ARGS; i++) {
      if ( com[i] != nullptr) delete[] (com[i]);
   }
}
*/
