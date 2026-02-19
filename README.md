# Glass
Glass is a fork of the Plasma 6 blur effect with additional features and bug fixes.

![Screenshot](/docs/glass.png)
![Did someone say liquid?](/docs/liquid_enough.png)

### 4v3ngR's changes
- Small change to blur.cpp and the shaders to have the corner roundness setting influence the refraction
- modified refraction shader to produce a better convex effect
- refraction now follows top and bottom corner radius settings
- on screen display windows (eg volume control) have top and bottom corners matching top corner radius setting
- remove edge texture option (flip/clamp) for the setting didn't appear to do anything with the new shader
- better support for plasma 6.5
- all console windows (settings etc) are blurred
- context menus are blurred


### 4v3ngR's theme patches
- Updated firefox, thunderbird, plasma, color schemes, and helper scripts can be found at my [glassOS repo](https://github.com/4v3ngR/glassOS)

### 4v3ngR's application and WM theme
- [Glass](https://github.com/4v3ngR/Glass) is a fork of Darkly to complement the improved refraction shader for that liquid refraction goodness.
  
### Features
- X11 and Wayland support
- Force blur
- Rounded corners with anti-aliasing
- Static blur for much lower GPU usage
- Adjust blur brightness, contrast and saturation
- Refraction (by [@DaddelZeit](https://github.com/DaddelZeit) and [@iGerman00](https://github.com/iGerman00))
  
### Support for previous Plasma releases
Currently supported versions: **6.6**, **6.5**
- big thanks to [@dnmodder](https://github.com/dnmodder) for providing the patch for 6.6.0

# Installation
> [!IMPORTANT]
> If the effect stops working after a system upgrade, you will need to rebuild it or reinstall the package.

## Packages
<details>
  <summary>NixOS (flakes)</summary>
  <br>

  ``flake.nix``:
  ```nix
    {
      inputs = {
        # nixpkgs repository
        nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable"

        # kwin-effects-glass flake module
        kwin-effects-glass = {
          url = "github:4v3ngR/kwin-effects-glass";
          inputs.nixpkgs.follows = "nixpkgs";
        };
      };
    }
  ```

  ```nix
    { inputs, pkgs, ... }:
    
    {
      # alternatively, put the attributes in the list into
      # 'users.users.<name>.packages' or 'home.packages' if
      # using home-manager
      environment.systemPackages = [
        inputs.kwin-effects-glass.packages.${pkgs.system}.default # for KDE Wayland
        inputs.kwin-effects-glass.packages.${pkgs.system}.x11 # for KDE X11
      ];
    }
  ```
</details>

<details>
  <summary>Arch (AUR)</summary>
  <br>
  
  ```sh
  yay -S kwin-effects-glass-git
  ```
  
  Thanks to [Avi Tretiak](https://github.com/avitretiak) [package details](https://aur.archlinux.org/packages/kwin-effects-glass-git)
</details>

<details>
  <summary>Fedora 43, 42 (copr)</summary>
  <br>
  
  ```sh
  sudo dnf copr enable ama1470/kwin-effects-glass
  sudo dnf install kwin-effects-glass
  ```
  
  > WARNING
  > This copr repo is maintained by [AMA147000](https://github.com/AMA147000) rather than the upstream developer and may break on changes. For packaging/updating error please open tickets on the [packaging repo](https://github.com/AMA147000/kwin-effects-glass-packaging) instead of this one.
</details>

## Manual
> [!NOTE]
> On Fedora Kinoite and other distributions based on it, the effect must be built in a container.

### Dependencies
- CMake
- Extra CMake Modules
- Plasma 6
- Qt 6
- KF6
- KWin development packages

<details>
  <summary>Arch Linux</summary>
  <br>

  Wayland:
  ```
  sudo pacman -S base-devel git extra-cmake-modules qt6-tools kwin
  ```
  
  X11:
  ```
  sudo pacman -S base-devel git extra-cmake-modules qt6-tools kwin-x11
  ```
</details>

<details>
  <summary>Debian-based (KDE Neon, Kubuntu, Ubuntu)</summary>
  <br>

  Wayland:
  ```
  sudo apt install -y git cmake g++ extra-cmake-modules qt6-tools-dev kwin-dev libkf6configwidgets-dev gettext libkf6crash-dev libkf6globalaccel-dev libkf6kio-dev libkf6service-dev libkf6notifications-dev libkf6kcmutils-dev libkdecorations3-dev libxcb-composite0-dev libxcb-randr0-dev libxcb-shm0-dev
  ```
  
  X11:
  ```
  sudo apt install -y git cmake g++ extra-cmake-modules qt6-tools-dev kwin-x11-dev libkf6configwidgets-dev gettext libkf6crash-dev libkf6globalaccel-dev libkf6kio-dev libkf6service-dev libkf6notifications-dev libkf6kcmutils-dev libkdecorations3-dev libxcb-composite0-dev libxcb-randr0-dev libxcb-shm0-dev
  ```
</details>

<details>
  <summary>Fedora 41, 42</summary>
  <br>

  Wayland:
  ```
  sudo dnf -y install git cmake extra-cmake-modules gcc-g++ kf6-kwindowsystem-devel plasma-workspace-devel libplasma-devel qt6-qtbase-private-devel qt6-qtbase-devel cmake kwin-devel extra-cmake-modules kwin-devel kf6-knotifications-devel kf6-kio-devel kf6-kcrash-devel kf6-ki18n-devel kf6-kguiaddons-devel libepoxy-devel kf6-kglobalaccel-devel kf6-kcmutils-devel kf6-kconfigwidgets-devel kf6-kdeclarative-devel kdecoration-devel kf6-kglobalaccel kf6-kdeclarative libplasma kf6-kio qt6-qtbase kf6-kguiaddons kf6-ki18n wayland-devel libdrm-devel rpm-build
  ```
  
  X11:
  ```
  sudo dnf -y install git cmake extra-cmake-modules gcc-g++ kf6-kwindowsystem-devel plasma-workspace-devel libplasma-devel qt6-qtbase-private-devel qt6-qtbase-devel cmake extra-cmake-modules kf6-knotifications-devel kf6-kio-devel kf6-kcrash-devel kf6-ki18n-devel kf6-kguiaddons-devel libepoxy-devel kf6-kglobalaccel-devel kf6-kcmutils-devel kf6-kconfigwidgets-devel kf6-kdeclarative-devel kdecoration-devel kf6-kglobalaccel kf6-kdeclarative libplasma kf6-kio qt6-qtbase kf6-kguiaddons kf6-ki18n wayland-devel libdrm-devel kwin-x11-devel rpm-build
  ```
</details>

<details>
  <summary>openSUSE</summary>
  <br>

  Wayland:
  ```
  sudo zypper in -y git cmake-full gcc-c++ kf6-extra-cmake-modules kcoreaddons-devel kguiaddons-devel kconfigwidgets-devel kwindowsystem-devel ki18n-devel kiconthemes-devel kpackage-devel frameworkintegration-devel kcmutils-devel kirigami2-devel "cmake(KF6Config)" "cmake(KF6CoreAddons)" "cmake(KF6FrameworkIntegration)" "cmake(KF6GuiAddons)" "cmake(KF6I18n)" "cmake(KF6KCMUtils)" "cmake(KF6KirigamiPlatform)" "cmake(KF6WindowSystem)" "cmake(Qt6Core)" "cmake(Qt6DBus)" "cmake(Qt6Quick)" "cmake(Qt6Svg)" "cmake(Qt6Widgets)" "cmake(Qt6Xml)" "cmake(Qt6UiTools)" "cmake(KF6Crash)" "cmake(KF6GlobalAccel)" "cmake(KF6KIO)" "cmake(KF6Service)" "cmake(KF6Notifications)" libepoxy-devel kwin6-devel
  ```
  
  X11:
  ```
  sudo zypper in -y git cmake-full gcc-c++ kf6-extra-cmake-modules kcoreaddons-devel kguiaddons-devel kconfigwidgets-devel kwindowsystem-devel ki18n-devel kiconthemes-devel kpackage-devel frameworkintegration-devel kcmutils-devel kirigami2-devel "cmake(KF6Config)" "cmake(KF6CoreAddons)" "cmake(KF6FrameworkIntegration)" "cmake(KF6GuiAddons)" "cmake(KF6I18n)" "cmake(KF6KCMUtils)" "cmake(KF6KirigamiPlatform)" "cmake(KF6WindowSystem)" "cmake(Qt6Core)" "cmake(Qt6DBus)" "cmake(Qt6Quick)" "cmake(Qt6Svg)" "cmake(Qt6Widgets)" "cmake(Qt6Xml)" "cmake(Qt6UiTools)" "cmake(KF6Crash)" "cmake(KF6GlobalAccel)" "cmake(KF6KIO)" "cmake(KF6Service)" "cmake(KF6Notifications)" libepoxy-devel kwin6-x11-devel
  ```
</details>

### Building
```sh
git clone https://github.com/4v3ngR/kwin-effects-glass
cd kwin-effects-glass
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install
```

<details>
  <summary>Building on Fedora Kinoite</summary>
  <br>

  ```sh
  # enter container
  git clone https://github.com/4v3ngR/kwin-effects-glass
  cd kwin-effects-glass
  mkdir build
  cd build
  cmake .. -DCMAKE_INSTALL_PREFIX=/usr
  make -j$(nproc)
  cpack -V -G RPM
  exit # exit container
  sudo rpm-ostree install kwin-effects-glass/build/kwin-glass.rpm
  ```
</details>

**Remove the *build* directory when rebuilding the effect.**

# Usage
This effect will conflict with the stock blur effect and any other forks of it.

1. Install the plugin.
2. Open the *Desktop Effects* page in *System Settings*.
3. Disable any blur effects.
4. Enable the *Glass* effect.

### Window transparency
The window needs to be translucent in order for the blur to be visible. This can be done in multiple ways:
- Use a transparent theme for the program if it supports it
- Use a transparent color scheme, such as [Alpha](https://store.kde.org/p/1972214)
- Create a window rule that reduces the window opacity

### Obtaining window classes
The classes of windows to blur can be specified in the effect settings. You can obtain them in two ways:
  - Run ``qdbus org.kde.KWin /KWin org.kde.KWin.queryWindowInfo`` and click on the window. You can use either *resourceClass* or *resourceName*.
  - Right click on the titlebar, go to *More Options* and *Configure Special Window/Application Settings*. The class can be found at *Window class (application)*. If there is a space, for example *Navigator firefox*, you can use either *Navigator* or *firefox*.

# High cursor latency or stuttering on Wayland
This effect can be very resource-intensive if you have a lot of windows open. On Wayland, high GPU load may result in higher cursor latency or even stuttering. If that bothers you, set the following environment variable: ``KWIN_DRM_NO_AMS=1``. If that's not enough, try enabling or disabling the software cursor by also setting ``KWIN_FORCE_SW_CURSOR=0`` or ``KWIN_FORCE_SW_CURSOR=1``.

Intel GPUs use software cursor by default due to [this bug](https://gitlab.freedesktop.org/drm/intel/-/issues/9571), however it doesn't seem to affect all GPUs.

# Credits
- [a-parhom/LightlyShaders](https://github.com/a-parhom/LightlyShaders) - CMakeLists.txt files
