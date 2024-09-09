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

## SSH into the Raspberry Pi
1. Ensure your Raspberry Pi is connected to a network (could be home wifi or mobile hotspot), and that the computer you're using is connected to the same network
2. Get your Raspberry Pi IP address by doing one of the following:
     - Boot up the Raspberry Pi and open terminal. Run
     ```sh
     hostname -I
     ```
     and you will get your Pi's IP address
     - Same as above but instead Run 
     ```sh
     nmcli device show
     ```
     Look for IP4.ADDRESS[1]. IP address is the numbers before /24. use the key 'q' to exit the code
3. On the computer you want to ssh into the raspberry pi, run 
```sh
ssh <username>@<IP address>
```
where the username is the username that you setup when you flashed your OS, and IP address is the address of the Raspberry Pi. Then you will be prompted to enter your password and then boom, you have successfully connected to the Pi remotedly. You should see <username>@<RaspberryPi> on the left of your terminal lines.

### USES
The above gives you a terminal so you can run code, however if you want to write code you can do the following:

1. On VS code, install the extension called remote development
2. Bring up the command palette and search "Remote SSH: Connect current window to host"
3. Add new host and type in the above ssh command, ie:
```sh
ssh <username>@<IP address>
```
4. It'll prompt you where to store the config file, just continue here doesn't really matter where you're storing it. Then it'll prompt you to entire your password, and if successful a new VS code window will open and now you can code on the Raspberry pi remotely.

### Side notes
For our purposes, we want to do something called X forwarding, on your terminal when you ssh, add the -X or -Y flag (will look into the differences), and when you run for example rpicam-hello you want to add the "--qt-preview" flag.

### LIBCAMERA BUILD:
sudo apt install libboost-dev libgnutls28-dev openssl libtiff-dev pybind11-dev qtbase5-dev libqt5core5a libqt5widgets5 meson cmake python3-yaml python3-ply libglib2.0-dev libgstreamer-plugins-base1.0-dev
cd libcamera
or wherever you cloned the libcamera repo
meson setup build --buildtype=release -Dpipelines=rpi/vc4,rpi/pisp -Dipas=rpi/vc4,rpi/pisp -Dv4l2=true -Dgstreamer=enabled -Dtest=false -Dlc-compliance=disabled -Dcam=disabled -Dqcam=disabled -Ddocumentation=disabled -Dpycamera=enabled
sudo ninja -C build install

### RPICAM-APPS BUILD:
sudo apt install libboost-program-options-dev libdrm-dev libexif-dev meson ninja-build libavcodec-dev libavdevice-dev libpng-dev libepoxy-dev
cd rpicam-apps
or wherever you cloned rpicam-apps
meson setup build -Denable_libav=enabled -Denable_drm=enabled -Denable_egl=enabled -Denable_qt=enabled -Denable_opencv=disabled -Denable_tflite=disabled
meson compile -C build
sudo meson install -C build
sudo ldconfig

### IF YOU WANT TO REBUILD RPICAM-APPS WITH YOUR OWN NEW BUILD
cd rpicam-apps
rm -rf build
meson setup build -Denable_libav=enabled -Denable_drm=enabled -Denable_egl=enabled -Denable_qt=enabled -Denable_opencv=disabled -Denable_tflite=disabled
meson compile -C build
sudo meson install -C build
sudo ldconfig

### MAKEFILE ALTERNATIVE TO REBUILD RPICAM-APPS
cd rpicam-apps
make

### Code Review Instructions

1. Two preview windows:

This version of the program was a proof-of-concept task given to us by Cian. We were to see if we could have two preview windows displaying at once. We were partially successful, as detailed below.

In order to navigate to the intended branch and directory to test  this code, please follow these instructions:

git checkout patrick-preview-test
cd rpicam-apps
make

then, on a Raspi environment (window won't display through an SSH), run

rpicam-patrick --verbose

This will open two preview windows, and feed both with the camera input. However, the program will crash after displaying only one frame. I have added a 2 second sleep statement so that the preview windows with the single frame will be visible.

2. rpicam_still fast image capture (initial approach):

These edits were an initial alternative approach to the task of improving the speed of taking a Raspberry Pi photo, modifying the rpicam_still files instead of the rpicam_jpeg files.

The modifications resulted in a time decrease from around 10 seconds to around 5 seconds, which is not as efficient as the result of the rpicam_jpeg changes that the team decided to pursue further. If you still want to test this out this alternative approach, complete the following instructions:

git checkout eleanor_photo_edits
cd rpicam-apps
make

Type this command into your Rpi terminal to take a photo, and the time it took to take the photo should be printed in the terminal:

libcamera-still --width 3280 --height 2464 -o test.jpg -n -t 1

***the right command might also be libcamera-still --width 3280 --height 2464 -o test.jpg (without the -n-t 1) but I don’t remember for sure

3. rpicam_jpeg Image Capture Speedup (faster version):

These edits pertain to the rpicam_jpeg.cpp file, and by restructuring the camera configuration and removing redundant code, image capture has been reduced from roughly 5.5 seconds to 0.6 seconds.

In order to navigate to the intended branch and directory to test  this code, please follow these instructions:

git checkout alistair_branch
cd rpicam-apps
make build

You should now be able to execute image capture commands with significantly reduced execution time.

For a basic image capture:

libcamera-jpeg -o test.jpg

4. daniel's branch

git checkout danBranch
cd rpicam-apps
make