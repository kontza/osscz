# TL;DR
1. Commit code changes.
2. Add tag:
  ```sh
  git tag v1.2.0
  ```
3. Push code and tag in one go:
  ```sh
  git ps --atomic origin main v1.2.0
  ```
