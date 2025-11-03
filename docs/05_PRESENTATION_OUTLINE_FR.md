# Plan de Présentation (Version 1 heure)

## 🎯 Structure de la Présentation

### 📌 Partie 1 : Vue d'ensemble du Projet (10 minutes)

#### 1. Objectif du Projet
```
Nous avons implémenté un serveur de chat conforme au protocole IRC
- Support des connexions multi-clients
- Utilisation d'I/O non-bloquants
- Implémentation des commandes IRC de base
- Conforme aux standards d'évaluation ft_irc
```

#### 2. Choix Techniques
```
- Langage : C++98
- Modèle I/O : poll() + socket non-bloquant
- Pattern de conception : Command Pattern
- Architecture : Pilotée par événements
```

#### 3. Structure des Fichiers
```
src/
  ├── main.cpp           # Point d'entrée
  ├── Server.cpp         # Logique principale
  ├── Listener.cpp       # Socket d'écoute
  ├── Client.cpp         # Gestion des clients
  ├── Channel.cpp        # Gestion des canaux
  └── cmds/              # Implémentation de chaque commande
      ├── cmd_join.cpp
      ├── cmd_privmsg.cpp
      └── ...
```

---

### 📌 Partie 2 : Architecture Principale (15 minutes)

#### 1. Diagramme de Flux Global

Montrer le diagramme d'architecture de `docs/01_ARCHITECTURE_FR.md`

**Points clés :**
- Server est au centre
- Listener gère les nouvelles connexions
- Client gère chaque connexion
- poll() pilote tous les événements

#### 2. Explication Détaillée de la Boucle Principale

```cpp
// Pseudo-code
while (true) {
    // 1. Attendre événements
    poll(&fds);
    
    // 2. Traiter événements
    for each fd with events:
        if (nouvelle connexion) accept()
        if (lisible) recv() + traiter commande
        if (inscriptible) send()
        if (erreur) fermer
    
    // 3. Nettoyage et ajouts
    Fermer ceux à fermer
    Ajouter nouvelles connexions
}
```

**À souligner :**
- ✅ Un seul poll()
- ✅ Tous les I/O après poll()
- ✅ Traitement non-bloquant

#### 3. Flux de Données

```
Commande client
    ↓
recv() → _inbuf → popLine() → Parser
    ↓
handleIrcMessage() → CommandRegistry
    ↓
Command::execute() spécifique
    ↓
sendLine() → _outbuf → send() → Client
```

---

### 📌 Partie 3 : Système de Commandes (10 minutes)

#### 1. Pattern de Conception

**Avantages du Command Pattern :**
- Extensibilité
- Interface unifiée
- Séparation des responsabilités

#### 2. Enregistrement des Commandes

Montrer `CommandInit.cpp`:
```cpp
registry.registerCmd("PASS", new CmdPass());
registry.registerCmd("NICK", new CmdNick());
// ...
```

#### 3. Exemple d'Exécution de Commande

Montrer le flux de `CmdPrivmsg::execute()`

---

### 📌 Partie 4 : Points Techniques Clés (15 minutes)

#### 1. I/O Non-Bloquant

**Pourquoi non-bloquant ?**
```
Mode bloquant : read() attend, bloque tout le serveur
Mode non-bloquant : read() retourne immédiatement, utiliser poll() pour attendre
```

**Notre implémentation :**
```cpp
// Tous les sockets configurés en O_NONBLOCK
fcntl(fd, F_SETFL, O_NONBLOCK);

// recv/send rencontrent EAGAIN, retour direct
if (errno == EAGAIN) return;  // Attendre prochain poll
```

#### 2. Gestion des Tampons

**Tampon d'entrée (_inbuf) :**
- Accumule les données reçues
- Découpe par lignes
- Traite les commandes partielles

**Tampon de sortie (_outbuf) :**
- Met en file les données à envoyer
- Envoi par lots
- Traite le blocage d'envoi

#### 3. Processus d'Enregistrement

```
PASS → NICK → USER → Envoie 001 bienvenue
```

**Application de l'ordre :**
- Avant PASS, NICK/USER sont refusés
- Non enregistré ne peut pas utiliser d'autres commandes

#### 4. Diffusion de Canal

```cpp
broadcastToChannel("#test", message, except_fd) {
    for (chaque membre) {
        if (fd != except_fd) {
            member->sendLine(message);
            enableWriteForFd(fd);
        }
    }
}
```

---

### 📌 Partie 5 : Traitement des Scénarios Spéciaux (10 minutes)

#### 1. Commande Partielle (Test Ctrl+D)

**Scénario :**
```
PRI[Ctrl+D]VMG[Ctrl+D] #test :bonjour[Enter]
```

**Traitement :**
- Accumulation dans `_inbuf`
- `popLine()` attend ligne complète
- Analyse correcte de la commande complète

#### 2. Déconnexion Client

**Traitement EOF :**
```cpp
if (recv() == 0) {
    // Traiter tour actuel puis fermer
    toCloseIdx.push_back(idx);
}
```

