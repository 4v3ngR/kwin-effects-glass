{
  stdenv,
  cmake,
  extra-cmake-modules,
  kwin-x11,
  wrapQtAppsHook,
  qttools,
  kwin-glass,
}:

stdenv.mkDerivation {
  inherit (kwin-glass)
    version
    src
    ;

  pname = kwin-glass.pname + "-x11";

  nativeBuildInputs = [
    cmake
    extra-cmake-modules
    wrapQtAppsHook
  ];

  buildInputs = [
    kwin-x11
    qttools
  ];

  meta = kwin-glass.meta // {
    description = kwin-glass.meta.description + " with X11 support";
  };
}
