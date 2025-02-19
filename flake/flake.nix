{
  description = "Platformio development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    myPackages = {
      url = "git+ssh://git@github.com/Jonas-Finkler/nix-packages.git";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.flake-utils.follows = "flake-utils";
    };
  };

  outputs = { self, nixpkgs, flake-utils, myPackages }:
    flake-utils.lib.eachDefaultSystem (system: 
      let 
        pkgs = import nixpkgs { 
          inherit system; 
          overlays = myPackages.overlays;
        };
        
        python = pkgs.python311;

        upl = pkgs.writeShellApplication {
          name = "upl";
          runtimeInputs = [ pkgs.platformio ];
          text = "pio run --target upload";
        };

        mon = pkgs.writeShellApplication {
          name = "mon";
          runtimeInputs = [ pkgs.platformio ];
          text = "pio device monitor --baud 115200";
        };

        devPkgs = [
          # (python.withPackages (ptPkgs: with ptPkgs; [
          #   # numpy
          #   # matplotlib
          # ]))
        ] ++ (with pkgs; [
          platformio
          upl
          mon
        ]);
        
      in {
    
        devShells.default = pkgs.mkShell {

          buildInputs = devPkgs;

          shellHook = ''
            export FLAKE="platformio"
            # OMP
            export OMP_NUM_THREADS=8
            # back to zsh
            exec zsh
          '';

        };

        packages.default = pkgs.singularity-tools.buildImage {
          name = "pio";
          contents = devPkgs ++ (with pkgs; [
            # coreutils-full # provides ls, cat, ...
          ]);

          # the shadowSetup creates passwd and group files to prevent singularity from complaining
          runAsRoot = ''
            #!${pkgs.stdenv.shell}
            ${pkgs.dockerTools.shadowSetup} 
          '';
          # drop into shell by default
          runScript = ''
            #!${pkgs.stdenv.shell}
            exec /bin/sh $@"
          '';
          diskSize = 1024 * 40;
          memSize = 1024 * 8;
        };
      }
    );
}
