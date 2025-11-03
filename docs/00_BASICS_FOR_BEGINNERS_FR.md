# Concepts de Base : Comprendre le Serveur IRC depuis Zéro

> Ce document est conçu pour les personnes qui ne sont pas familières avec la programmation réseau, expliqué en termes simples.

---

## 🏠 Analogie : Le Serveur comme un Bureau de Poste

Imaginez que notre serveur IRC est un **bureau de poste** :

```
Bureau de Poste (Serveur)
    ├── Agent au guichet (Listener) : accueille les nouveaux clients
    ├── Plusieurs boîtes aux lettres (Client) : une par client
    ├── Directeur du bureau (Server) : coordonne tout le travail
    └── Facteur (poll) : vérifie quelle boîte a du nouveau courrier
```

---

## 📞 Qu'est-ce qu'un Socket ?

### Analogie Simple
**Socket = Ligne Téléphonique**

Imaginez que chaque client et le serveur ont une « ligne téléphonique » entre eux, c'est un socket.

### Diagramme d'Architecture Complet

```
                    Serveur (Server)
        ┌───────────────────────────────────┐
        │                                   │
        │  ┌─────────────────────────────┐  │
        │  │   Listener (Socket d'Écoute)│  │
        │  │   Écoute sur port 6667      │  │
        │  │   En attente de connexions...│  │
        │  └─────────────────────────────┘  │
        │            ↓   ↓   ↓               │
        │    ┌───────┘   │   └───────┐       │
        │    ↓           ↓           ↓       │
        │ [Socket A] [Socket B] [Socket C]   │
        │ (Client A) (Client B) (Client C)   │
        └────│──────────│──────────│─────────┘
             │          │          │
             ↓          ↓          ↓
        Client A    Client B    Client C
```

**Points Clés à Comprendre :**
- **Listener** est un socket spécial à l'intérieur du serveur
- Listener **n'est pas** un client, c'est la « porte d'entrée » du serveur
- Les clients se connectent au port du Listener (ex: 6667)
- Après acceptation, Listener **crée un nouveau socket** pour communiquer avec le client

### Processus de Connexion

```
Étape 1: Démarrage du Serveur
┌─────────────┐
│   Server    │
│  ┌────────┐ │
│  │Listener│ │ ← Écoute sur 0.0.0.0:6667
│  │  6667  │ │    (En attente...)
│  └────────┘ │
└─────────────┘

Étape 2: Client A se Connecte
Client A ─────connect to 6667────→ Listener
                                    ↓
                                 accept()
                                    ↓
                          Crée nouveau socket (fd=4)
┌─────────────┐
│   Server    │
│  ┌────────┐ │
│  │Listener│ │ ← Continue d'écouter
│  └────────┘ │
│  [Socket 4]←┼──→ Client A (connexion dédiée)
└─────────────┘

Étape 3: Client B se Connecte Aussi
Client B ─────connect to 6667────→ Listener
                                    ↓
                                 accept()
                                    ↓
                          Crée nouveau socket (fd=5)
┌─────────────┐
│   Server    │
│  ┌────────┐ │
│  │Listener│ │ ← Continue d'écouter d'autres connexions
│  └────────┘ │
│  [Socket 4]←┼──→ Client A
│  [Socket 5]←┼──→ Client B
└─────────────┘
```

### Dans Notre Code

```cpp
// 1. Créer le socket d'écoute (Listener)
int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
bind(listen_fd, ...);   // Lier au port 6667
listen(listen_fd, ...); // Commencer l'écoute

// 2. Accepter une nouvelle connexion (géré par Listener)
int client_fd = accept(listen_fd, ...);  
// client_fd est un nouveau socket créé pour communiquer avec ce client

// 3. Toute communication ultérieure passe par client_fd
recv(client_fd, ...);  // Lire depuis le client
send(client_fd, ...);  // Envoyer au client
```

### Deux Types de Sockets

