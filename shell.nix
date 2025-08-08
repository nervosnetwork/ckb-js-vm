let
  # Pin nixpkgs to a specific commit/version
  nixpkgs = fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/c570c1f53044.tar.gz";
    sha256 = "1p9n1pk48sj0f7jsnnk09clkwh7sgcf47a14rwki24hgqlf6r4mn";
  };
  pkgs = import nixpkgs {};
  llvmPackages = pkgs.llvmPackages_19;
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    llvmPackages.clangNoCompilerRtWithLibc
    pkgs.musl
    gnumake
    # Add LLVM tools
    llvmPackages.bintools
    llvmPackages.llvm
  ];

  shellHook = ''
    echo "ckb-js-vm Development environment loaded"
    export NIX_HARDENING_ENABLE=""
#    export NIX_DEBUG=1
  '';
}
