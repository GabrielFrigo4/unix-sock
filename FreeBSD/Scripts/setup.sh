#!/bin/sh

### ################################################################################################################################

### ################################
### Setup System (Root)
### ################################

# GROUPS
pw groupmod wheel -m "$(id -un)"

# PACKAGE
pkg bootstrap --yes
pkg update
pkg upgrade --yes

# KVM/QEMU
pkg install --yes qemu-guest-agent
sysrc qemu_guest_agent_enable="YES"
sysrc qemu_guest_agent_flags="-d -m virtio-serial -p /dev/ttyV0.1"
service qemu-guest-agent start

# TERMINAL
sysrc allscreens_flags="-f spleen-16x32"

# SUDO
pkg install --yes sudo
cat << 'EOF' | tee "/usr/local/etc/sudoers.d/wheel" > "/dev/null"
%wheel ALL=(ALL:ALL) NOPASSWD: ALL
EOF
chmod 0440 "/usr/local/etc/sudoers.d/wheel"

# DOAS
pkg install --yes doas
cat << 'EOF' | tee "/usr/local/etc/doas.conf" > "/dev/null"
permit nopass :wheel
EOF
chmod 0440 "/usr/local/etc/doas.conf"

### ################################
### Setup Environment (Root)
### ################################

# COREDUMP
sysctl kern.coredump=0
cat << 'EOF' | tee -a "/etc/sysctl.conf" > "/dev/null"
kern.coredump=0
EOF

### ################################################################################################################################

### ################################
### Setup Shell (User)
### ################################

# SHELL
sudo pkg install --yes bash
sudo pkg install --yes zsh

### ################################
### Setup Wget (User)
### ################################

# WGET
sudo pkg install --yes wget
sudo pkg install --yes wget2
sudo pkg install --yes curl

### ################################
### Setup Git (User)
### ################################

# GIT ECOSYSTEM
sudo pkg install --yes git
sudo pkg install --yes git-credential-oauth
sudo pkg install --yes gh

# GIT CONFIG
rm "${HOME}/.gitconfig"
git config --global credential.helper "!gh auth git-credential"
git config --global user.email "gabriel.frigo4@gmail.com"
git config --global user.name "Gabriel Frigo"
git config --global init.defaultBranch "main"
git config --global pull.rebase false
git config --global color.ui auto

# GITHUB CONFIG
gh auth login
gh auth setup-git

### ################################
### Setup Ports (User)
### ################################

# PORTS
sudo git clone "https://git.FreeBSD.org/ports.git" "/usr/ports"

# UPDATE
cd "/usr/ports"
sudo git pull
cd "${HOME}"

### ################################
### Setup Jails (User)
### ################################

# JAILS
sudo sysrc jail_enable="YES"

### ################################
### Setup Containers (User)
### ################################

# BASTILLE
sudo pkg install --yes bastille
sudo sysrc bastille_enable="YES"
sudo service bastille start

### ################################################################################################################################

### ################################
### Config Shell (User)
### ################################

# Default Shell
sudo chsh -s "$(which sh)" "$(id -un)"
sudo chsh -s "$(which sh)" "root"

# Shell Bootstrap
SHELL_BLOCK="$(mktemp)"
cat << 'EOF' | tee "${SHELL_BLOCK}" > "/dev/null"
### ################################
### TRIGGERS ENVIRONMENT
### ################################

TRIGGERS_CACHE="${HOME}/.cache/triggers.sh"

