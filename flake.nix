{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    foolnotion.url = "github:foolnotion/nur-pkg";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    foolnotion,
    flake-utils,
    ...
  } @ inputs: let
    systems = ["x86_64-linux" "aarch64-linux"];
  in
    flake-utils.lib.eachSystem systems (system: let
      pkgs = import nixpkgs {inherit system;
        overlays = [ foolnotion.overlay ];
      };
      clang-tools = pkgs.clang-tools_18;
     
      nativeBuildInputs = with pkgs; [
        clang-tools
        gdb
        cmake
        gnuplot
        pkg-config
      ];
      buildInputs = with pkgs; [
        (boost.override { useMpi = true; })
        mpi
        fmt
        range-v3
        mdspan
      ];

      shell =
        (pkgs.mkShell.override {
          stdenv = pkgs.llvmPackages_18.stdenv;
        }) {
          inherit nativeBuildInputs buildInputs;
        };
    in {
      devShells = {
        default = shell;
      };
    });
}
