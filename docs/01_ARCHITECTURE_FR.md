# Vue d'ensemble de l'Architecture du Serveur IRC

## 🏗️ Architecture Globale

```
┌─────────────────────────────────────────────────────────┐
│                      main.cpp                           │
│  - Analyser les arguments (port, password)              │
│  - Créer l'instance Server                              │
│  - Appeler server.run()                                 │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                    Classe Server                         │
│  Membres principaux :                                    │
│  - Listener _listener          (écouter nouvelles        │
│                                 connexions)              │
│  - vector<pollfd> _pfds        (poll surveille tous fd) │
│  - map<int, Client*> _clients  (gérer tous clients)     │
│  - CommandRegistry _cmds       (système d'enregistrement│
│                                 des commandes)           │
│  - map<string, Channel> _channels (tous les canaux)     │
└─────────────────────────────────────────────────────────┘
                            ↓
        ┌──────────────────┼──────────────────┐
        ↓                  ↓                  ↓
   ┌─────────┐      ┌──────────┐      ┌──────────┐
   │Listener │      │ Client   │      │ Channel  │
   │(socket) │      │(chaque   │      │(gestion  │
   │         │      │connexion)│      │de canal) │
   └─────────┘      └──────────┘      └──────────┘
```

## 📂 Structure des Fichiers Principaux

### 1. Couche Réseau
- `Listener.cpp/hpp` - Socket d'écoute, accepte nouvelles connexions
- `acceptClients.cpp/hpp` - Traite accept() et configure non-bloquant
- `Client.cpp/hpp` - Gestion des tampons de lecture/écriture par client

### 2. Boucle d'Événements
- `Server.cpp` fonction `run()` - Boucle poll principale
- `processPollEvent()` - Traite les événements de chaque fd

### 3. Système de Commandes
- `CommandRegistry.cpp/hpp` - Registre des commandes
- `CommandInit.cpp` - Enregistre toutes les commandes
- `cmds/cmd_*.cpp` - Implémentation de chaque commande

### 4. Couche Protocole
- `Parser.cpp/hpp` - Analyse les messages IRC
- `Replies.hpp` - Codes de réponse numériques IRC

### 5. Couche Logique
- `Channel.cpp/hpp` - Gestion des canaux, modes, membres
- `Server.cpp` - Coordination de la logique métier

## 🔑 Patterns de Conception Clés

### 1. I/O Non-Bloquant
- Tous les sockets configurés en `O_NONBLOCK`
- Utilise `poll()` pour surveiller les événements
- Quand `recv()`/`send()` retourne `EAGAIN`, pas de nouvelle tentative, attendre le prochain poll

### 2. Command Pattern
- Chaque commande est une classe indépendante
- Interface unifiée : `execute(Server&, Client&, IrcMessage&)`
- Enregistré dans `CommandRegistry`

### 3. Piloté par Événements
- `poll()` pilote tout le système
- Classification des événements : POLLIN (lisible), POLLOUT (inscriptible), POLLERR (erreur)

## 📊 Flux de Données

```
Client envoie une commande
    ↓
poll() retourne POLLIN
    ↓
recv() lit dans _inbuf
    ↓
popLine() extrait une ligne complète
    ↓
Parser::parseLine() analyse
    ↓
Server::handleIrcMessage() distribue
    ↓
CommandRegistry::dispatch() route
    ↓
execute() de la classe Cmd spécifique
    ↓
Client::sendLine() met dans _outbuf
    ↓
enableWriteForFd() active POLLOUT
    ↓
poll() retourne POLLOUT
    ↓
send() envoie au client
```

## 🎯 Points Clés d'Évaluation

✅ **Un seul poll()**
- Emplacement : boucle principale de `Server::run()`

✅ **poll() avant tous les I/O**
- accept/recv/send sont tous appelés après le retour de poll()

✅ **Format fcntl() correct**
- Tous utilisent `fcntl(fd, F_SETFL, O_NONBLOCK)`

✅ **Traitement non-bloquant**
- EAGAIN ne déclenche pas de nouvelle tentative, attendre le prochain poll

✅ **Gestion multi-connexions**
- Tous les fd dans le même tableau poll