triggers-setup() {
	mkdir -p "${HOME}/.cache"
	echo "# $(command date +%Y-%m-%d)" > "${TRIGGERS_CACHE}"

	local IGNORE_LIST=" sh command eval alias unalias return echo printf test [ clear "
	TRIGGERS=""

	for file in /bin/* /sbin/* /usr/bin/* /usr/sbin/*; do
		if [ -f "$file" ] && [ -x "$file" ]; then
			local cmd_name="${file##*/}"
			case "${IGNORE_LIST}" in
				*" ${cmd_name} "*) continue ;;
			esac
			TRIGGERS="${TRIGGERS} ${cmd_name}"
		fi
	done

	for file in /usr/local/bin/? /usr/local/sbin/? \
				/usr/local/bin/?? /usr/local/sbin/?? \
				/usr/local/bin/??? /usr/local/sbin/??? \
				/usr/local/bin/???? /usr/local/sbin/???? \
				/usr/local/bin/????? /usr/local/sbin/?????; do
		if [ -f "$file" ] && [ -x "$file" ]; then
			local cmd_name="${file##*/}"
			case "${IGNORE_LIST}" in
				*" ${cmd_name} "*) continue ;;
			esac
			TRIGGERS="${TRIGGERS} ${cmd_name}"
		fi
	done

	TRIGGERS=$(echo ${TRIGGERS} | tr ' ' '\n' | sort -u | tr '\n' ' ')
	for cmd in ${TRIGGERS}; do
		if command -v "$cmd" > "/dev/null" 2>&1; then
			echo "unalias ${cmd} > \"/dev/null\" 2>&1" >> "${TRIGGERS_CACHE}"
			case "$cmd" in
				*+*|*-*|*.*)
					echo "alias ${cmd}='run_and_update ${cmd}'" >> "${TRIGGERS_CACHE}"
					;;
				*)
					echo "${cmd}() { run_and_update ${cmd} \"\$@\"; }" >> "${TRIGGERS_CACHE}"
					;;
			esac
		fi
	done
}

if [ -f "${TRIGGERS_CACHE}" ]; then
	read -r TRIGGERS_DATE < "${TRIGGERS_CACHE}"
	if [ "${TRIGGERS_DATE}" != "# $(command date +%Y-%m-%d)" ]; then
		command rm -f "${TRIGGERS_CACHE}"
	fi
fi

if [ ! -f "${TRIGGERS_CACHE}" ]; then
	triggers-setup
fi

### ################################
### TERMINAL ENVIRONMENT
### ################################

case "$(command ps -o comm= -p $PPID)" in
	su|-su) unset SHELL_INIT ;;
esac

if [ "${USER}" != "$(command id -un)" ]; then
	export USER="$(command id -un)"
	unset SHELL_INIT
fi

if [ -z "${SHELL_INIT}" ]; then
	if [ -z "${SHELL_TARGET}" ]; then
		case "$(command tty 2> "/dev/null")" in
			"/dev/ttyv"*|"/dev/console") SHELL_TARGET="$(command -v sh 2> "/dev/null")" ;;
			*) SHELL_TARGET="$(command -v zsh 2> "/dev/null")" ;;
		esac
		[ -x "${SHELL_TARGET}" ] || SHELL_TARGET="$(command -v sh 2> "/dev/null")"
	fi

	if [ -x "${SHELL_TARGET}" ] && [ "${SHELL_TARGET}" != "$(command -v sh 2> "/dev/null")" ]; then
		export SHELL_INIT=1
		export SHELL="${SHELL_TARGET}"
		unset SHELL_TARGET
		command printf "\033[H\033[2J\033[3J"
		exec "${SHELL}"
	else
		unset SHELL_TARGET
	fi
fi

### ################################
### BASIC ENVIRONMENT
### ################################
EOF
touch "${HOME}/.shrc"
cat "${SHELL_BLOCK}" "${HOME}/.shrc" > "${HOME}/.shrc.tmp" && mv "${HOME}/.shrc.tmp" "${HOME}/.shrc"
sudo touch "/root/.shrc"
sudo cat "${SHELL_BLOCK}" "/root/.shrc" | sudo tee "/root/.shrc.tmp" > "/dev/null" && sudo mv "/root/.shrc.tmp" "/root/.shrc"
rm "${SHELL_BLOCK}"

# Config Shell
cat << 'EOF' | tee -a "${HOME}/.shrc" | sudo tee -a "/root/.shrc" > "/dev/null"
### ################################
### SHELL INITIALIZATION
### ################################

export SHELL_INIT=1

### ################################
### SHELL ENVIRONMENT
### ################################

path_front() {
	if [ -d "${1}" ] && [[ ":${PATH}:" != *":${1}:"* ]]; then
		export PATH="${1}:${PATH}"
	fi
}

