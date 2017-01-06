/* file: FsTree.cc
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

#include "FsTree.h"
#include "FsNode.h"
#include "EditStep.h"

#include <unordered_map>
#include <string>
#include <cstring>
#include <ostream>
#include <iostream>
#include <stdexcept>
#include <list>
#include <unordered_set>
#include <queue>

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

/* Global so signal handler can access these for coordinating
 * multiprocessing.
 */
// pid to FsNode*, so signal handler can find entry in editQueue and keep
// track of running processes.
static unordered_map<pid_t,FsNode*> editProc;
// Values are edit steps pending the completion of mkdir of the key. Released
// to jobs when signal handler finds mkdir of the key is complete.
static unordered_multimap<FsNode*, EditStep> editQueue;
// Job queue for pending steps that are now ready to run.
static queue<EditStep> jobs;


/* function: comparRecent
 * ----------------------
 * Used to maintain FsNode ordering by file recency, then folder crowding.
 */
static inline bool comparRecent(const FsNode& nd1, const FsNode& nd2) {
   struct timespec t1 = nd1.date_changed;
   struct timespec t2 = nd2.date_changed;
   if (t1.tv_sec != t2.tv_sec) {
      return t1.tv_sec < t2.tv_sec;
   } else {
      if (t1.tv_nsec != t2.tv_nsec) {
         return t1.tv_nsec < t2.tv_nsec;
      } else {
         if (nd1.parent->num_files == nd2.parent->num_files)
            return nd1.isSub;
         else
            return nd1.parent->num_files > nd2.parent->num_files;
      }
   }
}


/* struct: FsNodePtr
 * -----------------
 */
struct FsTree::FsNodePtr {
   FsNode* n;
   FsNodePtr(FsNode* nd) : n(nd) {}

   bool operator< (const FsNodePtr& rhs) const {
      return comparRecent(*n, *(rhs.n));
   }
};


/* function: FsTree
 * ----------------
 * Constructor with two trees to merge as inputs.
 */
FsTree::FsTree(FsTree& ft1, FsTree& ft2, string pathout,
      unordered_multimap<string,FsNode>& fileStore) : kMaxProc(10) {
   cout << "Planning merged tree at " << pathout <<  endl;
   // Used to ensure we only visit files with a given hash value once.
   unordered_set<string> fhash;
   for (auto it : fileStore) {
      if (fhash.find(it.first) != fhash.end())
         continue;

      fhash.insert(it.first);
      if (fileStore.count(it.first) > 1) { // Identify duplicates
         // Find most recently created duplicate.
         pair<unordered_multimap<string,FsNode>::iterator,
              unordered_multimap<string,FsNode>::iterator> lims = fileStore.equal_range(it.first);
         unordered_multimap<string,FsNode>::iterator best_file;
         bool is_first = true;
         for (unordered_multimap<string,FsNode>::iterator dupe = lims.first;
              dupe != lims.second; dupe++) {
            if (is_first || comparRecent((dupe->second), (best_file->second))) {
               is_first = false;
               best_file = dupe; // replace with file in best dir.
            }
         }

         // Make all duplicates subordinate to the most recent one.
         for (unordered_multimap<string,FsNode>::iterator dupe = lims.first;
              dupe != lims.second; dupe++) {
            if (dupe != best_file)
               dupe->second.makeSub(&(best_file->second));
         }
      }
   }
   // Create root node for new tree.
   plannedNode.push_back(*(ft1.getRoot()));
   root = &(plannedNode.back());
   root->name = pathout;
   root->path = pathout;
   root->setParent(nullptr);
   editSteps.push(EditStep("mkdir", nullptr, root));

   // Track files found as superior in a duplicate hierarchy.
   unordered_set<FsNode*> sups;
   // Create tree from merging both input trees.
   mergeDirs(root, ft2.getRoot(), sups);
   // Resolve content and path duplicates found in trees.
   for (FsNode* sup : sups)
      makeFileHist(sup);
}


/* function: build
 * ---------------
 */
void FsTree::build(string rootpath,
                     unordered_multimap<string,FsNode>& fileStore,
                     list<FsNode>& folderStore) {
   cout << "Exploring tree at " << rootpath << endl;
   // Check path valid
   struct stat st;
   if (stat(rootpath.c_str(), &st) != 0)
      throw invalid_argument("Could not locate " + rootpath);
   if (!S_ISDIR(st.st_mode))
      throw invalid_argument(rootpath + " is not a directory.");
   // Add new node to filestore (initialize and hash file)
   FsNode nd;
   nd.name = rootpath;
   nd.path = rootpath;
   nd.type = "dir";
   // Recurse on dir contents
   folderStore.push_back(nd);
   FsNode& curNode = folderStore.back();
   root = &(curNode);

   explore(rootpath, fileStore, folderStore, root); 
}