**Dégradation gracieuse :**
- Terminer traitement des événements actuels
- Envoyer messages en attente
- Puis fermer

#### 3. Suspension Ctrl+Z (Point important !)

**Scénario :**
1. Client A appuie sur Ctrl+Z (suspension)
2. Autres clients envoient beaucoup de messages à A
3. A reprend (fg)

**Notre traitement :**
```cpp
send() retourne EAGAIN
    ↓
Conserve dans _outbuf
    ↓
Maintient surveillance POLLOUT
    ↓
Prochain poll() continue d'essayer
    ↓
Après reprise du client, reçoit tous les messages
```

**Démonstration :** Montrer ce scénario en direct

---

### 📌 Partie 6 : Points d'Évaluation (5 minutes)

#### Liste de Vérification Rapide

```
✅ Un seul poll()
✅ poll() avant tous les I/O
✅ fcntl(fd, F_SETFL, O_NONBLOCK)
✅ errno ne déclenche pas de nouvelle tentative
✅ Multi-connexions sans blocage
✅ Commande partielle correctement traitée
✅ Ctrl+Z ne bloque pas
```

#### Questions Possibles et Réponses

**Q : Pourquoi Ctrl+D sur ligne vide ferme-t-il la connexion ?**
```
R : EOF TCP indique que le client a fermé le canal d'écriture.
   Pour IRC, impossible d'envoyer des commandes n'a pas de sens.
   C'est le traitement correct du protocole.
```

**Q : Comment assurer un seul poll() ?**
```
R : grep -rn "poll(" src/
   Une seule occurrence dans Server::run()
```

**Q : Comment gérer haute concurrence ?**
```
R : Tous les fd dans le même tableau poll
   I/O non-bloquant assure pas de blocage mutuel
   Piloté par événements, traiter qui a événement
```

---

### 📌 Partie 7 : Démonstration (5 minutes)

#### Terminaux Préparés

**Terminal 1 : Serveur**
```bash
./ircserv 6667 password123
```

**Terminal 2 : Client A**
```bash
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
JOIN #test
```

**Terminal 3 : Client B**
```bash
nc -C localhost 6667
PASS password123
NICK bob
USER bob 0 * :Bob
JOIN #test
PRIVMSG #test :Bonjour tout le monde !
```

**Points à montrer :**
1. Les deux clients reçoivent des messages
2. Fonction de message privé
3. Test Ctrl+Z (si le temps le permet)

---

## 🎬 Astuces de Présentation

### Répartition du Temps
```
00:00 - 10:00  Vue d'ensemble + Architecture
10:00 - 20:00  Explication flux principal
20:00 - 30:00  Système de commandes
30:00 - 45:00  Points techniques clés
45:00 - 55:00  Scénarios spéciaux + Démo
55:00 - 60:00  Q&R
```

### Points à Souligner

#### À Mentionner Absolument :
1. ✅ **Un seul poll()**
2. ✅ **I/O Non-Bloquant**
3. ✅ **Architecture Pilotée par Événements**
4. ✅ **Command Pattern**
5. ✅ **Gestion des Tampons**

#### Où Montrer Votre Confiance :
1. Traitement Ctrl+Z (beaucoup échouent)
2. Système de commandes élégant
3. Architecture claire en couches
4. Conformité à toutes les exigences d'évaluation

### Préparation aux Questions Attendues

**Questions Techniques :**
- "Pourquoi poll et pas epoll ?" → Exigence du sujet + portabilité
- "Comment gérer beaucoup de connexions ?" → Non-bloquant + poll unique
- "Comment la gestion mémoire ?" → Chaque Client est un pointeur, delete à la sortie

**Questions d'Implémentation :**
- "Comment tester ?" → nc + client IRC + comparaison avec vrai serveur
- "Quelles difficultés rencontrées ?" → Compréhension I/O non-bloquant + traitement commandes partielles
- "Comment déboguer ?" → Ajouter sortie DEBUG + gdb

---

## 📝 Carte Aide-Mémoire Rapide

### Concepts de Base
```
1. Piloté par événements : poll() pilote tout
2. Non-bloquant : EAGAIN = attendre prochain, pas erreur
3. Tampons : accumuler entrée, mettre en file sortie
4. Fermeture différée : traiter tour actuel puis fermer
```

### Emplacements Clés du Code
```
- Appel poll() : Server.cpp:122
- Traitement événements : Server.cpp:309
- Distribution commandes : Server.cpp:109
- Lecture non-bloquante : Client.cpp:31
- Écriture non-bloquante : Client.cpp:78
```

### Points d'Évaluation
```
✅ 1 poll
✅ poll avant I/O
✅ fcntl correct
✅ errno ne réessaye pas
✅ Multi-connexions OK
✅ Ctrl+Z OK
```

---

## 💪 Confiance en Soi

Votre implémentation :
- ✅ Architecture claire
- ✅ Conforme à toutes les exigences
- ✅ Code de bonne qualité
- ✅ Informations de débogage complètes
- ✅ Tous les scénarios spéciaux traités

**Détendez-vous, vous êtes bien préparé !** 🎉