path_back() {
	if [ -d "${1}" ] && [[ ":${PATH}:" != *":${1}:"* ]]; then
		export PATH="${PATH}:${1}"
	fi
}

path_front "${HOME}/.local/bin"
export PATH=$(command printf "%s" "${PATH}" | command awk -v RS=: -v ORS=: '!a[$(0)]++' | command sed 's/:$//')

### ################################
### SHELL APPEARANCE
### ################################

git_branch() {
	_git_branch=" "
	if command git rev-parse --is-inside-work-tree > "/dev/null" 2>&1; then
		local branch="$(command git branch --show-current 2> "/dev/null" || command git rev-parse --short HEAD 2> "/dev/null")"
		if [ -n "$branch" ]; then
			local is_dirty="$(command git status --short -uno 2> "/dev/null" | command tail -n1)"
			local indicator=""
			[ -n "$is_dirty" ] && indicator="${Y}*"
			_git_branch=" ${B}(${R}${branch}${indicator}${B})${z} "
		fi
	fi
}

update_prompt() {
	local z="\[\e[0m\]"
	local R="\[\e[1;91m\]"
	local G="\[\e[1;92m\]"
	local Y="\[\e[1;93m\]"
	local B="\[\e[1;94m\]"
	local M="\[\e[1;95m\]"
	local C="\[\e[1;96m\]"
	local K="\[\e[1;90m\]"

	local _git_branch
	git_branch

	local u
	if [ "$(command id -u)" -eq 0 ]; then u="${R}"; else u="${G}"; fi

	local cur_user="$(command id -un)"
	local cur_host="$(command hostname -s)"
	local cur_dir="${PWD##*/}"
	[ "${PWD}" = "${HOME}" ] && cur_dir="~"
	[ "${PWD}" = "/" ] && cur_dir="/"

	export PS1="${u}${cur_user}${B}@${M}${cur_host}${K}:${K}[${Y}${cur_dir}${K}]${z}${_git_branch}${C}\$${z} "
}

alias :="update_prompt; command :"
update_prompt

run_and_update() {
	local cmd="${1}"
	shift
	command "$cmd" "$@"
	local ret=$?
	update_prompt
	return $ret
}

alias triggers-reset="rm -f ${TRIGGERS_CACHE} && triggers-setup && . ${TRIGGERS_CACHE}"
. "${TRIGGERS_CACHE}"

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

# Commands ALIAS
alias clear='command printf "\033[H\033[2J\033[3J"'
# Packages ALIAS
alias uppkg='command sudo pkg update && command sudo pkg upgrade --yes'
alias upall='uppkg'

### ################################
### SHELL CONFIGURATION
### ################################
EOF

### ################################
### Config Bash (User)
### ################################

# Install Oh-My-Bash
curl -fsSL "https://raw.githubusercontent.com/ohmybash/oh-my-bash/master/tools/install.sh" | bash -s -- --unattended
curl -fsSL "https://raw.githubusercontent.com/ohmybash/oh-my-bash/master/tools/install.sh" | sudo bash -s -- --unattended

# Setup Oh-My-Bash
sed -i '' 's/OSH_THEME=".*"/OSH_THEME=""/' "${HOME}/.bashrc"
sudo sed -i '' 's/OSH_THEME=".*"/OSH_THEME=""/' "/root/.bashrc"

# Config Bash
cat << 'EOF' | tee -a "${HOME}/.bashrc" | sudo tee -a "/root/.bashrc" > "/dev/null"
### ################################
### SHELL INITIALIZATION
### ################################

export SHELL_INIT=1

### ################################
### SHELL ENVIRONMENT
### ################################

path_front() {
	if [ -d "${1}" ] && [[ ":${PATH}:" != *":${1}:"* ]]; then
		export PATH="${1}:${PATH}"
	fi
}

path_back() {
	if [ -d "${1}" ] && [[ ":${PATH}:" != *":${1}:"* ]]; then
		export PATH="${PATH}:${1}"
	fi
}

path_front "${HOME}/.local/bin"
export PATH=$(printf "%s" "${PATH}" | awk -v RS=: -v ORS=: '!a[$(0)]++' | sed 's/:$//')

