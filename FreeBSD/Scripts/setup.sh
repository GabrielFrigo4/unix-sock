#!/bin/sh

### ################################################################################################################################

### ################################
### FreeBSD Handbook
### ################################

### https://docs.freebsd.org/en/books/handbook/

### ################################
### FreeBSD Developers' Handbook
### ################################

### https://docs.freebsd.org/en/books/developers-handbook/

### ################################
### FreeBSD FAQs
### ################################

### https://docs.freebsd.org/en/books/faq/

### ################################################################################################################################

### ################################
### Setup System
### ################################

# GROUPS
pw groupmod wheel -m gabriel
pw groupmod video -m gabriel

# PACKAGE
pkg bootstrap --yes
pkg update
pkg upgrade --yes

# VMWARE
pkg install --yes open-vm-tools
pkg install --yes xf86-video-vmware
pkg install --yes xf86-input-vmmouse
sysrc vmware_guest_kmod_enable="YES"
sysrc vmware_guestd_enable="YES"

# KVM/QEMU
pkg install --yes xf86-video-qxl
pkg install --yes qemu-guest-agent
sysrc qemu_guest_agent_enable="YES"
sysrc spice_vdagentd_enable="YES"

# UEFI
pkg install --yes xf86-video-scfb

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
### Setup Environment
### ################################

# DESKTOP
sudo pkg install --yes desktop-installer
sudo desktop-installer

# GNOME X11
sudo mv /usr/local/share/xsessions/gnome-classic.desktop /usr/local/share/xsessions/gnome-classic.desktop.bak
sudo mv /usr/local/share/xsessions/gnome-classic-xorg.desktop /usr/local/share/xsessions/gnome-classic-xorg.desktop.bak

# GNOME WAYLAND
sudo mv /usr/local/share/wayland-sessions/gnome-classic.desktop /usr/local/share/wayland-sessions/gnome-classic.desktop.bak
sudo mv /usr/local/share/wayland-sessions/gnome-classic-wayland.desktop /usr/local/share/wayland-sessions/gnome-classic-wayland.desktop.bak

### ################################
### System Environment
### ################################

# SHORTCUT
gsettings set "org.gnome.desktop.wm.keybindings"                "show-desktop"  "[]"
gsettings set "org.gnome.settings-daemon.plugins.media-keys"    "home"          "['<Control><Alt>h']"
gsettings set "org.gnome.settings-daemon.plugins.media-keys"    "calculator"    "['<Control><Alt>c']"
gsettings set "org.gnome.settings-daemon.plugins.media-keys"    "www"           "['<Control><Alt>g']"
gsettings set "org.gnome.settings-daemon.plugins.media-keys"    "search"        "['<Control><Alt>f']"

### ################################
### Custom Settings
### ################################

# CREATE LAUNCHER FUNCTION
create_launcher() {
	local INDEX="$1"
	local NAME="$2"
	local COMMAND="$3"
	local BINDING="$4"
	local KEY_PATH="/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/custom${INDEX}/"

	gsettings set "org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$KEY_PATH" "name" "$NAME"
	gsettings set "org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$KEY_PATH" "command" "$COMMAND"
	gsettings set "org.gnome.settings-daemon.plugins.media-keys.custom-keybinding:$KEY_PATH" "binding" "$BINDING"

	local CURRENT_LIST
	CURRENT_LIST="$(gsettings get "org.gnome.settings-daemon.plugins.media-keys" custom-keybindings)"

	if [[ "$CURRENT_LIST" != *"$KEY_PATH"* ]]; then
		local NEW_LIST
		if [ "$CURRENT_LIST" = "@as []" ]; then
			NEW_LIST="['$KEY_PATH']"
		else
			NEW_LIST="${CURRENT_LIST%]}, '$KEY_PATH']"
		fi
		gsettings set "org.gnome.settings-daemon.plugins.media-keys" custom-keybindings "$NEW_LIST"
		echo "✅ $NAME added successfully."
	else
		echo "ℹ️  $NAME was already configured."
	fi
}

# CREATE LAUNCHERS
create_launcher 0   "Launch Settings"       "gnome-control-center"  "<Control><Alt>s"
create_launcher 1   "Launch Console"        "kgx"                   "<Control><Alt>t"
create_launcher 2   "Launch Terminal"       "ptyxis"                "<Control><Alt>y"
create_launcher 3   "Launch Emacs"          "emacs"                 "<Control><Alt>e"

### ################################################################################################################################

### ################################
### Setup Wget
### ################################

# WGET
sudo pkg install --yes wget
sudo pkg install --yes wget2
sudo pkg install --yes curl

### ################################
### Setup Git
### ################################

# GIT
sudo pkg install --yes git
sudo pkg install --yes git-credential-oauth

# SETUP
rm "${HOME}/.gitconfig"
git config --global credential.helper "!gh auth git-credential"
git config --global user.email "gabriel.frigo4@gmail.com"
git config --global user.name "Gabriel Frigo"
git config --global init.defaultBranch "main"
git config --global pull.rebase false
git config --global color.ui auto

