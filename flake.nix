{
  description = "42sh, your shell";

  inputs = {
    nixpie.url = "git+https://gitlab.cri.epita.fr/cri/infrastructure/nixpie.git";
    nixpkgs.follows = "nixpie/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
    pre-commit-hooks = {
      url = "github:cachix/pre-commit-hooks.nix";
      inputs.nixpkgs.follows = "nixpie/nixpkgs";
    };
  };

  outputs = { self, nixpie, nixpkgs, flake-utils, pre-commit-hooks }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        # switch it to pkgs.stdenv if you want to build with gcc
        stdenv = with pkgs; overrideCC clangStdenv [ clang_12 llvmPackages_12.llvm llvmPackages_12.lld ];
      in
      rec {
        checks = {
          pre-commit-check = pre-commit-hooks.lib.${system}.run {
            src = ./.;
            hooks = {
              nixpkgs-fmt.enable = true;
              clang-format = {
                enable = true;
                types_or = [ "c" ];
              };
            };
          };
        };
        packages = flake-utils.lib.flattenTree {
          "42sh" = stdenv.mkDerivation {
            pname = "42sh";
            version = "1.0";
            src = ./.;
            buildInputs = [ pkgs.readline ];
            nativeBuildInputs = [ pkgs.meson pkgs.ninja ];
          };
        };
        defaultPackage = packages."42sh";
        apps."42sh" = flake-utils.lib.mkApp { drv = packages."42sh"; };
        defaultApp = apps."42sh";
        devShell = pkgs.mkShell.override { inherit stdenv; } {
          inherit (checks.pre-commit-check) shellHook;
          # toolchain hardening injects unwanted compiler flags.
          # fortify injects -O2 along with -D_FORTIFY_SOURCE=2,
          # which breaks the debugging experience
          hardeningDisable = [ "all" ];
          inputsFrom = [ packages."42sh" ];
          packages = [
            pkgs.python3
            pkgs.doxygen
            pkgs.gdb
            pkgs.afl
            pkgs.valgrind
          ];
        };
      }
    );
}