1. **Socket d'Écoute (Standard Central)**
   - **Un seul** (partagé par tout le serveur)
   - **Composant interne du serveur**
   - Spécialisé pour répondre aux « appels » des nouveaux clients
   - Ne transmet pas de données réelles
   - Correspond à notre `Listener`
   - Dans le code : `_listener.getFd()`

2. **Socket de Connexion (Ligne Individuelle)**
   - **Un par client**
   - Créé par `accept()` du Listener
   - Utilisé pour communiquer avec un client spécifique
   - Transmet les commandes et réponses réelles
   - Correspond à notre `Client`
   - Dans le code : le `_fd` de chaque objet `Client`

---

## 🔧 Qu'est-ce que fcntl ?

### Analogie Simple
**fcntl = Configurer le mode de fonctionnement de la ligne téléphonique**

Imaginez que le téléphone a deux modes de réponse :

#### Mode 1 : Mode Bloquant (par défaut)
```
📞 Driiing~~~ Driiing~~~
😴 Je reste assis ici à attendre que le téléphone sonne...
   J'attends...
   J'attends...
   J'attends...
   Enfin, un appel ! Je décroche !
```
**Problème :** En attendant le téléphone, je ne peux rien faire d'autre !

#### Mode 2 : Mode Non-Bloquant (ce que nous voulons)
```
📞 Y a-t-il un appel ?
   → Non : Ok, je fais autre chose
   → Oui : Je décroche et je traite
     
📞 Y a-t-il un appel ? (vérification continue)
   → Non : Ok, je fais autre chose
   → Oui : Je décroche et je traite
```
**Avantage :** Je peux gérer plusieurs choses en même temps !

### Dans Notre Code

```cpp
// Configurer le socket en mode non-bloquant
fcntl(fd, F_SETFL, O_NONBLOCK);

// C'est comme dire : « Cette ligne téléphonique, ne me fais pas attendre bêtement ! »
```

### Pourquoi C'est Important ?

Sans le mode non-bloquant :
```
Client A envoie un long message...
Le serveur attend que A finisse 😴
→ Les clients B, C, D sont tous ignorés ! ❌
```

Avec le mode non-bloquant :
```
Client A envoie une partie
Serveur : Reçu, je le stocke
Serveur : Je vérifie si B a des messages
Serveur : Je vérifie si C a des messages
Serveur : Je reviens continuer avec A
✅ Traitement équitable de tous les clients !
```

---

## 📊 Qu'est-ce que poll ?

### Analogie Simple
**poll = La tournée du facteur**

Imaginez qu'un facteur (poll) est responsable de patrouiller toutes les boîtes aux lettres :

```
Le facteur est en patrouille...

Boîte 1 (fd=4) : [Nouveau courrier !] ← Notification !
Boîte 2 (fd=5) : [Vide]
Boîte 3 (fd=6) : [Nouveau courrier !] ← Notification !
Boîte 4 (fd=7) : [Vide]

Le facteur rapporte : « Les boîtes 1 et 3 ont du nouveau courrier ! »
```

### Pourquoi Avons-nous Besoin de poll ?

**Sans poll :**
```cpp
// Méthode stupide : vérifier chaque client tour à tour
for (chaque client) {
    recv(fd, ...);  // Y a-t-il un message ?
                    // Sinon, j'attends... 😴
}
// ❌ Problème : Si le premier client n'a rien, on est bloqué !
```

**Avec poll :**
```cpp
// Méthode intelligente : vérifier tous en même temps
poll(...);  // Dis-moi quels clients ont des messages
// poll retourne : « fd=4 et fd=6 ont des messages ! »

// Traiter seulement ceux qui ont des messages
Traiter fd=4
Traiter fd=6
// ✅ Pas de temps perdu !
```

### Comment poll Fonctionne

