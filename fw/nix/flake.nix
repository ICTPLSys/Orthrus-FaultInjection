{
  description = "Kuriko's Python Template";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";

    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";

    devenv = {
      url = "github:cachix/devenv";
      inputs.nixpkgs.follows = "nixpkgs";
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
        lib,
        system,
        ...
      }: let
        pkgs = import inputs.nixpkgs {
          inherit system;
          config.allowUnfree = true;
          overlays = [];
        };
      in {
        devenv.shells.default = {
          hardeningDisable = ["all"];

          packages = with pkgs; [
            hello
            poetry
          ];

          enterShell = ''
            hello
          '';

          languages.python = {
            enable = true;
            poetry = {
              enable = true;
              activate.enable = true;
            };
          };

          pre-commit.hooks = {
            alejandra.enable = true;

            isort.enable = true;
            # mypy.enable = true;
            # pylint.enable = true;
            # pyright.enable = true;
            # flake8.enable = true;
          };
          cachix.push = "kurikomoe";
        };
      };

      flake = {};
    };
}
