# This file commits and pushes the root git directory and all submodules
#

import tac_utils
import logging
import git


fileData = tac_utils.BeginFile(__file__)

root_repo       = git.Repo(tac_utils.tac_root)
change_lines    = []
repos_to_commit = []

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



for sm in root_repo.submodules:

    if sm.exists() and sm.module_exists():
        repo = sm.module()
        VisitRepo(repo)


VisitRepo(root_repo)


commit_msg = ""

def CommitAndPush( repo ):
    try:
        print( f'commit and pushing to {repo.remotes.origin.url}')

        # Same as git commit -m commit_msg
        repo.index.commit(commit_msg) 

        origin = repo.remote(name='origin')

        # Same as git push
        origin.push() 

    except Exception as e:
        print(e) 

if len( repos_to_commit ) != 0:

    # Print changed lines so the user knows what commit msg to write
    for line in change_lines:
        print(line)

    print("Please enter commit msg: ")
    commit_msg = input()
    if len(commit_msg) == 0:
        commit_msg = "asdf"

    for repo in repos_to_commit:
        if repo != root_repo:
          CommitAndPush( repo )

# Revisit the root to see if we can pickup the sumbodule change
VisitRepo(root_repo)

if root_repo in repos_to_commit:
  CommitAndPush( root_repo )

tac_utils.EndFile(fileData)
