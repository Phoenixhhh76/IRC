# Points Clés d'Évaluation - Référence Rapide

## ✅ Basic Checks (Obligatoires, sinon note de 0)

### 1. Makefile et Compilation
```bash
# Points à vérifier
- [✅] Présence d'un Makefile
- [✅] Compilation sans erreur ni avertissement
- [✅] Utilisation de C++98
- [✅] Nom de l'exécutable : ircserv
- [✅] Options de compilation : -Wall -Wextra -Werror -std=c++98
```

**Démonstration :**
```bash
make clean
make
# Pas d'avertissement, pas d'erreur
```

### 2. Vérification du Nombre de poll()

**Exigence :** Il ne doit y avoir qu'**un seul** appel à poll()

**Notre implémentation :**
```cpp
// src/Server.cpp:122
void Server::run() {
    for (;;) {
        int ready = ::poll(&_pfds[0], _pfds.size(), -1);  // ← Seul poll()
        // ...
    }
}
```

**Preuve :**
```bash
grep -rn "poll(" src/ include/
# Ne trouve qu'une seule occurrence d'appel poll()
```

### 3. Moment d'Appel de poll()

**Exigence :** poll() doit être appelé avant chaque accept/read/recv/write/send

**Notre flux :**
```
poll() retourne
    ↓
Vérifie POLLIN → puis appelle accept()/recv()
Vérifie POLLOUT → puis appelle send()
```

**Emplacements clés dans le code :**
- `Server.cpp:122` - Appel de poll()
- `Server.cpp:313` - accept() après POLLIN
- `Server.cpp:344` - recv() après POLLIN  
- `Server.cpp:378` - send() après POLLOUT

### 4. Vérification de l'Utilisation de fcntl()

**Exigence :** Tous les fcntl() doivent être `fcntl(fd, F_SETFL, O_NONBLOCK)`

**Notre implémentation :**
```cpp
// src/Listener.cpp:71
fcntl(fd, F_SETFL, O_NONBLOCK)

// src/acceptClients.cpp:14
fcntl(fd, F_SETFL, O_NONBLOCK)

// src/bonus/Dcc.cpp:74 (corrigé)
fcntl(s, F_SETFL, O_NONBLOCK)
```

**Preuve :**
```bash
grep -rn "fcntl" src/
# Tous au format F_SETFL, O_NONBLOCK
```

### 5. Vérification de l'Utilisation de errno

**Exigence :** errno ne doit pas être utilisé pour déclencher une nouvelle tentative (comme réessayer de lire après `errno == EAGAIN`)

**Notre implémentation :**
```cpp
// Client::readFromSocket()
if (errno == EAGAIN || errno == EWOULDBLOCK) {
    return true;  // ← Retour direct, attendre prochain poll(), pas de nouvelle tentative !
}

// Client::flushOutbuf()
if (errno == EAGAIN || errno == EWOULDBLOCK) {
    return;  // ← Retour direct, attendre prochain poll(), pas de nouvelle tentative !
}
```

## 🌐 Networking (Fonctions Réseau)

### 1. Démarrage du Serveur
```bash
./ircserv 6667 password123
# Écoute sur toutes les interfaces (0.0.0.0:6667)
```

**Preuve :**
```bash
netstat -an | grep 6667
# Devrait afficher 0.0.0.0:6667 LISTEN
```

### 2. Test de Connexion nc
```bash
nc localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
# Devrait recevoir message de bienvenue
```

### 3. Test de Client IRC
```bash
# Utiliser irssi ou hexchat
/connect localhost 6667 password123
/nick testuser
```

### 4. Test Multi-Connexions

**Démonstration :** Ouvrir 3 terminaux simultanément
- Terminal A : connexion nc
- Terminal B : connexion client IRC
- Terminal C : une autre connexion nc

**Prouver le non-blocage :** Toutes les connexions peuvent fonctionner en même temps

### 5. Diffusion de Messages de Canal
```bash
# Client A
JOIN #test
PRIVMSG #test :Bonjour de A

# Client B (même canal)
# Devrait recevoir le message de A
```

## 🔧 Network Specials (Scénarios Spéciaux)

### 1. Test de Commande Partielle

```bash
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
# Tester commande segmentée (Ctrl+D en milieu de ligne)
PRI[Ctrl+D]VMG[Ctrl+D] #test :bonjour[Enter]
# ✅ Devrait analyser correctement la commande PRIVMSG complète
```

**Principe :** 
- Accumulation dans `_inbuf`
- `popLine()` ne retourne qu'après réception d'une ligne complète

### 2. Déconnexion Inattendue du Client

