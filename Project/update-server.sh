#!/usr/bin/env sh
set -eu

if [ -z "${1:-}" ]; then
	echo "Erro: Informe o diretório a ser copiado." >&2
	exit 1
fi

KEY="${PERSONAL_SERVER_KEY:-}"
if [ -z "${KEY}" ] || [ ! -f "${KEY}" ]; then
	for _cand in \
		"${VAULT_DIR:-}/keys/ssh-key-personal-server.key" \
		"${HOME}/.local/share/vault/keys/ssh-key-personal-server.key" \
		"${HOME}/.vault/keys/ssh-key-personal-server.key" \
		"${HOME}/.key/ssh-key-personal-server.key"; do
		if [ -f "${_cand}" ]; then
			KEY="${_cand}"
			break
		fi
	done
fi

SERVER_IP="${PERSONAL_SERVER_IP:-144.22.210.65}"
SERVER_USER="${PERSONAL_SERVER_USER:-ubuntu}"
USER_HOST="${SERVER_USER}@${SERVER_IP}"
TARGET_PATH="/home/${SERVER_USER}/game/${1}"
PARENT_DIR="$(dirname "${TARGET_PATH}")"

if [ -n "${KEY}" ] && [ -f "${KEY}" ]; then
	chmod 0600 "${KEY}" 2> "/dev/null" || true
	ssh -i "${KEY}" "${USER_HOST}" "mkdir -p \"${PARENT_DIR}\" && rm -rf \"${TARGET_PATH}\""
	scp -r -i "${KEY}" "${1}" "${USER_HOST}:${TARGET_PATH}"
else
	ssh "${USER_HOST}" "mkdir -p \"${PARENT_DIR}\" && rm -rf \"${TARGET_PATH}\""
	scp -r "${1}" "${USER_HOST}:${TARGET_PATH}"
fi
