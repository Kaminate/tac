# This file does ...
#

import tac_utils

import os
import logging
import git


tac_utils.BeginFile(__file__)

print("hi")
repo = git.Repo(tac_utils.tac_root)

# untracked_files = repo.untracked_files
# modified_files = repo.index.diff(None)

# num_untracked = len(untracked_files)
# num_modified = len(modified_files)

# for d in diffs:
#     print(d.a_path)

commit_msg = ""

# def GetCommitMsg():
#   print("Please enter commit msg: ")
#   commit_msg = input()

def VisitRepo( repo ):
    global commit_msg

    # untracked_files = repo.untracked_files
    # diffs = repo.index.diff(None)

    # num_untracked = len(untracked_files)
    # num_modified = len(modified_files)

    # for file in untracked_files:
        # print(d.a_path)

    # type is https://gitpython.readthedocs.io/en/stable/reference.html#git.diff.Diff
    # for diff in diffs:
        # print(diff.a_path)

    # repo_working_dir_fullpath = repo.working_dir
    # repo_remote_url = repo.remotes[0].config_reader.get("url")

    # repo.git.add(update=True)
    # repo.index.add('*')

    # repo.git.add(update=True)

    # if repo.is_dirty(untracked_files=True):



    if repo.is_dirty():
        print('repo is dirty')

        modified_files = [item.a_path for item in repo.index.diff(None)]
        print(modified_files)

        if modified_files:
            # Add only modified files to the staging area
            repo.index.add(modified_files)

            # print('modified files begin')
            # for file in modified_files:
            #     print( file )
            # print('modified files end')

    if repo.untracked_files:
        print('repo has untracked files')

        repo.index.add(repo.untracked_files)

        # print('untracked files begin')
        # for file in repo.untracked_files:
        #     print( file )
        # print('untracked files end')

    # staged_files = [entry.path for entry in repo.index.entries]
    # staged_files = [blob.path for blob in repo.index.iter_blobs()]
    staged_files = repo.index.diff("HEAD")
    if staged_files:
        print( "staged files begin")
        for file in staged_files:
            print( file.a_path)
        # print( "staged files:", staged_files)
        print( "staged files end")


    index_diff = repo.index.diff(None)

    if index_diff:
        # There are changes in the index to commit
        print("Changes in the index to commit:")
        
        for change in index_diff:
            strType = f"Change Type: {change.change_type}"
            strStatus = f"Change Status: {change.change_type_flags}"
            strPath = f"File Path: {change.a_path}"
            print(strtype, strStatus, strPath)


        print("Please enter commit msg: ")
        commit_msg = input()


        # diffs = repo.index.diff(None)
        # if len(diffs) == 0:
        #   print('no diffs')
        # else:
        #   print('begin diffs')
        #   for diff in diffs :
        #       print( diff.a_path )
        #   print('end diff')


        # repo.index.commit(commit_msg)
        # remote = repo.remote(name='origin')
        # remote.push()

###

# repo.index.commit(commit_msg)

#          | name of remote repositiory you are connected to
#          v 
# git push origin master

# repo.submodules
#   https://gitpython.readthedocs.io/en/stable/reference.html#git.repo.base.Repo.submodules
for sm in repo.submodules:

    # so sm is probably
    #     https://gitpython.readthedocs.io/en/stable/reference.html#module-git.objects.submodule.base


    if not sm.exists():
        continue

    if not sm.module_exists():
        continue

    print( sm.name )
    m = sm.module() # type Repo

    VisitRepo( m );
    break # temp, only test for 1st sm

    # sm.branch_name
    # if sm.module_exists():
    #   m = sm.module() # of type Repo
    
    # assert sm.module_exists()  # the module is available, which doesn't have to be the case.
    # assert sm.module().working_tree_dir.endswith("gitdb")
    

tac_utils.EndFile(__file__)
