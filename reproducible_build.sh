#!/usr/bin/env bash
#
# An utility script helping with reproducible script builds via docker.
# Note that this utility serves only as one example, docker is not
# necessarily THE way to do reproducible build, nor is it the best way
# to do reproducible build.
set -e

cleanup() {
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "Script exited with error code $exit_code"
    fi
}

trap cleanup EXIT

DOCKER="${DOCKER:-docker}"
# docker pull docker.io/cryptape/llvm-n-rust:20240630
DOCKER_IMAGE="${DOCKER_IMAGE:-docker.io/cryptape/llvm-n-rust@sha256:bafaf76d4f342a69b8691c08e77a330b7740631f3d1d9c9bee4ead521b29ee55}"
CHECKSUM_FILE_PATH="${CHECKSUM_FILE_PATH:-checksums.txt}"

# We are parsing command line arguments based on tips from:
# https://stackoverflow.com/a/14203146

while [[ $# -gt 0 ]]; do
  case $1 in
    -p|--proxy)
      PROXY="$2"
      shift # past argument
      shift # past value
      ;;
    -u|--update)
      UPDATE="yes"
      shift # past argument
      ;;
    --no-clean)
      NOCLEAN="yes"
      shift # past argument
      ;;
    -*|--*)
      echo "Unknown option $1"
      exit 1
      ;;
    *)
      echo "Unknown argument $1"
      exit 1
      ;;
  esac
done

if [[ -n "${PROXY}" ]]; then
  DOCKER_RUN_ARGS="-e ALL_PROXY=${PROXY} -e HTTPS_PROXY=${PROXY} -e HTTP_PROXY=${PROXY} ${DOCKER_RUN_ARGS}"
fi

TASKS=""
if [[ "${NOCLEAN}" != "yes" ]]; then
  TASKS+=" clean "
fi

if [[ "${UPDATE}" = "yes" ]]; then
  TASKS+=" checksum"
else
  TASKS+=" all "
fi

$DOCKER run --rm $DOCKER_RUN_ARGS -e UID=`id -u` -e GID=`id -g` -v `pwd`:/code $DOCKER_IMAGE make $TASKS
# Reset file ownerships for all files docker might touch
$DOCKER run --rm $DOCKER_RUN_ARGS -e UID=`id -u` -e GID=`id -g` -v `pwd`:/code $DOCKER_IMAGE bash -c 'chown -R -f $UID:$GID build'

if [[ "${UPDATE}" = "yes" ]]; then
    echo "${CHECKSUM_FILE_PATH} file is updated with latest binary hashes!"
else
    echo "checking checksums..."
    shasum -a 256 -c ${CHECKSUM_FILE_PATH}
fi