### ################################
### SHELL APPEARANCE
### ################################

git_branch() {
	if git rev-parse --is-inside-work-tree &> "/dev/null"; then
		local branch="$(git branch --show-current 2> "/dev/null" || git rev-parse --short HEAD 2> "/dev/null")"
		if [ -n "$branch" ]; then
			local is_dirty="$(git status --short -uno 2> "/dev/null" | tail -n1)"
			local indicator=""
			[ -n "$is_dirty" ] && indicator="${C_BRT_YELLOW}*"
			echo "❮${C_BRT_RED}󰊢 ${C_BRT_MAGENTA}${branch}${indicator}${C_NORM_YELLOW}❯"
		fi
	fi
}

update_prompt() {
	local C_RESET="\[\e[0m\]"

	local C_NORM_BLACK="\[\e[0;30m\]"
	local C_NORM_RED="\[\e[0;31m\]"
	local C_NORM_GREEN="\[\e[0;32m\]"
	local C_NORM_YELLOW="\[\e[0;33m\]"
	local C_NORM_BLUE="\[\e[0;34m\]"
	local C_NORM_MAGENTA="\[\e[0;35m\]"
	local C_NORM_CYAN="\[\e[0;36m\]"
	local C_NORM_WHITE="\[\e[0;37m\]"

	local C_BRT_GRAY="\[\e[1;90m\]"
	local C_BRT_RED="\[\e[1;91m\]"
	local C_BRT_GREEN="\[\e[1;92m\]"
	local C_BRT_YELLOW="\[\e[1;93m\]"
	local C_BRT_BLUE="\[\e[1;94m\]"
	local C_BRT_MAGENTA="\[\e[1;95m\]"
	local C_BRT_CYAN="\[\e[1;96m\]"
	local C_BRT_WHITE="\[\e[1;97m\]"

	local os_version="$(freebsd-version)"
	local sh_name="${0##*/}"
	sh_name="${sh_name#-}"

	local usr_color
	if [ "$(id -u)" -eq 0 ]; then
		usr_color="${C_BRT_RED}"
	else
		usr_color="${C_BRT_GREEN}"
	fi

	PS1="\n${C_NORM_YELLOW}${C_BRT_RED} ${C_BRT_MAGENTA}${os_version}${C_NORM_YELLOW}─${C_BRT_BLUE} ${C_BRT_MAGENTA}${sh_name}${C_NORM_YELLOW}"
	PS1+="\n${C_NORM_YELLOW}┌──❮ ${C_BRT_GREEN} \t${C_NORM_YELLOW} ❯─❮ ${C_BRT_GREEN} \D{%d/%m/%y}${C_NORM_YELLOW} ❯─❮ ${C_BRT_YELLOW} ${C_BRT_CYAN}\W${C_NORM_YELLOW} ❯─ ❮${C_BRT_BLUE} ${usr_color}\u${C_NORM_YELLOW}❯ $(git_branch)"
	PS1+="\n${C_NORM_YELLOW}└─${C_BRT_BLUE}${C_RESET} "
}

PROMPT_COMMAND=update_prompt

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

# Commands ALIAS
alias clear='printf "\e[H\e[2J\e[3J"'
# Packages ALIAS
alias uppkg='sudo pkg update && sudo pkg upgrade --yes'
alias upall='uppkg'

### ################################
### SHELL CONFIGURATION
### ################################
EOF

### ################################
### Config Zsh (User)
### ################################

# Install Oh-My-Zsh
curl -fsSL "https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh" | zsh -s -- --unattended
curl -fsSL "https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh" | sudo zsh -s -- --unattended

# Config Zsh
cat << 'EOF' | tee -a "${HOME}/.zshrc" | sudo tee -a "/root/.zshrc" > "/dev/null"
### ################################
### SHELL INITIALIZATION
### ################################

export SHELL_INIT=1

### ################################
### SHELL OPTIONS SETUP
### ################################

# Expansion OPTIONS
setopt PROMPT_SUBST

# Globbing OPTIONS
setopt EXTENDED_GLOB
setopt GLOB_DOTS

