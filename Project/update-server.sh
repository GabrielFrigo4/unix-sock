#!/bin/sh
set -e

if [ -z "$1" ]; then
	echo "Erro: Informe o diretório a ser copiado."
	exit 1
fi

KEY="${HOME}/.key/ssh-key-frigo-server.key"
USER_HOST="ubuntu@144.22.210.65"
TARGET_PATH="/home/ubuntu/game/${1}"
PARENT_DIR="$(dirname "${TARGET_PATH}")"

ssh -i "${KEY}" "${USER_HOST}" "mkdir -p \"${PARENT_DIR}\" && rm -rf \"${TARGET_PATH}\""
scp -r -i "${KEY}" "${1}" "${USER_HOST}:${TARGET_PATH}"
