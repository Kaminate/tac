# This file does ...
#

import tac_utils

import os
import logging
import git


tac_utils.BeginFile(__file__)

print("hi")
root_repo = git.Repo(tac_utils.tac_root)

change_lines = []
repos_to_commit = []

def PrintRepoPath( repo ):
    print( f'{repo.working_tree_dir} {repo.remotes.origin.url}' )

###


#          | name of remote repositiory you are connected to
#          v 
# git push origin master

# repo.submodules
#   https://gitpython.readthedocs.io/en/stable/reference.html#git.repo.base.Repo.submodules



def VisitRepo(repo):

    global change_lines
    global repos_to_commit

    # Stage all changes
    repo.git.add(all=True) 

    diffs = repo.head.commit.diff()
    if diffs:

        # Add this repo as one of the repos to commit to
        repos_to_commit.append( repo )


        # Append to list of changes
        for diff in diffs:
            strs = [f'{diff.change_type} {diff.a_path}']
            if diff.a_path != diff.b_path:
                strs.extend (['-->', diff.b_path ])
            change_lines.append(' '.join(strs))



# ONLY_DO_FIRST_SUBMODULE = True

for sm in root_repo.submodules:

    if sm.exists() and sm.module_exists():
        repo = sm.module()
        VisitRepo(repo)

    # if ONLY_DO_FIRST_SUBMODULE:
        # break

# if not ONLY_DO_FIRST_SUBMODULE:
VisitRepo(root_repo)


if len( repos_to_commit ) != 0:

    # Print changed lines so the user knows what commit msg to write
    for line in change_lines:
        print(line)

    print("Please enter commit msg: ")
    commit_msg = input()
    if len(commit_msg) == 0:
        commit_msg = "asdf"

    for repo in repos_to_commit:
        print( f'pushing to {repo.remotes.origin.url}')
        try:
            # Same as git commit -m commit_msg
            repo.index.commit(commit_msg) 

            origin = repo.remote(name='origin')

            # Same as git push
            origin.push() 

        except Exception as e:
            print(e) 

tac_utils.EndFile(__file__)
