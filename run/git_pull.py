# This file git pulls all submodules and root module
#

import tac_utils
import git
import threading

tac_utils.SetLogLevel( 'INFO' )

fileData = tac_utils.BeginFile(__file__)

root_repo = git.Repo(tac_utils.tac_root)
sub_repos = [sm.module() for sm in root_repo.submodules if sm.exists() and sm.module_exists()]
all_repos = sub_repos + [root_repo]

all_repo_names = [repo.remotes.origin.url for repo in all_repos]
max_name_length = max(len(name) for name in all_repo_names)
all_repo_names = list(name.ljust(max_name_length) for name in all_repo_names)

repo_count = len(all_repos)
thread_results = [None] * repo_count
threads = [None] * repo_count

def ThreadFn(i):
  thread_results[i] = all_repos[i].git.pull()

for i in range(repo_count):
  threads[i] = threading.Thread(target = ThreadFn, args=(i,))
  threads[i].start()

for thread in threads:
  thread.join()

for name, out in zip(all_repo_names, thread_results):
  print(f"git pull {name} - {out}")


tac_utils.EndFile(fileData)