```cpp
struct pollfd {
    int fd;        // File descriptor (numéro de téléphone)
    short events;  // Quels événements je veux surveiller ?
                   // POLLIN  = quelqu'un appelle
                   // POLLOUT = je peux appeler
    short revents; // Événements réels survenus (rempli par poll)
};

// Créer la liste de surveillance
pollfd fds[10];
fds[0].fd = 4;  fds[0].events = POLLIN;
fds[1].fd = 5;  fds[1].events = POLLIN;
// ...

// Commencer la surveillance !
int ready = poll(fds, 10, -1);  // -1 = attente infinie
// poll « s'endort » jusqu'à ce qu'un événement se produise

// poll se réveille ! Il y a ready événements
for (chaque fd) {
    if (fds[i].revents & POLLIN) {
        // Ce fd a de nouvelles données !
    }
}
```

---

## 🔄 Comment Fonctionne Notre Fonction run()

### Diagramme de Flux Complet

```
                    Démarrer le Serveur
                        ↓
            ┌───────────────────────┐
            │   for (;;) Boucle     │
            │   Infinie             │
            └───────────────────────┘
                        ↓
            ┌───────────────────────┐
            │  poll() Attendre      │ ← Dormir jusqu'à ce que quelque chose se passe
            │  Tous les fd clients  │
            └───────────────────────┘
                        ↓
                  Événement survenu !
                        ↓
            ┌───────────────────────┐
            │  Vérifier chaque fd   │
            └───────────────────────┘
                        ↓
        ┌───────────────┼───────────────┐
        │               │               │
    [Nouvelle        [Lecture]      [Écriture]
    Connexion]
        ↓               ↓               ↓
    accept()        recv()          send()
    Créer nouveau   Lire commande   Envoyer réponse
    client
        │               │               │
        └───────────────┴───────────────┘
                        ↓
                  Traité
                        ↓
                  Retour à poll()
```

### Explication Détaillée des Étapes

#### Étape 1 : Initialisation

```cpp
Server::Server(int port, const string& password) {
    // 1. Créer le socket d'écoute (standard)
    _listener = Listener(port);
    
    // 2. Ajouter le standard à la liste de surveillance poll
    pollfd p;
    p.fd = _listener.getFd();      // Numéro du standard
    p.events = POLLIN;              // Surveiller « nouveau client »
    _pfds.push_back(p);             // Ajouter à la liste
    
    // 3. Configurer en non-bloquant
    fcntl(p.fd, F_SETFL, O_NONBLOCK);
}
```

**Analogie :**
- Le bureau de poste ouvre
- Le guichet est prêt
- Commencer les opérations !

#### Étape 2 : Début de la Boucle Principale

```cpp
void Server::run() {
    cout << "Le serveur commence à fonctionner..." << endl;
    
    for (;;) {  // Boucle infinie (ouvert en permanence)
        // Étape suivante...
```

#### Étape 3 : Attendre des Événements (poll)

```cpp
        // Tous les fd sont dans le tableau _pfds
        // Peut contenir :
        // _pfds[0] = listener (standard)
        // _pfds[1] = client A
        // _pfds[2] = client B
        // _pfds[3] = client C
        
        int ready = poll(&_pfds[0], _pfds.size(), -1);
        //              ↑           ↑              ↑
        //              tableau     combien de fd  attente infinie
        
        // poll() « dort » ici
        // Quand un fd a un événement, poll() se réveille
        // ready = combien de fd ont des événements
```

**Analogie :**
```
Le facteur s'assoit pour se reposer 😴
Soudain !
📬 Boîte 1 a du nouveau courrier !
📬 Boîte 3 aussi !
Le facteur se réveille : « Il y a 2 boîtes à traiter ! »
```

#### Étape 4 : Traiter les Événements

```cpp
        // Parcourir tous les fd
        for (size_t i = 0; i < _pfds.size(); ++i) {
            
            if (_pfds[i].revents == 0) {
                continue;  // Ce fd n'a pas d'événement, ignorer
            }
            
            // Ce fd a un événement !
            int fd = _pfds[i].fd;
            
            // Déterminer quel type d'événement...
```

##### Type d'Événement A : Nouveau Client (POLLIN sur listener)

