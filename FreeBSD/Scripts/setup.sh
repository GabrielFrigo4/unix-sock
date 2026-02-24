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
sysrc kld_list+="vmmw_guest_kmod"

# KVM/QEMU
pkg install --yes qemu-guest-agent
pkg install --yes drm-kmod
sysrc qemu_guest_agent_enable="YES"
sysrc spice_vdagentd_enable="YES"
sysrc kld_list+="virtio_gpu"

# TERMINAL
sysrc allscreens_flags="-f spleen-16x32"

# SUDO
pkg install --yes sudo
cat << 'EOF' | tee "/usr/local/etc/sudoers.d/wheel" > "/dev/null"
%wheel ALL=(ALL:ALL) ALL
EOF
chmod 0440 "/usr/local/etc/sudoers.d/wheel"

# DOAS
pkg install --yes doas
cat << 'EOF' | tee "/usr/local/etc/doas.conf" > "/dev/null"
permit persist :wheel
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
	CURRENT_LIST=$(gsettings get "org.gnome.settings-daemon.plugins.media-keys" custom-keybindings)

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
create_launcher 1   "Launch Terminal"       "kgx"                   "<Control><Alt>t"
create_launcher 2   "Launch Emacs"          "emacs"                 "<Control><Alt>e"

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

# GITHUB
sudo pkg install --yes gh
gh auth login

# SETUP
git config --global credential.helper oauth
git config --global user.email "gabriel.frigo4@gmail.com"
git config --global user.name "Gabriel Frigo"
git config --global pull.rebase false

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
KGX_BLOCK="$(mktemp)"
cat << 'EOF' > "${KGX_BLOCK}"
### ################################
### KGX ENVIRONMENT
### ################################

if [ -z "${KGX_INIT}" ]; then
	if [ -z "${KGX_SHELL}" ]; then
		KGX_SHELL="$(which zsh)"
	fi

	if [ -x "${KGX_SHELL}" ]; then
		export SHELL="${KGX_SHELL}"
		unset KGX_INIT KGX_SHELL
		clear
		exec "${SHELL}"
	else
		unset KGX_INIT KGX_SHELL
	fi
fi
EOF
touch "${HOME}/.shrc"
cat "${KGX_BLOCK}" "${HOME}/.shrc" > "${HOME}/.shrc.tmp" && mv "${HOME}/.shrc.tmp" "${HOME}/.shrc"
sudo touch "/root/.shrc"
sudo cat "${KGX_BLOCK}" "/root/.shrc" | sudo tee "/root/.shrc.tmp" > /dev/null && sudo mv "/root/.shrc.tmp" "/root/.shrc"
rm "${KGX_BLOCK}"

# Config Shell
cat << 'EOF' | tee -a "${HOME}/.shrc" | sudo tee -a "/root/.shrc" > "/dev/null"
### ################################
### SHELL ENVIRONMENT
### ################################

export KGX_INIT=1

export C_RED='\[\e[1;31m\]'
export C_GREEN='\[\e[1;32m\]'
export C_YELLOW='\[\e[1;33m\]'
export C_BLUE='\[\e[1;34m\]'
export C_PURPLE='\[\e[1;35m\]'
export C_CYAN='\[\e[1;36m\]'
export C_GRAY='\[\e[1;30m\]'
export C_RESET='\[\e[0m\]'

