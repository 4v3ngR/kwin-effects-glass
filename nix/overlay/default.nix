final: _prev: {
  kwin-glass = final.kdePackages.callPackage ../package/kwin-glass.nix { };
  kwin-glass-x11 = final.kdePackages.callPackage ../package/kwin-glass-x11.nix { };
}
