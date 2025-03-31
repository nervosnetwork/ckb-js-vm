let
  # Pin nixpkgs to a specific commit/version
  nixpkgs = fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/c570c1f53044.tar.gz";
    sha256 = "1p9n1pk48sj0f7jsnnk09clkwh7sgcf47a14rwki24hgqlf6r4mn";
  };
  pkgs = import nixpkgs {};
  llvmPackages = pkgs.llvmPackages_18;
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    llvmPackages.libcxxStdenv
    gnumake
    # For shasum command (part of perl)
    perl
  ];

  shellHook = ''
    alias clang-format-18="${pkgs.llvmPackages_18.libcxxStdenv}/bin/clang-format"

    echo "ckb-js-vm Development environment loaded"
  '';
}