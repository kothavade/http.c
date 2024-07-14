{
  pkgs ? import <nixpkgs> { },
}:
let
  cc = pkgs.stdenv.mkDerivation rec {
    name = "codecrafters-cli-${version}";
    version = "33";

    # https://github.com/codecrafters-io/cli/releases
    src = pkgs.fetchurl {
      url = "https://github.com/codecrafters-io/cli/releases/download/v${version}/v${version}_darwin_arm64.tar.gz";
      sha256 = "sha256-vBaLSW/5inanndfdiImPJF+pAhXtM1XALfz0bWP7N9k=";
    };

    unpackPhase = ''
      mkdir -p $out
      tar -xzvf ${src}
    '';

    installPhase = ''
      mkdir -p $out/bin
      cp -r . $out/bin/
    '';
  };
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    curl
    clang-tools
    cc
    # valgrind
  ];
}