/* function: handleChildProc
 * -------------------------
 *  Signal handler for SIGCHLD installed in FsTree::execTform. Upon child process
 *  termination, moves EditSteps pending its creation into the hob queue.
 */
static void handleChildProc(int sig) {
   int status;
   pid_t pid;
   while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
      if (!(WIFEXITED(status) || WIFSIGNALED(status)))
         continue;
      // Find FsNode* corresponding to process that just exited. Move edit steps
      // depending and waiting on this process to the job list.
      unordered_map<pid_t,FsNode*>::iterator it = editProc.find(pid);
      if (it == editProc.end())
         continue;
      FsNode* nd =  it->second;
      nd->is_created = true;

      if (editQueue.find(nd) != editQueue.end()) {
         pair<unordered_multimap<FsNode*, EditStep>::iterator,
                 unordered_multimap<FsNode*, EditStep>::iterator> lims = editQueue.equal_range(nd);
         for (unordered_multimap<FsNode*,EditStep>::iterator client = lims.first;
                 client != lims.second; client++) {
            jobs.push(client->second);
         }
         editQueue.erase(nd);
      }
      editProc.erase(pid);
   }
   if (pid == -1 && errno != ECHILD)
      throw system_error(errno, system_category());
}


/* function: execTform
 * -------------------
 *  Has signal handler handleChildProc, uses globals jobs, editQueue, editProc.
 */
void FsTree::execTform() {
   if (plannedNode.empty())
      throw domain_error("execTfrom() must be called on a tree built from existing trees.");
   struct sigaction action;
   action.sa_handler = handleChildProc;
   sigemptyset(&action.sa_mask);
   action.sa_flags = SA_RESTART;
   if (sigaction(SIGCHLD, &action, NULL) < 0)
      throw system_error(errno, system_category());

   cout << "Tform!" << endl;
   // In general, block SIGCHLD signals since they will modify editQueue,
   // editSteps and jobs.
   sigset_t set, oldset;
   sigemptyset(&set);
   sigaddset(&set, SIGCHLD);
   sigprocmask(SIG_BLOCK, &set, &oldset);
   while (!(editSteps.empty() && editQueue.empty() && jobs.empty())) {
      // Wait for running process number to go down, or pending jobs to be moved
      // to the job queue.
      while ( editProc.size() > kMaxProc || (jobs.empty() && editSteps.empty()) ) {
         // Let SIGCHLD signals be handled.
         sigprocmask(SIG_SETMASK, &oldset, NULL);
         pause();
         // Proper error handling should involve following three lines, but 
         // when uncommented, there is a always an error of system call
         // interrupted. Program seems to function fine without though.
         //int err = pause();
         //if (err < 0 && err != EINTR)
         //   throw system_error(errno, system_category());
         sigprocmask(SIG_BLOCK, &set, NULL);
      }
      
      EditStep step;
      if (!getNextStep(step)) // If all steps depend on running jobs, loop and wait.
         continue;
      pid_t pid = fork();
      if (pid == -1)
         throw system_error(errno, system_category());
      if (pid == 0) {
         // Wait for parent process to have created entry in editProc (so waiting
         // dependent jobs can be notified that the current job is done).
         kill(getpid(), SIGSTOP);
         execvp(step.com[0], step.com);
         throw system_error(errno, system_category());
      }
      editProc.insert(pair<pid_t,FsNode*> (pid, step.acting));
      // Wait for current step's process to have created and halted itself before
      // continuing.
      int status;
      pid_t caughtpid;
      while ((caughtpid = waitpid(pid, &status, WUNTRACED)) > 0) {
         if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP) {
            kill(pid, SIGCONT);
            break;
         } 
      }
   }
   // Now we are waiting for all jobs to finish, and SIGCHLD handling cannot disrupt
   // the logic.
   sigprocmask(SIG_SETMASK, &oldset, NULL);
   
   // See similar loop above for why error handling is not done with pause.
   while (editProc.size() > 0)
      pause();
}


/* function: operator<<
 * --------------------
 */
ostream& operator<<(ostream& os, const FsTree& ft) {
   if (ft.root == nullptr)
      os << "Empty FsTree\n";
   else
      os << ft.root->toString("");
   return os;
}


/* function: explore
 * -----------------
 */
