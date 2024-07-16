{
  pkgs ? import <nixpkgs> { },
}:
let
  cc = pkgs.stdenv.mkDerivation rec {
    name = "codecrafters-cli-${version}";
    version = "34";

    # https://github.com/codecrafters-io/cli/releases
    src = pkgs.fetchurl {
      url = "https://github.com/codecrafters-io/cli/releases/download/v${version}/v${version}_darwin_arm64.tar.gz";
      sha256 = "sha256-dh2J9oTqAsbHaaY4Y6nUM1IlFym+njXWG5Kpj6MfBT8=";
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
