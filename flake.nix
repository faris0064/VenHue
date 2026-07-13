{
  description = "VenHue Linux development environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      llvm = pkgs.llvmPackages_21;
    in {
      devShells.${system}.default = pkgs.mkShell.override {
        stdenv = llvm.stdenv;
      } {
        CMAKE_POLICY_VERSION_MINIMUM = "3.5";
        QML_IMPORT_PATH = "${pkgs.qt6.qtdeclarative}/lib/qt-6/qml";

        nativeBuildInputs = [
          pkgs.cmake
          pkgs.git
          llvm.clang-tools
          pkgs.ninja
          pkgs.pkg-config
          pkgs.qt6.wrapQtAppsHook
        ];

        buildInputs = with pkgs; [
          brotli
          qt6.qtbase
          qt6.qtdeclarative
          qt6.qtwayland
          zstd
        ];
      };
    };
}