# GITHUB
sudo pkg install --yes gh
gh auth login
gh auth setup-git

### ################################
### Setup Ports
### ################################

# PORTS
sudo git clone "https://git.FreeBSD.org/ports.git" "/usr/ports"

# UPDATE
cd "/usr/ports"
sudo git pull
cd ~

### ################################################################################################################################

### ################################
### Setup Shell
### ################################

# Default Shell
sudo chsh -s "$(which sh)" "$(whoami)"
sudo chsh -s "$(which sh)" "root"

# Kgx Config
SHELL_BLOCK="$(mktemp)"
cat << 'EOF' > "${SHELL_BLOCK}"
### ################################
### TERMINAL ENVIRONMENT
### ################################

if [ -z "${SHELL_INIT}" ]; then
	if [ -z "${KGX_SHELL}" ]; then
		KGX_SHELL="$(which zsh)"
	fi

	if [ -x "${KGX_SHELL}" ]; then
		export SHELL="${KGX_SHELL}"
		unset SHELL_INIT KGX_SHELL
		printf "\033[H\033[2J\033[3J"
		exec "${SHELL}"
	else
		unset SHELL_INIT KGX_SHELL
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
### SHELL ENVIRONMENT
### ################################

export SHELL_INIT=1

### ################################
### SHELL APPEARANCE
### ################################

git_branch() {
	_git_branch=" "
	if command git rev-parse --is-inside-work-tree > "/dev/null" 2>&1; then
		local branch="$(command git branch --show-current 2> "/dev/null" || command git rev-parse --short HEAD 2> "/dev/null")"
		if [ -n "$branch" ]; then
			local is_dirty="$(command git status --short -uno 2> "/dev/null" | tail -n1)"
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

	local cur_user="${USER:-$(command id -un)}"
	local cur_host="$(command hostname -s)"
	local cur_dir="${PWD##*/}"
	[ "${PWD}" = "${HOME}" ] && cur_dir="~"
	[ "${PWD}" = "/" ] && cur_dir="/"

	export PS1="${u}${cur_user}${B}@${M}${cur_host}${K}:${K}[${Y}${cur_dir}${K}]${z}${_git_branch}${C}\$${z} "
}

update_prompt

run_and_update() {
	local cmd="$1"
	shift
	command "$cmd" "$@"
	local ret=$?
	update_prompt
	return $ret
}

TRIGGERS="cd git gh"
TRIGGERS="$TRIGGERS pkg make cmake ninja cc clang gcc"
TRIGGERS="$TRIGGERS vi vim nvim emacs nano micro edit ee"
TRIGGERS="$TRIGGERS touch cp mv rm rmdir mkdir chmod chown ln"
TRIGGERS="$TRIGGERS wget curl tar 7z unzip ssh"

for cmd in $TRIGGERS; do
	unalias "$cmd" 2> "/dev/null"
	eval "${cmd}() { run_and_update ${cmd} \"\$@\"; }"
done

alias c++="run_and_update c++"
alias clang++="run_and_update clang++"
alias g++="run_and_update g++"
alias :="update_prompt; command :"

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

# Commands ALIAS
alias clear='printf "\033[H\033[2J\033[3J"'
# Packages ALIAS
alias uppkg='sudo pkg update && sudo pkg upgrade --yes'
alias upall='uppkg'

### ################################
### SHELL CONFIGURATION
### ################################
EOF

### ################################
### Setup Bash
### ################################

# Install Bash
sudo pkg install --yes bash

# Config Bash
cat << 'EOF' | tee -a "${HOME}/.bashrc" | sudo tee -a "/root/.bashrc" > "/dev/null"
### ################################
### SHELL ENVIRONMENT
### ################################

export SHELL_INIT=1

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
### Setup Zsh
### ################################

# Install Zsh
sudo pkg install --yes zsh
curl -fsSL "https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh" | sh -s -- --unattended
curl -fsSL "https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh" | sudo sh -s -- --unattended

# Config Zsh
cat << 'EOF' | tee -a "${HOME}/.zshrc" | sudo tee -a "/root/.zshrc" > "/dev/null"
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

export SHELL_INIT=1

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
### Installing System Fonts
### ################################

sudo pkg install --yes fontconfig
mkdir -p "${HOME}/.local/share/fonts"

### ################################
### RobotoMono Nerd Fonts
### ################################

# https://www.nerdfonts.com/font-downloads
wget "https://github.com/ryanoasis/nerd-fonts/releases/latest/download/RobotoMono.zip" -O "RobotoMono.zip"
unzip -o RobotoMono.zip -d "${HOME}/.local/share/fonts"
rm -f "${HOME}/.local/share/fonts/LICENSE.txt"
rm -f "${HOME}/.local/share/fonts/README.md"
rm -f RobotoMono.zip

### ################################
### JetBrains Nerd Fonts
### ################################

# https://www.nerdfonts.com/font-downloads
wget "https://github.com/ryanoasis/nerd-fonts/releases/latest/download/JetBrainsMono.zip" -O "JetBrainsMono.zip"
unzip -o JetBrainsMono.zip -d "${HOME}/.local/share/fonts"
rm -f "${HOME}/.local/share/fonts/OFL.txt"
rm -f "${HOME}/.local/share/fonts/README.md"
rm -f JetBrainsMono.zip

