{
  description = "auto_scee dev environment";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";

    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";

    devenv = {
      url = "github:cachix/devenv";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    nixpkgs-python = {
      url = "github:cachix/nixpkgs-python";
      inputs = {nixpkgs.follows = "nixpkgs";};
    };
  };

  nixConfig = {
    extra-trusted-public-keys = "devenv.cachix.org-1:w1cLUi8dv3hnoSPGAuibQv+f9TZLr6cv/Hm9XgU50cw=";
    extra-substituters = "https://devenv.cachix.org";
  };

  outputs = inputs @ {flake-parts, ...}:
    flake-parts.lib.mkFlake {inherit inputs;} {
      imports = [
        inputs.devenv.flakeModule
      ];

      systems = ["x86_64-linux" "aarch64-linux" "aarch64-darwin" "x86_64-darwin"];

      perSystem = {
        config,
        self',
        inputs',
        system,
        ...
      }: let
        pkgs = import inputs.nixpkgs {
          inherit system;
          config.allowUnfree = true;
          overlays = [];
        };

        lib = pkgs.lib;

        gcc-bin = pkgs.writeShellScriptBin "gcc" ''
          ${pkgs.gcc9}/bin/gcc -Wno-error $@
        '';

        gcc-9-bin = pkgs.writeShellScriptBin "gcc-9" ''
          ${gcc-bin}/bin/gcc -Wno-error $@
        '';
      in {
        devenv.shells.default = {
          hardeningDisable = ["all"];

          # stdenv = pkgs.stdenvAdapters.useMoldLinker pkgs.stdenv;
          stdenv = pkgs.llvmPackages_16.stdenv;
          # stdenv = pkgs.gcc9Stdenv;

          packages = with pkgs; [
            # gcc-bin
            # gcc-9-bin
            # gcc9

            # clang_16

            # clang16
            # glibc
            rdma-core.dev
            libmnl

            # build requisitions
            pkg-config
            stdenv.cc.cc.lib

            ccache
            mold
            autoreconfHook
            autogen
            cmake
            ninja

            boost

            ## shenango
            libnl
            numactl

            ## intel-isal
            nasm
            yasm

            # baselines
            rocksdb
            ## rocksdb req
            liburing
            snappy
            lz4
            zlib
            zstd
            jemalloc
            mimalloc
            gflags
            gtest

            redis

            # tools
            just
            clang-tools
            hello
            vcpkg

            # c_cpp related tools
            valgrind
            flawfinder
            cppcheck

            # backtrace deps
            libunwind
            libbfd
            libdwarf-lite
            elfutils
          ];

          enterShell = ''
            export NIX_ENFORCE_NO_NATIVE=0

            export CFLAGS="-Wno-error $CFLAGS"
            export CXXFLAGS="-Wno-error $CXXFLAGS"

            # disable keyring to avoid poetry stuck
            export PYTHON_KEYRING_BACKEND=keyring.backends.null.Keyring

            export VCPKG_ROOT="${pkgs.vcpkg}/share/vcpkg"

            # disable `as` to let intel-isal fallback to nasm rather than using gcc/as
            export AS=""

            hello
          '';

          # languages.python = {
          #   enable = true;
          #   package = pkgs.python311;
          #   uv.enable = true;
          # };

          pre-commit.hooks = {
            alejandra.enable = true;

            clang-format.enable = false;
            clang-format.types_or = lib.mkForce ["c" "c++"];

            # clang-tidy.enable = true;

            isort.enable = true;
          };

          cachix.push = "kurikomoe";
        };
      };

      flake = {};
    };
}