update_prompt() {
	local branch="$(command git symbolic-ref --short HEAD 2>/dev/null || command git rev-parse --short HEAD 2>/dev/null)"
	local git_info=" "

	if [ -n "${branch}" ]; then
		git_info=" ${C_BLUE}(${C_RED}${branch}${C_BLUE})${C_RESET} "
	fi

	if [ "$(id -u)" -eq 0 ]; then
		usr_color="${C_RED}"
	else
		usr_color="${C_GREEN}"
	fi

	export PS1="${usr_color}\u${C_BLUE}@${C_PURPLE}\h${C_GRAY}:${C_GRAY}[${C_YELLOW}\w${C_GRAY}]${C_RESET}${git_info}${C_CYAN}\$${C_RESET} "
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

cd()    { run_and_update cd "$@"; }
rm()    { run_and_update rm "$@"; }
rmdir() { run_and_update rmdir "$@"; }
git()   { run_and_update git "$@"; }
gh()    { run_and_update gh "$@"; }
wget()  { run_and_update wget "$@"; }
curl()  { run_and_update curl "$@"; }
unzip() { run_and_update unzip "$@"; }
tar()   { run_and_update tar "$@"; }
7z()    { run_and_update 7z "$@"; }

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

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

export KGX_INIT=1

git_branch() {
	local branch="$(git symbolic-ref --short HEAD 2>/dev/null || git rev-parse --short HEAD 2>/dev/null)"
	if [ -n "${branch}" ]; then
		echo "${branch}"
	fi
}

show_git_branch() {
	if git rev-parse --is-inside-work-tree &>/dev/null; then
		local branch="$(git_branch)"
		if [ -n "${branch}" ]; then
			echo "❮\[\e[1;31m\]󰊢 \[\e[1;35m\]${branch}\[\e[0;33m\]❯"
		fi
	fi
}

os_version=$(freebsd-version)
sh_name=$(ps -p $$ -o comm=)
if [ "$(id -u)" -eq 0 ]; then
	usr_color="\[\e[1;31m\]"
else
	usr_color="\[\e[1;32m\]"
fi
update_prompt() {
	PS1="\n\[\e[0;33m\]\[\e[1;31m\] \[\e[1;35m\]${os_version}\[\e[0;33m\]─\[\e[1;34m\] \[\e[1;35m\]${sh_name}\[\e[0;33m\]"
	PS1+="\n\[\e[0;33m\]┌──❮ \[\e[1;32m\] \t\[\e[0;33m\] ❯─❮ \[\e[1;32m\] \D{%d/%m/%y}\[\e[0;33m\] ❯─❮ \[\e[1;33m\] \[\e[1;36m\]\W\[\e[0;33m\] ❯─ ❮\[\e[1;34m\] ${usr_color}\u\[\e[0;33m\]❯ $(show_git_branch)"
	PS1+="\n\[\e[0;33m\]└─\[\e[1;34m\]\[\e[0m\] "
}
PROMPT_COMMAND=update_prompt

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

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

# History OPTIONS
setopt SHARE_HISTORY
setopt HIST_IGNORE_DUPS
setopt HIST_IGNORE_SPACE
setopt HIST_REDUCE_BLANKS

# Globbing & Expansion OPTIONS
setopt EXTENDED_GLOB
setopt GLOB_DOTS
setopt PROMPT_SUBST

# Interaction OPTIONS
setopt CORRECT
setopt INTERACTIVE_COMMENTS
unsetopt BEEP

# Navigation OPTIONS
setopt AUTO_CD

### ################################
### SHELL ENVIRONMENT
### ################################

export KGX_INIT=1

git_branch() {
	local branch="$(git symbolic-ref --short HEAD 2>/dev/null || git rev-parse --short HEAD 2>/dev/null)"
	if [ -n "${branch}" ]; then
		echo "${branch}"
	fi
}

show_git_branch() {
	if git rev-parse --is-inside-work-tree &>/dev/null; then
		local branch="$(git_branch)"
		if [ -n "${branch}" ]; then
			echo "❮%B%F{red}󰊢 %F{magenta}${branch}%b%F{yellow}❯"
		fi
	fi
}

os_version=$(freebsd-version)
sh_name=$(ps -p $$ -o comm=)
if [ "$(id -u)" -eq 0 ]; then
	usr_color="%B%F{red}"
else
	usr_color="%B%F{green}"
fi
export PROMPT=$'
%b%F{yellow}%B%F{red} %F{magenta}${os_version}%b%F{yellow}─%B%F{blue} %F{magenta}${sh_name}%b%F{yellow}
%b%F{yellow}┌──❮ %B%F{green} %*%b%F{yellow} ❯─❮ %B%F{green} %D{%d/%m/%y}%b%F{yellow} ❯─❮ %B%F{yellow} %B%F{cyan}%c%b%F{yellow} ❯─ ❮%B%F{blue} ${usr_color}%n%b%F{yellow}❯ \$(show_git_branch)
%b%F{yellow}└─%B%F{blue}%f%b '

### ################################
### SHELL FUNCTIONS
### ################################

### ################################
### SHELL ALIAS
### ################################

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

### ################################################################################################################################
