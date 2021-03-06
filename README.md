# Bridge12 - Support for Bridge12 ODNP spectrometer

This repository is used in conjunction with the **OpenVnmrJ** and **ovjTools** repositories to build a package to support the Bridge12 ODNP spectrometer. This package must be built on an Ubuntu 18 system. Network access will be required. The following instructions may be used.

1. If git does not exist on the computer, install it.
```
    sudo apt install git
```

2. Create a directroy to hold all build-related files
```
    cd ~
    mkdir ovjbuild
    cd ovjbuild
```
3. Get the Bridge12 repository.
```
    git clone https://github.com/DanIverson/Bridge12.git
```

4. Install Ubuntu packages required for the build process.
```
    cd Bridge12/bin
    ./toolChain
```

5. Build the package. As a first step, the OpenVnmrJ and ovjTools repositories will automatically be downloaded. The build processes takes about 10 minutes.
```
    ./buildb12
```
6. Success of the build process can be checked with the ```whatsin``` script.
```
    ./whatsin <path_to_log_file>
```

7. Optional step to package OpenVnmrJ for distribution. The actual name of the zip file is selectable.
```
    cd ~/ovjbuild
    zip -ry OpenVnmrJ_B12.zip dvdimageB12
```

8. Install OpenVnmrJ
```
    cd ~/ovjbuild/dvdimageB12
    ./load.nmr
```

9. Run OpenVnmrJ. You will probably need to logout and then login again. You can double-click the OpenVnmrJ Desktop icon and select "Trust and Launch". Alternatively, you can open a terminal window and enter
```
    vnmrj
```
10. After OpenVnmrJ starts, to configure the spectrometer, in the OpenVnmrJ command line, enter
```
    config
````




