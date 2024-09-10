# p05c-rpi-gpu
## High Performance Multi-Stream Camera System for Raspberry Pi

## Project Info

#### Team: P05C

#### University/Canvas Name: COMP3888_M12_02

#### Tutor: Lin Zhang \[[lzha8455@uni.sydney.edu.au](mailto:lzha8455@uni.sydney.edu.au)\]

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

<br>

![](InsiteProjectSolutions.png) 

<br>

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
1. Network Connection

Ensure your Raspberry Pi is connected to a network (could be home wifi or mobile hotspot), and that the computer you're using is connected to the same network.


2. Obtaining RaspberryPi Local IP Address

**The following method does NOT require an external monitor connected to the RPi**

- Provided your Raspberry Pi and Computer are on the same Wifi Network, open up your computer terminal/command prompt and run:
```sh
ping <raspberrypi>.local
```
Here, \<raspberrypi\> represents the name of the Raspberry Pi (NOT your username).

If that does not work, try without the ".local":
```sh
ping <raspberrypi>
```
If neither work, there is most likely an issue with your Wifi connections. Double check that both your Raspberry Pi and Computer are connected to the **SAME NETWORK**.

<br>

**Note: The following methods require an external monitor connected to the RPi**

- Boot up the Raspberry Pi and open terminal (RPi) and run:    
     ```sh
     hostname -I
     ```
     This will output your RPi's Local IP address.
     <br></br>
- Alternatively, Boot up the Raspberry Pi and open terminal (RPi) and run:
     ```sh
     nmcli device show
     ```
     Look for IP4.ADDRESS[1]. IP address is the numbers before /24. use the key 'q' to exit the code
     <br></br>

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

## Build Instructions

### libcamera Build:
```sh
sudo apt install libboost-dev libgnutls28-dev openssl libtiff-dev pybind11-dev qtbase5-dev libqt5core5a libqt5widgets5 meson cmake python3-yaml python3-ply libglib2.0-dev libgstreamer-plugins-base1.0-dev

cd libcamera
# or wherever you cloned the libcamera repo

meson setup build --buildtype=release -Dpipelines=rpi/vc4,rpi/pisp -Dipas=rpi/vc4,rpi/pisp -Dv4l2=true -Dgstreamer=enabled -Dtest=false -Dlc-compliance=disabled -Dcam=disabled -Dqcam=disabled -Ddocumentation=disabled -Dpycamera=enabled

sudo ninja -C build install
```

### rpicam-apps Build:
```sh
sudo apt install libboost-program-options-dev libdrm-dev libexif-dev meson ninja-build libavcodec-dev libavdevice-dev libpng-dev libepoxy-dev

cd rpicam-apps
# or wherever you cloned rpicam-apps

meson setup build -Denable_libav=enabled -Denable_drm=enabled -Denable_egl=enabled -Denable_qt=enabled -Denable_opencv=disabled -Denable_tflite=disabled

meson compile -C build

sudo meson install -C build

sudo ldconfig
```

### rpicam-apps Rebuild (with your own New Build):
```sh
cd rpicam-apps
# or wherever you cloned rpicam-apps

rm -rf build

meson setup build -Denable_libav=enabled -Denable_drm=enabled -Denable_egl=enabled -Denable_qt=enabled -Denable_opencv=disabled -Denable_tflite=disabled

meson compile -C build

sudo meson install -C build

sudo ldconfig
```

### rpicam-apps Rebuild (Makefile Alternative):
```sh
cd rpicam-apps
make
```
<br>

## Code Review Instructions

### Two Preview Windows:

<br>

This version of the program was a proof-of-concept task given to us by Cian. We were to see if we could have two preview windows displaying at once. We were partially successful, as detailed below.

In order to navigate to the intended branch and directory to test  this code, please follow these instructions:

```sh
git checkout patrick-preview-test
cd rpicam-apps
make
```

then, on a Raspi environment (window won't display through an SSH), run

```sh
rpicam-patrick --verbose
```

This will open two preview windows, and feed both with the camera input. However, the program will crash after displaying only one frame. I have added a 2 second sleep statement so that the preview windows with the single frame will be visible.

<br><br>

### Image Capture Speedup [rpicam-still]:

<br>

*Note: this was the **initial** attempt.*

These edits were an initial alternative approach to the task of improving the speed of taking a Raspberry Pi photo, modifying the rpicam_still files instead of the rpicam_jpeg files.

The modifications resulted in a time decrease from around 10 seconds to around 5 seconds, which is not as efficient as the result of the rpicam_jpeg changes that the team decided to pursue further. If you still want to test this out this alternative approach, complete the following instructions:

```sh
git checkout eleanor_photo_edits
cd rpicam-apps
make
```

Type this command into your Rpi terminal to take a photo, and the time it took to take the photo should be printed in the terminal:

```sh
libcamera-still --width 3280 --height 2464 -o test.jpg -n -t 1
```

<br><br>

### Image Capture Speedup [rpicam-jpeg]:

<br>

*Note: this is the faster version.*

These edits pertain to the rpicam_jpeg.cpp file, and by restructuring the camera configuration and removing redundant code, image capture has been reduced from roughly 5.5 seconds to **0.6** seconds.

In order to navigate to the intended branch and directory to test this code, please follow these instructions:

```sh
git checkout alistair_branch
cd rpicam-apps
make build
```

You should now be able to execute image capture commands with significantly reduced execution time.

For a basic image capture:

```sh
libcamera-jpeg -o test.jpg
```

For performance testing of image capture, navigate to the tests/scripts directory and then execute benchmark_libcamera.sh by the following:

```sh
cd tests/scripts
chmod +x benchmark_libcamera.sh
./benchmark_libcamera.sh
```

This will run the image capture command 20 times, outputting the execution time in milliseconds to data/timings.txt in real-time. You can compare this with the old_timings.txt in /data, which is from the same testcase run on the old version of rpicam_jpeg to see the difference (90% reduction).

To see a benchmark plot of the two versions of rpicam_jpeg, run analyse_timings.py:

```sh
python analyse_timings.py
```
This testcase assesses the performance of the new rpicam_jpeg, ensuring a consistent reduction in execution time. The produced plot is located in /data. 

<br>

### External App [danBranch]:

<br>

```sh
git checkout danBranch
cd rpicam-apps
make
```

<br>

### External App [andrei-new-app]:

<br>

```sh
git checkout andrei-new-app
cd andrei-libcamera/src
g++ andrei-libcamera.cpp -I/usr/local/include/libcamera -lcamera
```