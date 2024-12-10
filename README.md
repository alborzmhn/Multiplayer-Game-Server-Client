
# Multiplayer Game Server-Client Project

This repository contains a C++ implementation of a server-client architecture for a multiplayer game based on Rock-Paper-Scissors. It includes the server-side logic, client-side interaction, and a project report.

---

## 📁 Project Contents

- **server.cpp** - Contains the game server implementation, handling client connections, game rooms, and match results.
- **client.cpp** - The client-side implementation allowing players to connect to the server and play the game.
- **report.pdf** - A detailed project report discussing the implementation, system design, and evaluation results.

---

## 📦 Project Features

### Server Features:
- **Game Management:**
  - Room-based game sessions.
  - Real-time game state management.
  - Support for multiple concurrent games.
  
- **Networking:**
  - TCP-based client-server communication.
  - Broadcast notifications for game events.

- **Game Logic:**
  - Rock-Paper-Scissors game logic.
  - Automatic winner determination.
  - Broadcast game results after each match.

### Client Features:
- **Player Interaction:**
  - Connects to the server and joins game rooms.
  - Receives game updates and match results.
  - Selects game actions interactively.

- **Network Communication:**
  - Sends player choices and receives server responses in real time.

---

## 🚀 Getting Started

1. **Clone the repository:**
   ```bash
   git clone https://github.com/YourUsername/YourRepo.git
   ```

2. **Navigate to the project directory:**
   ```bash
   cd YourRepo
   ```

3. **Compile the Server and Client:**
   ```bash
   g++ server.cpp -o server
   g++ client.cpp -o client
   ```

4. **Run the Server:**
   ```bash
   ./server <server_ip> <server_port> <number_of_rooms>
   ```

5. **Run the Client (separate terminal):**
   ```bash
   ./client <server_ip> <server_port>
   ```

---

## 🧪 How It Works

1. **Server Initialization:**
   - Creates multiple game rooms.
   - Waits for client connections.

2. **Client Connection:**
   - Connects to the server and waits for a game room assignment.

3. **Game Play:**
   - Clients receive game prompts and send their choices.
   - Server evaluates the game result and broadcasts the outcome.

4. **Game End:**
   - Server broadcasts match results when the game ends.

---

## ⚙️ Requirements

- **Compiler:** g++
- **System:** Linux-based system recommended
- **Libraries:** Standard C++ libraries

