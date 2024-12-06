{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    { flake-parts, treefmt-nix, ... }@inputs:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
    in
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [ treefmt-nix.flakeModule ];
      inherit systems;
      perSystem =
        { pkgs, ... }:
        let
          llvmStdenv = pkgs.llvmPackages_18.libcxxStdenv;
          nativeBuildInputs = with pkgs; [
            clang-tools_18
            gdb
            cmake
            gnuplot
            valgrind
            pkg-config
          ];
          buildInputs = with pkgs; [
            (boost.override {
              useMpi = true;
              stdenv = llvmStdenv;
            })
            mpi
            pkgs.llvmPackages_18.openmp
            (fmt.override { stdenv = llvmStdenv; })
            range-v3
          ];

          shell = (pkgs.mkShell.override { stdenv = llvmStdenv; }) { inherit nativeBuildInputs buildInputs; };
        in
        {
          imports = [ ./nix/treefmt.nix ];
          devShells = {
            default = shell;
          };
        };
    };
}
