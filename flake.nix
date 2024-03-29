{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    ...
  } @ inputs: let
    systems = ["x86_64-linux" "aarch64-linux"];
  in
    flake-utils.lib.eachSystem systems (system: let
      pkgs = import nixpkgs {inherit system;};

      clang-tools = pkgs.clang-tools_17;
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
      ];

      shell =
        (pkgs.mkShell.override {
          stdenv = pkgs.llvmPackages_17.stdenv;
        }) {
          inherit nativeBuildInputs buildInputs;
        };
    in {
      devShells = {
        default = shell;
      };
    });
}
