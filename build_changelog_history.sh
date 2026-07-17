#!/bin/bash

{
    echo "===== CURRENT BRANCH ====="
    git branch --show-current

    echo
    echo "===== TAGS ====="
    git tag --sort=creatordate

    echo
    echo "===== BRANCHES ====="
    git branch --all

    echo
    echo "===== COMMIT GRAPH ====="
    git log --all --graph --decorate --date=short \
      --pretty=format:'%h %ad %d %s'

    echo
    echo
    echo "===== DETAILED COMMITS ====="
    git log --all --date=short --decorate --stat \
      --pretty=format:'%n---%ncommit %H%nshort %h%ndate %ad%nauthor %an <%ae>%ndecorations %d%nsubject %s%nbody %b'

    echo
    echo
    echo "===== FILE CHANGES BY COMMIT ====="
    git log --all --date=short --name-status \
      --pretty=format:'%n---%n%h %ad %s'

    echo
    echo "===== RECENT PRS ====="
    gh pr list --state all --limit 100 \
      --json number,title,state,mergedAt,createdAt,author,headRefName,baseRefName,url

    echo
    echo "===== RECENT ISSUES ====="
    gh issue list --state all --limit 100 \
      --json number,title,state,createdAt,closedAt,author,url,labels
  } > changelog_source.txt

