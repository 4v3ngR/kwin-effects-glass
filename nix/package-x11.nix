{ lib
, stdenv
, cmake
, extra-cmake-modules
, kwin-x11
, wrapQtAppsHook
, qttools
}:

stdenv.mkDerivation rec {
  pname = "kwin-glass";
  version = "1.6.3";

  src = ./..;

  nativeBuildInputs = [
    cmake
    extra-cmake-modules
    wrapQtAppsHook
  ];

  buildInputs = [
    kwin-x11
    qttools
  ];

  meta = with lib; {
    description = "Fork of the KWin Blur effect for KDE Plasma 6 with additional features (including force blur) and bug fixes";
    license = licenses.gpl3;
    homepage = "https://github.com/4v3ngR/kwin-effects-glass";
  };
}
