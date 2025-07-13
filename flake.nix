{
  description = "A simple C++ project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            pkgs.cmake
            pkgs.gcc
            pkgs.ffmpeg
            pkgs.yt-dlp
            pkgs.httplib
          ];
          shellHook = ''
            # Only set these if they're not already defined
            if [ -z "$YOUTUBE_RTMP_URL" ]; then
              echo "Warning: YOUTUBE_RTMP_URL is not set. Please set it in your shell environment."
            fi
            if [ -z "$YOUTUBE_STREAM_KEY" ]; then
              echo "Warning: YOUTUBE_STREAM_KEY is not set. Please set it in your shell environment."
            fi
          '';
        };
      });
}