void FsTree::explore(string rootpath,
                     unordered_multimap<string,FsNode>& fileStore,
                     list<FsNode>& folderStore,
                     FsNode* parent) {
   // Check path valid
   struct stat st;
   if (stat(rootpath.c_str(), &st) != 0)
      throw invalid_argument("Could not locate " + rootpath);
   if (!S_ISDIR(st.st_mode))
      throw invalid_argument(rootpath + " is not a directory.");
   DIR* dir = opendir(rootpath.c_str());
   if (dir == nullptr)
      throw invalid_argument("Need permission to access " + rootpath);

   struct dirent* entry;
   struct stat fst;
   
   while (true) {
      // Get stat for next dir entry
      entry = readdir(dir);
      if (entry == nullptr) break;
      if (! (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) )
            continue;
      string path = rootpath + "/" + entry->d_name;
      if (stat(path.c_str(), &fst) != 0)
         throw system_error(errno, system_category());
      // Add new node to filestore (initialize and hash file)
      FsNode nd;
      nd.size = fst.st_size;
      nd.name = entry->d_name;
      nd.setParent(parent);
      nd.date_changed = fst.st_ctim;
      nd.path = parent->path + "/" + nd.name;
      if (S_ISDIR(fst.st_mode)) {
         nd.type = "dir";
      } else if (S_ISREG(fst.st_mode)) {
         size_t pos = nd.name.find_last_of('.');
         nd.type = (pos == string::npos) ? "other" : nd.name.substr(pos);
      } else if (S_ISLNK(fst.st_mode)) {
         nd.type = "link";
      } else {
         nd.type = "other";
      }
      if (nd.type == "dir") {
         // Recurse on dir contents
         folderStore.push_back(nd);
         FsNode& curNode = folderStore.back();
         parent->children.insert(pair<string, FsNode*> (nd.name, &curNode));
         explore(path, fileStore, folderStore, &curNode); 
      } else {
         // Add file to map with its contents' MD5 hash value as its key.
         parent->num_files++;
         int fd;
         if ( (fd = open(path.c_str(), O_RDONLY)) < 0 )
            throw system_error(errno, system_category());

         unsigned char* fdata = new unsigned char[nd.size];
         if (fdata == nullptr)
            throw system_error(errno, system_category());

         // TODO Add different hash scheme for large files (so faster)?
         unsigned char res[MD5_DIGEST_LENGTH];
         if (read(fd, (void*)fdata, nd.size) < 0)
            throw system_error(errno, system_category());

         MD5(fdata, nd.size, res);

         if (close(fd) < 0)
            throw system_error(errno, system_category());

         delete[] fdata;

         string v;
         for (int i = 0; i < MD5_DIGEST_LENGTH; i++) v += to_string(res[i]);
         unordered_multimap<string,FsNode>::iterator ret =
            fileStore.insert(pair <string,FsNode> (v,nd));

         parent->children.insert(pair<string, FsNode*> (nd.name, &(ret->second)));
      }
   }
   if (closedir(dir) < 0)
      throw system_error(errno, system_category());
}


/* function: traverseSubs
 * ----------------------
 *  Helper for makeFilsHist.
 */
void FsTree::traverseSubs(FsNode* nd, priority_queue<FsNodePtr>& pq) {
   pq.push(FsNodePtr(nd));
   for (FsNode* sub : nd->subordinates)
      traverseSubs(sub, pq);
}


/* function: makeFileHist
 * ----------------------
 *  Helper for constructor with two trees as inputs.
 */
void FsTree::makeFileHist(FsNode* src) {
   // Gather and sort duplicate nodes by recency.
   priority_queue<FsNodePtr> pq;
   traverseSubs(src, pq);

   // Create edit steps for a history directory where most recent duplicate is.
   FsNode* sup = pq.top().n;
   pq.pop();

   plannedNode.push_back(FsNode("." + sup->name + "_hist", sup->dstParent, "dir"));
   FsNode* hist_nd = &(plannedNode.back());
   editSteps.push(EditStep("mkdir", nullptr, hist_nd));
   sup->dstParent->children.insert(pair<string, FsNode*> (hist_nd->name, hist_nd));

   // Create edit steps to copy every older duplicate in the history folder.
   while (!pq.empty()) {
      FsNode* sub_nd = pq.top().n;
      // Not top, so ensure node is removed from children list of original
      // desitnation node.
      sub_nd->dstParent->children.erase(sub_nd->name);
      sub_nd->setDstParent(hist_nd);
      editSteps.push(EditStep("cp", sub_nd, hist_nd));
      hist_nd->children.insert(pair<string, FsNode*> (sub_nd->name, sub_nd));
      pq.pop();
   }
   editSteps.push(EditStep("cp", sup, sup->dstParent));
   sup->dstParent->children.insert(pair<string, FsNode*> (sup->name, sup));
   sup->isSub = false;
}


/* function: mergeDirs
 * -------------------
 * Recursive function which does most of the work of the constuctor with two trees
 * as inputs to merge two directories.
 */
