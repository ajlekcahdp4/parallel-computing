{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
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
        (boost.override { useMpi = true; stdenv = llvmStdenv; })
        mpi
        (fmt.override {stdenv = llvmStdenv;})
        range-v3
      ];

      shell =
        (pkgs.mkShell.override {
          stdenv = llvmStdenv;
        }) {
          inherit nativeBuildInputs buildInputs;
        };
    in {
      devShells = {
        default = shell;
      };
    });
}
