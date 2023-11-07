# This file git pulls all submodules and root module
#

import tac_utils
import git


tac_utils.BeginFile(__file__)


root_repo = git.Repo(tac_utils.tac_root)

sub_repos = [sm.module() for sm in root_repo.submodules if sm.exists() and sm.module_exists()]
all_repos = sub_repos + [root_repo]
all_repo_names = [repo.remotes.origin.url for repo in all_repos]
max_name_length = max(len(name) for name in all_repo_names)
all_repo_names_padded = (name.ljust(max_name_length) for name in all_repo_names)
        
for repo, name in zip(all_repos, all_repo_names_padded):
    out = repo.git.pull()
    print(f"git pull {name} - {out}")



tac_utils.EndFile(__file__)