```cpp
            if ((_pfds[i].revents & POLLIN) && 
                fd == _listener.getFd()) {
                
                // Le standard sonne ! Nouveau client !
                int new_fd = accept(...);
                
                // Ajouter le nouveau client à la liste de surveillance
                pollfd np;
                np.fd = new_fd;
                np.events = POLLIN;  // Surveiller les messages du client
                _pfds.push_back(np);
                
                // Configurer en non-bloquant
                fcntl(new_fd, F_SETFL, O_NONBLOCK);
            }
```

**Analogie :**
```
🔔 La sonnette retentit !
Nouveau client entre
Guichet : « Bonjour ! Voici votre numéro de boîte aux lettres »
Ajouter la boîte du client à la liste de patrouille
```

##### Type d'Événement B : Client Parle (POLLIN sur client)

```cpp
            else if (_pfds[i].revents & POLLIN) {
                // Un client a un message à lire
                
                char buf[4096];
                int n = recv(fd, buf, sizeof(buf), 0);
                
                if (n > 0) {
                    // Données reçues !
                    traiter_ce_message();
                }
                else if (n == 0) {
                    // Le client raccroche (EOF)
                    fermer_cette_connexion();
                }
                else if (errno == EAGAIN) {
                    // Pas de données pour le moment, ce n'est pas grave
                    // On vérifiera à nouveau au prochain poll
                }
            }
```

**Analogie :**
```
📬 Boîte 2 a du nouveau courrier !
Ouvrir la boîte
Lire le contenu de la lettre
« PRIVMSG #test :Bonjour »
Traiter cette commande
```

##### Type d'Événement C : Peut Répondre (POLLOUT)

```cpp
            else if (_pfds[i].revents & POLLOUT) {
                // On peut envoyer des données maintenant
                
                int n = send(fd, outbuf, size, 0);
                
                if (n > 0) {
                    // Envoi réussi
                    supprimer_partie_envoyee();
                }
                else if (errno == EAGAIN) {
                    // Ne peut pas envoyer pour le moment
                    // Réessayer au prochain poll
                }
                
                if (envoi_termine) {
                    // Désactiver la surveillance POLLOUT
                    _pfds[i].events &= ~POLLOUT;
                }
            }
```

**Analogie :**
```
✉️ On peut poster une lettre au client B maintenant !
Mettre la réponse dans la boîte
Envoi réussi ✅
Si la boîte est pleine (EAGAIN), réessayer plus tard
```

#### Étape 5 : Nettoyage et Répétition

```cpp
        }  // Fin de la boucle for
        
        // Tous les événements traités
        // Retour à poll() pour continuer l'attente
        
    }  // Boucle infinie
}
```

---

## 🎬 Exemple d'Exécution Réelle

Regardons un scénario complet :

### Scénario : Deux Clients Discutent

```
Chronologie poll()           Événement              Action du Serveur
─────────────────────────────────────────────────────────────────────
T0      Dort 😴         
                        Client A se connecte       
        Se réveille !   listener POLLIN            accept()
                                                   Créer fd=4
                                                   Ajouter à _pfds
        
T1      Dort 😴
                        Client B se connecte
        Se réveille !   listener POLLIN            accept()
                                                   Créer fd=5
                                                   Ajouter à _pfds
        
T2      Dort 😴
                        A envoie : NICK alice
        Se réveille !   fd=4 POLLIN                recv()
                                                   Traiter commande NICK
                                                   sendLine() vers fd=4
                                                   Activer fd=4 POLLOUT
        
T3      Dort 😴
        Se réveille !   fd=4 POLLOUT               send() message de bienvenue
                                                   Envoi terminé
                                                   Désactiver POLLOUT
        
T4      Dort 😴
                        A: PRIVMSG B :Salut
        Se réveille !   fd=4 POLLIN                recv()
                                                   Trouver B (fd=5)
                                                   sendLine() vers fd=5
                                                   Activer fd=5 POLLOUT
        
T5      Dort 😴
        Se réveille !   fd=5 POLLOUT               send() à B
                                                   B reçoit le message de A !
```

