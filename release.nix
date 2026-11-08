{
  system ? builtins.currentSystem,
}:

let
  # …With flakes as the input pinner
  flake_lock = builtins.fromJSON (builtins.readFile ./flake.lock);

  nixpkgs =
    let
      lock = flake_lock.nodes.nixpkgs.locked;
    in
    builtins.fetchTarball {
      inherit (lock) url;
      sha256 = lock.narHash;
    };

  pkgs = import nixpkgs {
    inherit system;
    overlays = [
      (import ./nix/overlay)
      (import ./nix/overlay/development.nix)
    ];
  };
in
{
  inherit (pkgs)
    kwin-glass
    kwin-glass-x11
    ;
  default = pkgs.kwin-glass;
  shell = pkgs.kwin-glass-dev-shell;
}
