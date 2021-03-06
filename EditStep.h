/* file: EditStep.h
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

#pragma once
#include <string>
#include "FsNode.h"

#define UNIDUPE_MAX_ARGS 5

class EditStep {
  public:
   EditStep() {}
   EditStep(std::string o, FsNode* s, FsNode* d); // Sets all member vars.
//   ~EditStep(); // Breaks program when uncommented for some reason.

   std::string op;
   char* com[UNIDUPE_MAX_ARGS]; // Terminal commands used by execvp.
   FsNode* acting; // Acting node (src for copy, dst for mkdir).
};