# History OPTIONS
setopt SHARE_HISTORY
setopt HIST_IGNORE_DUPS
setopt HIST_IGNORE_SPACE
setopt HIST_REDUCE_BLANKS
setopt HIST_EXPIRE_DUPS_FIRST
setopt HIST_FIND_NO_DUPS
setopt HIST_VERIFY

# Interaction OPTIONS
setopt CORRECT
setopt INTERACTIVE_COMMENTS
setopt RM_STAR_WAIT
setopt NO_CLOBBER
unsetopt BEEP

# Navigation OPTIONS
setopt AUTO_CD
setopt AUTO_PUSHD
setopt PUSHD_IGNORE_DUPS
setopt PUSHD_SILENT
setopt COMPLETE_IN_WORD

### ################################
### SHELL ENVIRONMENT
### ################################

path_front() {
	if [ -d "${1}" ] && [[ ":${PATH}:" != *":${1}:"* ]]; then
		export PATH="${1}:${PATH}"
	fi
}

path_back() {
	if [ -d "${1}" ] && [[ ":${PATH}:" != *":${1}:"* ]]; then
		export PATH="${PATH}:${1}"
	fi
}

path_front "${HOME}/.local/bin"
export PATH=$(printf "%s" "${PATH}" | awk -v RS=: -v ORS=: '!a[$(0)]++' | sed 's/:$//')

### ################################
### SHELL APPEARANCE
### ################################

() {
	zstyle ':prompt:colors' reset     '%f%b'

	zstyle ':prompt:colors' n_black   '%b%F{0}'
	zstyle ':prompt:colors' n_red     '%b%F{1}'
	zstyle ':prompt:colors' n_green   '%b%F{2}'
	zstyle ':prompt:colors' n_yellow  '%b%F{3}'
	zstyle ':prompt:colors' n_blue    '%b%F{4}'
	zstyle ':prompt:colors' n_magenta '%b%F{5}'
	zstyle ':prompt:colors' n_cyan    '%b%F{6}'
	zstyle ':prompt:colors' n_white   '%b%F{7}'

	zstyle ':prompt:colors' b_gray    '%B%F{8}'
	zstyle ':prompt:colors' b_red     '%B%F{9}'
	zstyle ':prompt:colors' b_green   '%B%F{10}'
	zstyle ':prompt:colors' b_yellow  '%B%F{11}'
	zstyle ':prompt:colors' b_blue    '%B%F{12}'
	zstyle ':prompt:colors' b_magenta '%B%F{13}'
	zstyle ':prompt:colors' b_cyan    '%B%F{14}'
	zstyle ':prompt:colors' b_white   '%B%F{15}'

	git_branch() {
		if git rev-parse --is-inside-work-tree &> "/dev/null"; then
			local branch="$(git branch --show-current 2> "/dev/null" || git rev-parse --short HEAD 2> "/dev/null")"
			if [[ -n "$branch" ]]; then
				local y Y R M
				zstyle -s ':prompt:colors' n_yellow y
				zstyle -s ':prompt:colors' b_yellow Y
				zstyle -s ':prompt:colors' b_red R
				zstyle -s ':prompt:colors' b_magenta M
				local indicator=""
				[[ -n "$(git status --short -uno 2> "/dev/null" | tail -n1)" ]] && indicator="${Y}*"
				echo "❮${R}󰊢 ${M}${branch}${indicator}${y}❯"
			fi
		fi
	}

	local z
	zstyle -s ':prompt:colors' reset z

	local k K r R g G y Y b B m M c C w W
	zstyle -s ':prompt:colors' n_black   k; zstyle -s ':prompt:colors' b_gray    K
	zstyle -s ':prompt:colors' n_red     r; zstyle -s ':prompt:colors' b_red     R
	zstyle -s ':prompt:colors' n_green   g; zstyle -s ':prompt:colors' b_green   G
	zstyle -s ':prompt:colors' n_yellow  y; zstyle -s ':prompt:colors' b_yellow  Y
	zstyle -s ':prompt:colors' n_blue    b; zstyle -s ':prompt:colors' b_blue    B
	zstyle -s ':prompt:colors' n_magenta m; zstyle -s ':prompt:colors' b_magenta M
	zstyle -s ':prompt:colors' n_cyan    c; zstyle -s ':prompt:colors' b_cyan    C
	zstyle -s ':prompt:colors' n_white   w; zstyle -s ':prompt:colors' b_white   W

	local u
	if [ "$(id -u)" -eq 0 ]; then
		zstyle -s ':prompt:colors' b_red u
	else
		zstyle -s ':prompt:colors' b_green u
	fi

	local os_version="$(freebsd-version)"
	local sh_name="$ZSH_NAME"

	export PROMPT="
${y}${R} ${M}${os_version}${y}─${B} ${M}${sh_name}${y}
${y}┌──❮ ${G} %*${y} ❯─❮ ${G} %D{%d/%m/%y}${y} ❯─❮ ${Y} ${C}%c${y} ❯─ ❮${B} ${u}%n${y}❯ \$(git_branch)
${y}└─${B}${z} "
}

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

