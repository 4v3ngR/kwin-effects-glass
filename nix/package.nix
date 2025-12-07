{ lib
, stdenv
, cmake
, extra-cmake-modules
, kwin
, wrapQtAppsHook
, qttools
}:

stdenv.mkDerivation rec {
  pname = "kwin-glass";
  version = "1.5.0";

  src = ./..;

  nativeBuildInputs = [
    cmake
    extra-cmake-modules
    wrapQtAppsHook
  ];

  buildInputs = [
    kwin
    qttools
  ];

  meta = with lib; {
    description = "Fork of the KWin Blur effect for KDE Plasma 6 with additional features (including force blur) and bug fixes";
    license = licenses.gpl3;
    homepage = "https://github.com/4v3ngR/kwin-effects-glass";
  };
}
