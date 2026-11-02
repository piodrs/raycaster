{
  description = "Raycaster";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/26.05";

  outputs =
    { nixpkgs, ... }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
    in
    {
      devShells.x86_64-linux.default = pkgs.mkShell {
        packages = with pkgs; [
          gcc
          gdb
          pkg-config
          raylib
        ];
      };
    };
}