### ################################
### MesloLGS Nerd Fonts
### ################################

# https://github.com/romkatv/powerlevel10k#meslo-nerd-font-patched-for-powerlevel10k
# MesloLGS NF Regular
wget "https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Regular.ttf" -O "MesloLGS NF Regular.ttf"
mv "MesloLGS NF Regular.ttf" "${HOME}/.local/share/fonts"
# MesloLGS NF Bold
wget "https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Bold.ttf" -O "MesloLGS NF Bold.ttf"
mv "MesloLGS NF Bold.ttf" "${HOME}/.local/share/fonts"
# MesloLGS NF Italic
wget "https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Italic.ttf" -O "MesloLGS NF Italic.ttf"
mv "MesloLGS NF Italic.ttf" "${HOME}/.local/share/fonts"
# MesloLGS NF Bold Italic
wget "https://github.com/romkatv/powerlevel10k-media/raw/master/MesloLGS%20NF%20Bold%20Italic.ttf" -O "MesloLGS NF Bold Italic.ttf"
mv "MesloLGS NF Bold Italic.ttf" "${HOME}/.local/share/fonts"

### ################################
### JetBrains Mono Nerd Fonts
### ################################

# https://github.com/JetBrains/JetBrainsMono
sh -c "$(curl -fsSL https://raw.githubusercontent.com/JetBrains/JetBrainsMono/master/install_manual.sh)"

### ################################
### Nerd Font Symbols Only
### ################################

# https://www.nerdfonts.com/font-downloads
wget "https://github.com/ryanoasis/nerd-fonts/releases/latest/download/NerdFontsSymbolsOnly.zip" -O "NerdFontsSymbolsOnly.zip"
unzip -o "NerdFontsSymbolsOnly.zip" -d "${HOME}/.local/share/fonts"
rm -f "${HOME}/.local/share/fonts/10-nerd-font-symbols.conf"
rm -f "${HOME}/.local/share/fonts/LICENSE"
rm -f "${HOME}/.local/share/fonts/README.md"
rm "NerdFontsSymbolsOnly.zip"

### ################################
### Update Font Cache
### ################################

fc-cache -fv

### ################################################################################################################################

### ################################
### Installing Needed Tools
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

### ################################
### Installing Rust Tools
### ################################

# New Tools
sudo pkg install --yes eza
sudo pkg install --yes fd-find
sudo pkg install --yes bat
sudo pkg install --yes eza
sudo pkg install --yes grex
sudo pkg install --yes ripgrep

### ################################
### Installing System Fetch
### ################################

# Fetch
sudo pkg install --yes neofetch
sudo pkg install --yes fastfetch
sudo pkg install --yes ufetch
sudo pkg install --yes pfetch-rs
sudo pkg install --yes cpufetch

### ################################
### Installing System Tools
### ################################

# Clipboard
sudo pkg install --yes wl-clipboard
sudo pkg install --yes xclip

### ################################
### Installing Web/Net Tools
### ################################

# Browser
touch "${HOME}/.w3m/history"
sudo pkg install --yes w3m
sudo pkg install --yes lynx
sudo pkg install --yes elinks

# NetCat
sudo pkg install --yes netcat

### ################################
### Installing treeSitter
### ################################

sudo pkg install --yes tree-sitter
sudo pkg install --yes tree-sitter-cli
sudo pkg install --yes tree-sitter-grammars
sudo pkg install --yes tree-sitter-graph

### ################################################################################################################################

### ################################
### Installing Terminal Editor
### ################################

# Terminal Editor
sudo pkg install --yes micro
sudo pkg install --yes nano
sudo pkg install --yes neovim

### ################################
### Installing Window Editor
### ################################

# Window Editor
sudo pkg install --yes emacs

### ################################
### Setup Emacs Config
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
;; Configuração de Fonte (JetBrains Mono)
(setq doom-font (font-spec :family "JetBrainsMonoNL Nerd Font Mono" :size 16 :weight 'medium)
      doom-variable-pitch-font (font-spec :family "JetBrainsMonoNL Nerd Font Mono" :size 16))
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
### Setup NeoVim Config
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
### Installing Theme in Micro
### ################################

### https://draculatheme.com/micro
git clone "https://github.com/dracula/micro.git"
mkdir -p "${HOME}/.config/micro/colorschemes"
cp "micro/dracula.micro" "${HOME}/.config/micro/colorschemes/dracula.micro"
sudo rm -f -r micro
cat << 'EOF' > "${HOME}/.config/micro/settings.json"
{
	"colorscheme": "dracula"
}
EOF

### ################################################################################################################################

### ################################
### Installing Languages
### ################################

# C/C++
sudo pkg install --yes gcc

# Python
sudo pkg install --yes python

### ################################################################################################################################

# Web Browser
sudo pkg install --yes firefox

# Terminal
sudo pkg install --yes ptyxis

### ################################################################################################################################
