# Bridge12 - Support for Bridge12 ODNP spectrometer

This repository is used in conjunction with the **OpenVnmrJ** and **ovjTools** repositories to build a package to support the Bridge12 ODNP spectrometer. This package can be built on Ubuntu 18 or newer systems. Network access will be required. The following instructions may be used.

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

5. Build the package. As a first step, the OpenVnmrJ and ovjTools repositories will automatically be downloaded.
```
    ./buildb12
```
6. Success of the build process can be checked with the ```whatsin``` script.
```
    ./whatsin <path_to_log_file>
```

7. Optional step to package OpenVnmrJ for distribution. The actual name of the zip file is selectable.
```
    cd ../..
    zip -ry OpenVnmrJ_B12.zip dvdimageB12
```

8. Install OpenVnmrJ
```
    cd ~/ovjbuild/dvdimageB12
    ./load.nmr
```




