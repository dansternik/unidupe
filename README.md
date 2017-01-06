# unidupe
Merge folders, unifying duplicate files (by path or by content) in Linux.

## Usage:
```unidupe pathin1 pathin2 pathout```
## Description
If your files generated over the years are spread and duplicated over multiple machines, OS, and drives, unidupe is a good start. Merge two folders that contain similar structures (eg: home directories) and loads of duplicates (same files with different names, or same path but different files). Files will be preserved: the merged folder will contain copies, not moves of your files. The most recent duplicate file will be preserved and in its folder, a "history" will be created. "History" refers to a hidden folder containing all identified duplicates. Runs in linux terminal.
