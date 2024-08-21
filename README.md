# p05c-rpi-gpu

## Setup

1.   Clone the repository and enter the directory. Ensure you have the `--recurse-submodules` flag so that `libcamera-async` is also cloned.

     ```sh
     git clone --recurse-submodules https://github.com/consiliumsolutions/p05c-rpi-gpu
     cd p05c-rpi-gpu
     ```

2.   Change your git name and set your email to your university email.

     ```sh
     git config user.name "Full Name"
     git config user.email "unikey@uni.sydney.edu.au"
     ```

3.   Add an option in your git config so that on each `git pull`, you also fetch the latest changes from the `libcamera-async` submobule.
     ```sh
     git config submodule.recurse true
     ```

## Making changes

Ensure that you checkout a new branch before starting any work

```sh
git checkout -b branch_name
...
git add .
git commit -am "short description"
git push -u origin branch_name
```

Once the branch is ready to be merged, open a new pull request in the [Pull Requests](https://github.com/consiliumsolutions/p05c-rpi-gpu/pulls) tab. Each branch needs at least one team member to review the code, and when this is done the branch can be merged into the main branch.
