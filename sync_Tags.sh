#! /bin/bash
cd $REPO/.git
git tag -l | xargs git tag -d
git fetch origin --prune
git fetch origin --tags
