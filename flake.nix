{
  description = "Fork of the KWin Blur effect for KDE Plasma 6 with additional features (including force blur) and bug fixes";

  inputs = {
    # CAUTION: nixpkgs-unstable lacks kdePackages.kwin-x11
    nixpkgs.url = "https://channels.nixos.org/nixos-26.05/nixexprs.tar.zst";
  };

  outputs =
    { self, nixpkgs, ... }:
    let
      supportedSystem = [
        "x86_64-linux"
        "aarch64-linux"
      ];

      eachSystem =
        systems: f:
        builtins.foldl' (
          a: s: a // builtins.mapAttrs (k: v: (a.${k} or { }) // { ${s} = v; }) (f s)
        ) { } systems;
    in
    eachSystem supportedSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      {
        packages = {
          default = pkgs.kdePackages.callPackage ./nix/package.nix { };
          x11 = pkgs.kdePackages.callPackage ./nix/package-x11.nix { };
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
        };
      }
    );
}
