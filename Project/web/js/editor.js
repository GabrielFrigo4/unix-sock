const UI = {
	fileList: document.getElementById("fileList"),
	editor: document.getElementById("meuEditor"),
	currentFileSpan: document.getElementById("currentFile"),
	btnSave: document.getElementById("btnSalvar"),
	btnDelete: document.getElementById("btnExcluir"),
	btnNew: document.getElementById("btnNovo"),
	inputNew: document.getElementById("newFileName"),
	statusMsg: document.getElementById("statusMensagem"),
	loginOverlay: document.getElementById("loginOverlay"),
	loginError: document.getElementById("loginError"),
	btnLogin: document.getElementById("btnLogin"),
	inputUser: document.getElementById("adminUser"),
	inputPass: document.getElementById("adminPass"),
};

const State = {
	currentFile: null,
};

const getToken = () => sessionStorage.getItem("admin_token");
const setToken = (token) => sessionStorage.setItem("admin_token", token);
const clearToken = () => sessionStorage.removeItem("admin_token");

const authHeaders = () => ({
	Authorization: `Bearer ${getToken()}`,
});

const showLoginForm = () => {
	UI.loginOverlay.style.display = "flex";
	UI.loginError.textContent = "";
};

const hideLoginForm = () => {
	UI.loginOverlay.style.display = "none";
};

const handleUnauthorized = (res) => {
	if (res.status === 401) {
		clearToken();
		showLoginForm();
		return true;
	}
	return false;
};

const doLogin = async () => {
	const user = UI.inputUser.value.trim();
	const pass = UI.inputPass.value;

	if (!user || !pass) {
		UI.loginError.textContent = "Preencha todos os campos.";
		return;
	}

	try {
		const res = await fetch("/api/auth/login", {
			method: "POST",
			headers: { "Content-Type": "application/json" },
			body: JSON.stringify({ user, pass }),
		});

		if (!res.ok) {
			UI.loginError.textContent = "Credenciais inválidas.";
			return;
		}

		const data = await res.json();
		setToken(data.token);
		hideLoginForm();
		refreshFileList();
	} catch (e) {
		UI.loginError.textContent = "Erro de conexão.";
	}
};

const refreshFileList = async () => {
	if (!getToken()) {
		showLoginForm();
		return;
	}

	try {
		const res = await fetch("/api/files", { headers: authHeaders() });
		if (handleUnauthorized(res)) return;

		const files = await res.json();

		if (files.length === 0) {
			UI.fileList.innerHTML = "<li>Nenhum arquivo encontrado</li>";
			return;
		}

		UI.fileList.innerHTML = files
			.map((file) => {
				const activeClass = file === State.currentFile ? 'class="ativo"' : "";
				return `<li ${activeClass} onclick="openFile('${file}')">${file}</li>`;
			})
			.join("");
	} catch (e) {
		console.error("Erro ao listar arquivos:", e);
	}
};

const showStatus = (message, type) => {
	UI.statusMsg.textContent = message;
	UI.statusMsg.className = `status-visivel ${type}`;
	setTimeout(() => {
		UI.statusMsg.className = "status-oculto";
	}, 3000);
};

window.openFile = async (fileName) => {
	if (!getToken()) return showLoginForm();

	State.currentFile = fileName;
	UI.currentFileSpan.textContent = fileName;
	UI.editor.disabled = false;
	UI.btnSave.disabled = false;
	UI.btnDelete.disabled = false;
	UI.editor.placeholder = "Carregando...";

	const res = await fetch(`/api/data/${fileName}`, { headers: authHeaders() });
	if (handleUnauthorized(res)) return;

	const text = res.ok ? await res.text() : "";

	UI.editor.value = text;
	UI.editor.placeholder = "Digite aqui...";
	refreshFileList();
};

const saveCurrentFile = async () => {
	if (!State.currentFile || UI.btnSave.disabled || !getToken()) return;

	UI.btnSave.textContent = "Salvando...";
	UI.btnSave.disabled = true;

	const res = await fetch(`/api/data/${State.currentFile}`, {
		method: "PUT",
		headers: { "Content-Type": "text/plain", ...authHeaders() },
		body: UI.editor.value,
	});

	if (handleUnauthorized(res)) return;

	const success = res.ok;
	showStatus(
		success ? "Salvo com sucesso!" : "Erro ao salvar.",
		success ? "sucesso" : "erro",
	);

	UI.btnSave.textContent = "Salvar Arquivo";
	UI.btnSave.disabled = false;
};

UI.btnSave.onclick = saveCurrentFile;

UI.btnNew.onclick = async () => {
	const rawName = UI.inputNew.value.trim();
	if (!rawName || !getToken()) return;

	const fileName = rawName.includes(".") ? rawName : `${rawName}.txt`;

	await fetch(`/api/data/${fileName}`, {
		method: "POST",
		headers: authHeaders(),
		body: "",
	});

	UI.inputNew.value = "";
	window.openFile(fileName);
};

UI.btnDelete.onclick = async () => {
	if (!State.currentFile || !confirm(`Tem certeza que deseja excluir ${State.currentFile}?`))
		return;

	UI.btnDelete.disabled = true;
	const res = await fetch(`/api/data/${State.currentFile}`, {
		method: "DELETE",
		headers: authHeaders(),
	});

	if (handleUnauthorized(res)) return;

	if (res.ok) {
		State.currentFile = null;
		UI.currentFileSpan.textContent = "Nenhum arquivo selecionado";
		UI.editor.value = "";
		UI.editor.disabled = true;
		UI.btnSave.disabled = true;
		UI.btnDelete.disabled = true;
		showStatus("Excluído com sucesso!", "sucesso");
		refreshFileList();
	} else {
		showStatus("Erro ao excluir.", "erro");
		UI.btnDelete.disabled = false;
	}
};

UI.btnLogin.onclick = doLogin;
UI.inputPass.addEventListener("keydown", (event) => {
	if (event.key === "Enter") doLogin();
});

window.addEventListener("keydown", (event) => {
	if ((event.ctrlKey || event.metaKey) && event.key === "s") {
		event.preventDefault();
		saveCurrentFile();
	}
});

window.onload = () => {
	if (getToken()) {
		hideLoginForm();
		refreshFileList();
	} else {
		showLoginForm();
	}
};

setInterval(refreshFileList, 2048);
