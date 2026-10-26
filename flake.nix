{
  description = "Raycaster";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/26.05";

  outputs =
    { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              gcc
              gdb
              pkg-config
              raylib

              clang-tools
            ];

            shellHook = ''
              echo "Raycaster development environment"
            '';
          };
        }
      );
    };
}