void FsTree::mergeDirs(FsNode* nd1, FsNode* nd2, unordered_set<FsNode*>& sups) {
   // TODO add in different logic to organize too many files (>44) into separate dirs.
   //      by creation date first, then by file type.
   // TODO add special case for dirs when ".[...]_hist" => a tree resulting from unifs.
   unordered_map<string, FsNode*> step_children;
   unordered_set<string> visited;
   // Check children of nd1
   for (unordered_map<string, FsNode*>::iterator ch1 = nd1->children.begin();
        ch1 != nd1->children.end(); ch1++) {
      FsNode* ch1nd = ch1->second;
      // If present in both nodes
      unordered_map<string, FsNode*>::iterator ch2 = nd2->children.find(ch1->first);
      FsNode* ch2nd = nullptr;
      if (ch2 != nd2->children.end()) {
         ch2nd = ch2->second;
         visited.insert(ch1->first);
      }

      // If directory
      if (ch1nd->type == "dir") {
         // Add edit step to create dir.
         if (ch2nd == nullptr) {
            // Create container for nd1 contents. Will recurse on container and
            // nd1.
            ch2nd = ch1nd;
            plannedNode.push_back(FsNode(ch1nd->name, nd1, "dir"));
            ch1nd = &(plannedNode.back());
         }
         ch1nd->setParent(nd1);
         ch2nd->setParent(nd1);
         editSteps.push(EditStep("mkdir", nullptr, ch1nd));
         step_children.insert(pair<string, FsNode*> (ch1nd->name, ch1nd));
         // Recurse
         mergeDirs(ch1nd, ch2nd, sups);
      } else { // If file
            ch1nd->setDstParent(nd1); // Destination folder for file
         if (ch2nd != nullptr){ // Filename exists in both nodes
            // Either ch2nd or ch1nd could be most recent of duplicates
            ch2nd->setDstParent(nd1);
            if (!(ch1nd->isSub || ch2nd->isSub))
               ch1nd->makeSub(ch2nd); // Subordinate one duplicate to the other
            // Add either's topSup to sups and their duplicate tree will be processed
            FsNode* sub = (ch1nd->isSub) ? ch1nd : ch2nd;
            FsNode* not_sub = (sub == ch1nd) ? ch2nd : ch1nd;
            sups.insert(sub->topSup);
            if (sub->topSup != not_sub) // Ensure no subordinate loops form
               not_sub->makeSub(sub);
         } else if (!ch1nd->isSub && ch1nd->subordinates.empty()) { // Not a duplicate
           editSteps.push(EditStep("cp", ch1nd, nd1)); 
           step_children.insert(pair<string, FsNode*> (ch1nd->name, ch1nd));
         } else if (ch1nd->isSub) {
            sups.insert(ch1nd->topSup);
         }
      }
   }
   
   for (unordered_map<string, FsNode*>::iterator ch2 = nd2->children.begin();
        ch2 != nd2->children.end(); ch2++) {
      if (visited.find(ch2->first) == visited.end()) {
         if (ch2->second->type == "dir") { // If dir, similar to above
            FsNode* ch2nd = ch2->second;
            ch2nd->setParent(nd1);
            plannedNode.push_back(FsNode(ch2nd->name, nd1, "dir"));
            FsNode* ch1nd = &(plannedNode.back());
            editSteps.push(EditStep("mkdir", nullptr, ch1nd));
            step_children.insert(pair<string, FsNode*> (ch1nd->name, ch1nd));
            // Recurse
            mergeDirs(ch1nd, ch2nd, sups);
         } else { // If file, similar to subcases above (without collision case)
            ch2->second->setDstParent(nd1);
            if (!ch2->second->isSub && ch2->second->subordinates.empty()) {
               step_children.insert(pair<string, FsNode*> (ch2->first, ch2->second));
               editSteps.push(EditStep("cp", ch2->second, nd1)); 
            } else if (ch2->second->isSub) {
               sups.insert(ch2->second->topSup);
            }
         }
      }
   }
   nd1->children = step_children;
}


/* function: getNextStep
 * ---------------------
 * Helper for execTform.
 */
bool FsTree::getNextStep(EditStep& step) {
   // If there are no jobs in the queue, fetch a new one from the editSteps queue.
   if (jobs.empty()) {
      while (!editSteps.empty()) {
         step = editSteps.front();
         editSteps.pop();
         if (step.acting->parent != nullptr) { // Only root has null parent.
            FsNode* ascendantNode = (step.op == "mkdir") ?
               step.acting->parent : step.acting->dstParent;
            if (!ascendantNode->is_created) { // Queue step and draw another.
               editQueue.insert(pair<FsNode*, EditStep> (ascendantNode, step));
               continue;
            }
         }
         return true;
      }
   } else {
      step = jobs.front();
      jobs.pop();
      return true;
   }
   return false;
}
