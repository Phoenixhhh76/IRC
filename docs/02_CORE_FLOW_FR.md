# Flux Principal Détaillé

## 🚀 Processus de Démarrage du Serveur

### main.cpp
```cpp
int main(int argc, char** argv) {
    // 1. Analyser les arguments
    int port = atoi(argv[1]);
    string password = argv[2];
    
    // 2. Créer le serveur
    Server server(port, password);
    
    // 3. Exécuter la boucle principale
    server.run();  // ← Ne retourne jamais, jusqu'à Ctrl+C
}
```

### Constructeur Server
```cpp
Server::Server(int port, const string& password)
    : _listener(port),  // ← Créer le socket d'écoute
      _password(password) {
    
    // 1. Ajouter le fd listener au tableau poll
    pollfd p;
    p.fd = _listener.getFd();
    p.events = POLLIN;  // ← Surveiller seulement la lecture (nouvelles connexions)
    _pfds.push_back(p);
    
    // 2. Enregistrer toutes les commandes
    initCommands();
}
```

## 🔄 Boucle d'Événements Principale (Server::run)

```cpp
void Server::run() {
    for (;;) {  // ← Boucle infinie
        // ========== 1. Attendre les événements ==========
        int ready = poll(&_pfds[0], _pfds.size(), -1);
        // -1 = attente infinie, jusqu'à un événement
        
        if (ready < 0) {
            if (errno == EINTR) continue;  // Interrompu par signal, réessayer
            throw runtime_error("poll failed");
        }
        
        // ========== 2. Traiter tous les événements ==========
        vector<int> toAddFds;      // Nouvelles connexions
        vector<size_t> toCloseIdx; // Indices à fermer
        
        for (size_t i = 0; i < _pfds.size(); ++i) {
            if (_pfds[i].revents) {  // ← Événement survenu
                processPollEvent(i, toAddFds, toCloseIdx);
            }
        }
        
        // ========== 3. Nettoyage et Ajouts ==========
        // D'abord fermer (ordre inverse, éviter décalage d'indices)
        for (size_t k = 0; k < toCloseIdx.size(); ++k) {
            size_t idx = toCloseIdx[toCloseIdx.size() - 1 - k];
            closeClient(idx);
        }
        
        // Ensuite ajouter
        for (size_t j = 0; j < toAddFds.size(); ++j) {
            addNewClient(toAddFds[j]);
        }
    }
}
```

## 📥 Traitement des Événements (processPollEvent)

```cpp
void Server::processPollEvent(size_t idx, ...) {
    pollfd cur = _pfds[idx];
    
    // ========== A. Nouvelle Connexion (Listener) ==========
    if ((cur.revents & POLLIN) && cur.fd == _listener.getFd()) {
        vector<pair<int, string>> news = acceptClients(_listener.getFd());
        // acceptClients() boucle accept() jusqu'à EAGAIN
        
        for (chaque nouveau fd) {
            // Créer objet Client
            Client* newClient = new Client(fd);
            _clients[fd] = newClient;
            
            // Ajouter au tableau poll
            pollfd np;
            np.fd = fd;
            np.events = POLLIN;  // ← Surveiller l'entrée du client
            _pfds.push_back(np);
        }
    }
    
    // ========== B. Client Lisible (POLLIN) ==========
    else if (cur.revents & POLLIN) {
        Client* cl = _clients[fd];
        
        // 1. Lire les données
        if (!cl->readFromSocket()) {  // ← recv()
            toCloseIdx.push_back(idx);  // EOF ou erreur
            return;
        }
        
        // 2. Traiter les commandes ligne par ligne
        string line;
        while (cl->popLine(line)) {  // ← Extraire ligne complète du tampon
            IrcMessage m = parseLine(line);
            handleIrcMessage(*cl, m);  // ← Traiter la commande
        }
    }
    
    // ========== C. Client Inscriptible (POLLOUT) ==========
    else if (cur.revents & POLLOUT) {
        Client* cl = _clients[fd];
        
        cl->flushOutbuf();  // ← send()
        
        // Si terminé, désactiver POLLOUT (éviter busy loop)
        if (cl->isOutbufEmpty()) {
            _pfds[idx].events &= ~POLLOUT;
        }
    }
    
    // ========== D. Erreur/Fermeture ==========
    if (cur.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        toCloseIdx.push_back(idx);
    }
}
```

## 📤 Lecture/Écriture Non-Bloquante

### Lecture (Client::readFromSocket)
```cpp
bool Client::readFromSocket() {
    char buf[4096];
    for (;;) {  // ← Boucle de lecture, jusqu'à épuisement des données
        ssize_t n = recv(_fd, buf, sizeof(buf), 0);
        
        if (n > 0) {
            _inbuf.append(buf, n);  // ← Accumuler dans le tampon
            continue;  // ← Continuer à lire
        }
        
        if (n == 0) return false;  // ← EOF, besoin de fermer
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return true;  // ← Lecture de ce tour terminée, attendre le prochain poll
        }
        
        return false;  // ← Erreur
    }
}
```

### Écriture (Client::flushOutbuf)
```cpp
void Client::flushOutbuf() {
    while (!_outbuf.empty()) {  // ← Boucle d'envoi
        ssize_t n = send(_fd, _outbuf.data(), _outbuf.size(), 0);
        
        if (n > 0) {
            _outbuf.erase(0, n);  // ← Supprimer la partie envoyée
            continue;  // ← Continuer à envoyer
        }
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;  // ← Impossible d'envoyer pour le moment, réessayer au prochain poll
        }
        
        throw runtime_error("send error");
    }
}
```

## 🔐 Processus d'Enregistrement

```
Connexion client
    ↓
PASS password123  ← Doit être le premier
    ↓
NICK alice
    ↓
USER alice 0 * :Alice Smith
    ↓
Server vérifie : hasPass && hasNick && hasUser
    ↓
Envoie RPL_WELCOME (001)
    ↓
registered = true
    ↓
Peut utiliser d'autres commandes
```

## ⏱️ Diagramme de Séquence : Envoi de Message

```
Temps  Client A         poll()         Server          Client B
──────────────────────────────────────────────────────────
T0   PRIVMSG B :salut
     ─────────────→  POLLIN         recv()
                     Événement A     Analyser commande
                                    Trouver B
                                    B->sendLine(...)
                                    B._outbuf = "..."
                                    enableWriteForFd(B)
                                    _pfds[B].events |= POLLOUT
                                                        
T1                   POLLOUT        send()         ←────
                     Événement B    B._outbuf      Reçoit message
                                    Envoi réussi
                                    _pfds[B].events &= ~POLLOUT
```

## 🎯 Points Clés

1. **poll() unique** : Toute l'attente dans un seul appel poll
2. **Piloté par événements** : I/O seulement après retour de poll
3. **Non-bloquant** : EAGAIN n'est pas une erreur, mais "attendre le prochain poll"
4. **Tampons** : _inbuf accumule, _outbuf met en file
5. **Fermeture différée** : Fermer après traitement du tour actuel, éviter perte de messages

