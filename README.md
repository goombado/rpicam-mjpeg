# p05c-rpi-gpu - High Performance Multi-Stream Camera System for Raspberry Pi

## Project Info

#### Team: P05C

#### University/Canvas Name: COMP3888_M12_02

#### Tutor: Lin Zhang [lzha8455@uni.sydney.edu.au](mailto:lzha8455@uni.sydney.edu.au)

### Team Members

| Name               | SID       | Unikey   | Email                      |
| ------------------ | --------- | -------- | -------------------------- |
| Andrei Agnew       | 510471659 | aagn5279 | aagn5279@uni.sydney.edu.au |
| Alistair MacDonald | 510465636 | amac2328 | amac2328@uni.sydney.edu.au |
| Daniel Miu         | 510423924 | dmiu7957 | dmiu7957@uni.sydney.edu.au |
| Eleanor Taylor     | 540908525 | etay0703 | etay0703@uni.sydney.edu.au |
| Grant Dong         | 510472058 | gdon7480 | gdon7480@uni.sydney.edu.au |
| Patrick Sparks     | 440181875 | pspa2200 | pspa2200@uni.sydney.edu.au |

### Client Information

| Name           | Email                       |
| -------------- | --------------------------- |
| Cian Byrne     | cian.byrne@coliemore.com.au |
| Kitt Morjanova | kitt@mailington.com         |

### Documentation

Our Documentation in Confluence can be found at [https://comp3888-m12-02-2024.atlassian.net/wiki/spaces/COMP3888M1/overview](https://comp3888-m12-02-2024.atlassian.net/wiki/spaces/COMP3888M1/overview)

## Setup

1.   Clone the repository and enter the directory. Ensure you have the `--recurse-submodules` flag so that `libcamera-async` is also cloned.

     ```sh
     git clone --recurse-submodules https://github.com/consiliumsolutions/p05c-rpi-gpu
     cd p05c-rpi-gpu
     ```

2.   Change your git name and set your email to your university email.

     ```sh
     git config user.name "Full Name SID"
     git config user.email "unikey@uni.sydney.edu.au"
     ```

## Making changes

**IMPORTANT: IF MAKING CHANGES TO `libcamera-async`, YOU MUST DO THAT IN THE REPOSITORY ITSELF: [https://github.com/COMP3888-M12-02-2024/libcamera-async](https://github.com/COMP3888-M12-02-2024/libcamera-async)**

Before making any new changes, create a new branch and fetch any changes that have been made to the libcamera submodule. Importantly, you must update the submodule **after** you have checked out your new branch, and `git pull` **before** checking out.

```sh
git pull
git checkout -b branch_name
git submodule update --remote --merge
```

After doing some work, make sure to add, commit and push.

```sh
git add .
git commit -m "short description"
git push origin branch_name
```

Once the branch is ready to be merged, open a new pull request in the [Pull Requests](https://github.com/consiliumsolutions/p05c-rpi-gpu/pulls) tab. Each branch needs at least one team member to review the code, and when this is done the branch can be merged into the main branch.

