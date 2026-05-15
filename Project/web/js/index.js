const UI = {
	playerName: () => document.getElementById("playerName").value.trim().substring(0, 32),
	roomList: document.getElementById("roomList"),
};

const fetchRooms = async () => {
	try {
		const res = await fetch("/api/rooms");
		const rooms = await res.json();

		if (rooms.length === 0) {
			UI.roomList.innerHTML =
				'<div style="text-align: center; color: #94a3b8;">Nenhuma sala aberta. Crie uma!</div>';
			return;
		}

		UI.roomList.innerHTML = rooms
			.map((room) => {
				const playerO = room.player_o
					? `<span class="room-players">vs ${room.player_o}</span>`
					: '<span class="room-players waiting">Aguardando oponente...</span>';

				return `
                <div class="room-card">
                    <div class="room-info">
                        <strong>Sala: ${room.id}</strong>
                        <br><small>${room.player_x} ${playerO}</small>
                    </div>
                    <button class="btn-join" onclick="joinRoom('${room.id}')">Entrar 🎮</button>
                </div>
            `;
			})
			.join("");
	} catch (e) {
		console.error("Erro ao buscar salas:", e);
	}
};

window.criarSala = async () => {
	const player = UI.playerName();
	if (!player) return alert("Digite seu Nickname antes de criar uma sala!");

	try {
		const res = await fetch("/api/rooms", {
			method: "POST",
			headers: { "Content-Type": "application/json" },
			body: JSON.stringify({ player }),
		});

		if (!res.ok) {
			const data = await res.json();
			return alert(data.error || "Erro ao criar sala.");
		}

		const data = await res.json();

		sessionStorage.setItem("sala", data.room_id);
		sessionStorage.setItem("jogador", player);
		sessionStorage.setItem("simbolo", data.symbol);
		window.location.href = "sala";
	} catch (e) {
		alert("Erro de conexão ao criar sala.");
	}
};

window.joinRoom = async (roomId) => {
	const player = UI.playerName();
	if (!player) return alert("Digite seu Nickname antes de entrar!");

	try {
		const res = await fetch(`/api/rooms/${roomId}/join`, {
			method: "POST",
			headers: { "Content-Type": "application/json" },
			body: JSON.stringify({ player }),
		});

		if (!res.ok) {
			const data = await res.json();
			return alert(data.error || "Erro ao entrar na sala.");
		}

		const data = await res.json();

		sessionStorage.setItem("sala", roomId);
		sessionStorage.setItem("jogador", player);
		sessionStorage.setItem("simbolo", data.symbol);
		window.location.href = "sala";
	} catch (e) {
		alert("Erro de conexão ao entrar na sala.");
	}
};

document.addEventListener("DOMContentLoaded", fetchRooms);
setInterval(fetchRooms, 1024);
