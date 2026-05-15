const Config = {
	roomId: sessionStorage.getItem("sala"),
	playerName: sessionStorage.getItem("jogador"),
	playerSymbol: sessionStorage.getItem("simbolo"),
};

if (!Config.roomId || !Config.playerName || !Config.playerSymbol) {
	window.location.href = "/";
}

const UI = {
	roomLabel: document.getElementById("lblSala"),
	playerLabel: document.getElementById("lblJogador"),
	symbolLabel: document.getElementById("lblSimbolo"),
	statusBanner: document.getElementById("statusTurno"),
	cells: document.querySelectorAll(".cell"),
	tempoBanner: document.getElementById("statusTempo"),
	tempoLabel: document.getElementById("lblTempo"),
};

UI.roomLabel.textContent = Config.roomId;
UI.playerLabel.textContent = Config.playerName;
UI.symbolLabel.textContent = Config.playerSymbol;

const State = {
	gameData: null,
	isUpdating: false,
};

const renderGameOver = () => {
	const { winner, players } = State.gameData;
	if (winner === "Empate") {
		UI.statusBanner.className = "status-banner empate";
		UI.statusBanner.textContent = "Deu Velha! (Empate)";
	} else if (winner === Config.playerSymbol) {
		UI.statusBanner.className = "status-banner vitoria";
		UI.statusBanner.textContent = `Vitória! Parabéns, ${Config.playerName}! 🎉`;
	} else {
		UI.statusBanner.className = "status-banner derrota";
		const winnerName = winner === "X" ? players.X : players.O;
		UI.statusBanner.textContent = `Derrota! Vitória de ${winnerName} 😢`;
	}
};

const updateInterface = () => {
	if (!State.gameData) return;

	UI.cells.forEach((cell, index) => {
		const value = State.gameData.board[index];
		cell.textContent = value;
		cell.className = `cell ${value}`;
	});

	if (State.gameData.winner) return renderGameOver();

	const isMyTurn = State.gameData.turn === Config.playerSymbol;
	UI.statusBanner.textContent = isMyTurn
		? "Sua vez de jogar!"
		: `Aguardando oponente (${State.gameData.turn})...`;
	UI.statusBanner.className = `status-banner ${isMyTurn ? "vez-ativa" : "vez-espera"}`;
};

const atualizarCronometro = () => {
	if (!State.gameData) return;

	if (State.gameData.winner) {
		UI.tempoBanner.style.display = "none";
		return;
	}

	const agoraSecs = Math.floor(Date.now() / 1000);
	const tempoDecorrido = agoraSecs - State.gameData.createdAt;
	const tempoRestante = 300 - tempoDecorrido;

	if (tempoRestante <= 0) {
		UI.tempoLabel.textContent = "00:00";
		return;
	}

	const min = String(Math.floor(tempoRestante / 60)).padStart(2, "0");
	const sec = String(tempoRestante % 60).padStart(2, "0");
	UI.tempoLabel.textContent = `${min}:${sec}`;

	if (tempoRestante <= 60) {
		UI.tempoBanner.classList.add("urgente");
	}
};

const fetchServerState = async () => {
	if (State.isUpdating || State.gameData?.winner) return;

	try {
		const res = await fetch(`/api/rooms/${Config.roomId}`);

		if (res.status === 404) {
			if (!State.gameData?.winner) {
				alert("⏰ Tempo Esgotado! A sala foi encerrada pelo servidor.");
				window.location.href = "/";
			}
			return;
		}

		const data = await res.json();
		if (State.isUpdating) return;
		State.gameData = data;

		updateInterface();
		atualizarCronometro();
	} catch (e) {
		console.error("Erro ao buscar estado:", e);
	}
};

window.jogar = async (index) => {
	if (
		!State.gameData ||
		State.gameData.winner ||
		State.gameData.turn !== Config.playerSymbol ||
		State.gameData.board[index]
	)
		return;

	State.isUpdating = true;

	try {
		const res = await fetch(`/api/rooms/${Config.roomId}/move`, {
			method: "POST",
			headers: { "Content-Type": "application/json" },
			body: JSON.stringify({
				player: Config.playerName,
				symbol: Config.playerSymbol,
				cell: index,
			}),
		});

		if (!res.ok) {
			const data = await res.json();
			console.warn("Jogada rejeitada:", data.error);
		} else {
			const data = await res.json();
			State.gameData = data;
			updateInterface();
			atualizarCronometro();
		}
	} catch (e) {
		console.error("Erro ao enviar jogada:", e);
	} finally {
		State.isUpdating = false;
	}
};

window.voltarParaLobby = () => {
	window.location.href = "/";
};

setInterval(atualizarCronometro, 1000);
setInterval(fetchServerState, 512);
fetchServerState();