```bash
# Terminal A : connexion nc
# Terminal B : kill -9 <PID de A>
# Ou dans terminal A appuyer sur Ctrl+C

# Le serveur devrait :
- [✅] Fermer correctement ce client
- [✅] Les autres connexions continuent de fonctionner
- [✅] Ne pas planter
```

### 3. Déconnexion au Milieu d'une Commande

```bash
nc localhost 6667
PASS password123
NICK alice
JOIN #t[Ctrl+C]  ← Interruption au milieu d'une commande

# Le serveur devrait :
- [✅] Ne pas entrer dans un état étrange
- [✅] Ne pas se bloquer
- [✅] Nettoyer correctement les ressources
```

### 4. Test Ctrl+Z (Important !)

```bash
# Terminal A
nc localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
JOIN #test
[Ctrl+Z]  ← Suspension

# Terminal B
nc localhost 6667
PASS password123
NICK bob
USER bob 0 * :Bob
JOIN #test
# Envoyer beaucoup de messages à alice
PRIVMSG alice :msg1
PRIVMSG alice :msg2
...(nombreux messages)

# Terminal A
fg  ← Reprise

# Vérifications :
- [✅] alice reçoit tous les messages
- [✅] Le serveur ne s'est pas bloqué
- [✅] Pas de fuite mémoire
```

**Principe :**
- `send()` retourne `EAGAIN`, conserve dans `_outbuf`
- Maintient la surveillance POLLOUT
- Après reprise du client, continue l'envoi

## 📝 Client Commands (Commandes Client)

### Test des Commandes de Base

```bash
# 1. Processus d'enregistrement
PASS password123
NICK alice
USER alice 0 * :Alice Smith
# ✅ Reçoit message de bienvenue 001

# 2. Test PRIVMSG
PRIVMSG bob :Bonjour Bob        # Message privé
PRIVMSG #test :Bonjour canal   # Message de canal

# 3. JOIN/PART
JOIN #test
PART #test :Au revoir

# 4. TOPIC
TOPIC #test :Nouveau sujet

# 5. MODE
MODE #test +t
MODE #test +l 10
```

## 🎯 Ordre de Démonstration Suggéré

### Partie 1 : Vérifications de Base (5 minutes)
1. Montrer compilation Makefile
2. Prouver qu'il n'y a qu'un seul poll()
3. Montrer utilisation correcte de fcntl()

### Partie 2 : Fonctions Réseau (10 minutes)
1. Démarrer le serveur
2. Test de connexion nc
3. Fonctionnement simultané de plusieurs connexions
4. Diffusion de messages de canal

### Partie 3 : Scénarios Spéciaux (10 minutes)
1. Commande partielle (Ctrl+D en milieu de ligne)
2. Déconnexion de client
3. **Test de suspension Ctrl+Z (Point important !)**

### Partie 4 : Explication du Code (15 minutes)
1. Diagramme d'architecture
2. Flux de la boucle principale
3. Système de commandes
4. Traitement I/O non-bloquant

## 📊 Liste de Vérification d'Évaluation

```
Vérifications de Base (toutes obligatoires)
□ Makefile correct
□ Un seul poll()
□ poll() appelé avant I/O
□ Format fcntl() correct
□ errno ne déclenche pas de nouvelle tentative

Fonctions Réseau
□ Le serveur démarre et écoute
□ nc peut se connecter
□ Le client IRC peut se connecter
□ Plusieurs connexions fonctionnent ensemble
□ Messages de canal correctement diffusés

Scénarios Spéciaux
□ Commande partielle correctement traitée
□ Déconnexion client correctement nettoyée
□ Ctrl+Z ne bloque pas le serveur
□ Messages normaux après reprise

Fonctions de Commande
□ Enregistrement PASS/NICK/USER
□ PRIVMSG privé et canal
□ JOIN/PART corrects
□ Autres commandes implémentées
```

## 🔑 Phrases Clés

**Quand l'évaluateur demande "Pourquoi...":**

- **Q : Pourquoi Ctrl+D sur ligne vide ferme-t-il ?**
  - R : "C'est un EOF TCP, indiquant que le client a fermé le canal d'écriture. Pour IRC, garder une connexion après l'impossibilité d'envoyer des commandes n'a pas de sens."

- **Q : Pourquoi un seul poll() ?**
  - R : "C'est une exigence. Tous les fd (listener + tous les clients) sont dans le même tableau poll pour assurer un ordonnancement équitable."

- **Q : Comment gérer I/O non-bloquant ?**
  - R : "EAGAIN n'est pas une erreur, mais 'temporairement pas de données/impossible d'écrire', on retourne directement, en attendant la prochaine notification de poll()."