---

## 🤔 Questions Fréquentes

### Q1 : Pourquoi utiliser poll plutôt que le multi-threading ?

**R :** 
```
Solution Multi-Threading :
- Un thread par client
- 100 clients = 100 threads
- Grande consommation de mémoire
- Changements de contexte lents
- Mécanismes de verrouillage complexes

Solution poll :
- Thread unique
- 100 clients = 100 fd dans un tableau
- Faible consommation de mémoire
- Pas de changement de contexte
- Pas besoin de verrous
```

### Q2 : Qu'est-ce que « non-bloquant » ?

**R :** Utilisons l'analogie d'un restaurant :

```
Mode Bloquant (Restaurant Traditionnel) :
Vous : « Je voudrais un steak »
La cuisine commence à préparer...
Vous restez au comptoir à attendre 😴
30 minutes plus tard...
Vous : Enfin prêt !
→ Pendant ce temps, impossible de servir d'autres clients

Mode Non-Bloquant (Fast-Food) :
Vous : « Je voudrais un steak »
Cuisine : « D'accord, revenez dans 30 minutes »
Vous : Servir le client suivant
Vous : Servir le client suivant
...
30 minutes plus tard
Vous : Vérifier, le steak est prêt !
Vous : Le donner au client
→ Peut servir plusieurs clients simultanément
```

### Q3 : Qu'est-ce que EAGAIN ?

**R :** EAGAIN = "Pas maintenant, réessayez plus tard"

```
Scénario 1 : Lors de la lecture
recv() retourne EAGAIN
→ Signification : « Pas de données pour le moment, mais la connexion n'est pas coupée »
→ Ce n'est pas une erreur !
→ Vérifier à nouveau au prochain poll()

Scénario 2 : Lors de l'écriture
send() retourne EAGAIN
→ Signification : « Le tampon de réception de l'autre est plein, envoyer plus tard »
→ Ce n'est pas une erreur !
→ Réessayer au prochain poll()
```

### Q4 : Pourquoi un seul poll() ?

**R :**
```
Problème avec plusieurs poll() :
poll1() surveille fd 1-50 😴
poll2() surveille fd 51-100 😴

→ Si les fd de poll1 n'ont pas d'événements
   poll1 attendra indéfiniment
   Les fd de poll2 seront ignorés !

Avantage d'un seul poll() :
poll() surveille tous les fd 😴
→ N'importe lequel a un événement, il se réveille
→ Traitement équitable de tous les clients
```

---

## 📝 Résumé : L'Essence de run()

```
run() est un « employé de traitement d'événements » :

1. S'asseoir au bureau (entrer dans la boucle)
2. Attendre que le téléphone sonne (poll dort)
3. Le téléphone sonne ! (poll se réveille)
4. Voir qui appelle (vérifier revents)
5. Traiter cet appel (recv/send)
6. Attendre le prochain appel (retour à poll)

Caractéristiques clés :
- Ne se repose jamais (boucle infinie)
- Traite tous les appels équitablement (poll unique)
- N'attend pas bêtement (non-bloquant)
- Traitement efficace (piloté par événements)
```

---

## 🎯 Résumé en Une Phrase pour Vos Collègues

> **Notre serveur utilise un seul poll() pour surveiller tous les clients, comme un facteur qui patrouille toutes les boîtes aux lettres. Quand une boîte (client) a du nouveau courrier (message), poll nous le notifie pour que nous le traitions. Tous les sockets sont configurés en non-bloquant, donc nous ne restons jamais bloqués en attendant un client, ce qui nous permet de servir tout le monde équitablement.**

---

## 📚 Lecture Complémentaire

Après avoir lu ce document, vous pouvez continuer avec :
- `02_CORE_FLOW.md` - Détails de flux plus approfondis
- `04_EVALUATION_POINTS.md` - Comment présenter aux évaluateurs

---

**Maintenant vous devriez comprendre comment fonctionne run() !** 🎉