# Commands ALIAS
alias clear='printf "\e[H\e[2J\e[3J"'
# Packages ALIAS
alias uppkg='sudo pkg update && sudo pkg upgrade --yes'
alias upall='uppkg'

### ################################
### SHELL CONFIGURATION
### ################################
EOF

### ################################################################################################################################

### ################################
### Installing Needed Tools (User)
### ################################

# Man Pages
sudo pkg install --yes mandoc

# Build System
sudo pkg install --yes cmake
sudo pkg install --yes ninja
sudo pkg install --yes meson

# Compress and Decompress
sudo pkg install --yes zip
sudo pkg install --yes unzip
sudo pkg install --yes 7-zip

# Cryptography
sudo pkg install --yes gnupg
sudo pkg install --yes openssl
sudo pkg install --yes libressl

### ################################
### Installing Rust Tools (User)
### ################################

# New Tools
sudo pkg install --yes eza
sudo pkg install --yes fd-find
sudo pkg install --yes bat
sudo pkg install --yes eza
sudo pkg install --yes grex
sudo pkg install --yes ripgrep

### ################################
### Installing System Fetch (User)
### ################################

# Fetch
sudo pkg install --yes neofetch
sudo pkg install --yes fastfetch
sudo pkg install --yes ufetch
sudo pkg install --yes pfetch-rs
sudo pkg install --yes cpufetch

### ################################
### Installing Web/Net Tools (User)
### ################################

# Browser
touch "${HOME}/.w3m/history"
sudo pkg install --yes w3m
sudo pkg install --yes lynx
sudo pkg install --yes elinks

# NetCat
sudo pkg install --yes netcat

### ################################
### Installing TreeSitter (User)
### ################################

# TreeSitter
sudo pkg install --yes tree-sitter
sudo pkg install --yes tree-sitter-cli
sudo pkg install --yes tree-sitter-grammars
sudo pkg install --yes tree-sitter-graph

### ################################################################################################################################

### ################################
### Installing Editor (User)
### ################################

# Terminal Editor
sudo pkg install --yes emacs-nox
sudo pkg install --yes neovim
sudo pkg install --yes vim
sudo pkg install --yes helix
sudo pkg install --yes micro
sudo pkg install --yes mg
sudo pkg install --yes nano

### ################################
### Installing Git Config (User)
### ################################

# Vim
git clone "https://github.com/GabrielFrigo4/vimfiles.git" "${HOME}/vimfiles"
cat << 'EOF' | tee "${HOME}/.vimrc" > "/dev/null"
set rtp+=~/vimfiles
source ~/vimfiles/vimrc
EOF
# Helix
git clone "https://github.com/GabrielFrigo4/helix.git" "${HOME}/.config/helix"

### ################################
### Updating Git Config (User)
### ################################

# Update
cd "${HOME}/vimfiles"
git pull
cd "${HOME}/.config/helix"
git pull
cd "${HOME}"

### ################################
### Setup Emacs Config (User)
### ################################

# Remove Lixo
rm -rf "${HOME}/.emacs" 2> "/dev/null"
rm -rf "${HOME}/.emacs.d" 2> "/dev/null"
rm -rf "${HOME}/.config/emacs" 2> "/dev/null"
rm -rf "${HOME}/.config/doom" 2> "/dev/null"

# Setup Doom Emacs
git clone --depth 1 "https://github.com/doomemacs/doomemacs" "${HOME}/.config/emacs"
mkdir -p "${HOME}/.config/doom/snippets"
~/.config/emacs/bin/doom install --force

# Setup Packages
cat << 'EOF' | tee -a "${HOME}/.config/doom/packages.el" > "/dev/null"
(package! mermaid-mode)
(package! ob-mermaid)
EOF
~/.config/emacs/bin/doom sync

# Setup init.el
sed -i 's/;;tree-sitter/tree-sitter/' "${HOME}/.config/doom/init.el"
sed -i 's/;;(cc +lsp)/(cc +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;(rust +lsp)/(rust +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;python/(python +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;javascript/(javascript +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;typescript/(typescript +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;toml/(toml +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/;;sql/(sql +lsp +tree-sitter)/' "${HOME}/.config/doom/init.el"
sed -i 's/sh[[:space:]]*;/(sh +tree-sitter) ;/' "${HOME}/.config/doom/init.el"
~/.config/emacs/bin/doom sync

# Setup config.el
cat << 'EOF' | tee -a "${HOME}/.config/doom/config.el" > "/dev/null"
;; Ativar Cursor Piscante
(blink-cursor-mode t)

;; Configuração Mermaid
(use-package! mermaid-mode
  :mode "\\.mermaid\\'"
  :mode "\\.mmd\\'"
  :config
  (setq mermaid-mmdc-location "mmdc")
  (setq mermaid-output-format "png"))

(use-package! ob-mermaid
  :after org
  :config
  (setq ob-mermaid-cli-path "mmdc"))
EOF
~/.config/emacs/bin/doom sync

# Update Doom Emacs
~/.config/emacs/bin/doom upgrade

### ################################
### Setup NeoVim Config (User)
### ################################

# Remove Lixo
rm -rf "${HOME}/.config/nvim" 2> "/dev/null"

# Setup LazyVim
git clone "https://github.com/LazyVim/starter" "${HOME}/.config/nvim"
rm -rf "${HOME}/.config/nvim/.git"

# Setup options.lua
cat << 'EOF' | tee -a "${HOME}/.config/nvim/lua/config/options.lua" > "/dev/null"
-- Ativar Cursor Piscante
local cursor_gui = vim.api.nvim_get_option_value("guicursor", {})
local cursor_group = vim.api.nvim_create_augroup('ConfigCursor', { clear = true })
vim.api.nvim_create_autocmd({ 'VimEnter', 'VimResume' }, {
	group = cursor_group,
	pattern = '*',
	command = 'set guicursor=' .. cursor_gui .. ',a:blinkwait500-blinkoff500-blinkon500-Cursor/lCursor'
})
vim.api.nvim_create_autocmd({ 'VimLeave', 'VimSuspend' }, {
	group = cursor_group,
	pattern = '*',
	command = 'set guicursor='
})
EOF

### ################################
### Setup Micro Config (User)
### ################################

### https://draculatheme.com/micro
git clone "https://github.com/dracula/micro.git"
mkdir -p "${HOME}/.config/micro/colorschemes"
cp "micro/dracula.micro" "${HOME}/.config/micro/colorschemes/dracula.micro"
sudo rm -f -r micro
cat << 'EOF' | tee "${HOME}/.config/micro/settings.json" > "/dev/null"
{
	"colorscheme": "dracula"
}
EOF

### ################################################################################################################################

### ################################
### Installing Languages (User)
### ################################

# C/C++
sudo pkg install --yes gcc
sudo pkg install --yes llvm

# Assembly
sudo pkg install --yes nasm
sudo pkg install --yes fasm

# Python
sudo pkg install --yes python

# Lua
sudo pkg install --yes lua54 lua54-luarocks
sudo pkg install --yes lua53 lua53-luarocks
sudo pkg install --yes lua52 lua52-luarocks
sudo pkg install --yes lua51 lua51-luarocks
sudo pkg install --yes luajit

### ################################################################################################################################
