# Mécanisme de Fermeture TCP Détaillé

## 🔌 Les Deux Canaux d'une Connexion TCP

Une connexion TCP a en réalité **deux canaux indépendants** :

```
Client                                  Serveur
   │                                      │
   │ ────────→ Canal écriture (envoyer)→  │  Canal lecture (recevoir)
   │                                      │
   │ ←──────── Canal lecture (recevoir)←  │  Canal écriture (envoyer)
   │                                      │
```

### Deux Méthodes de Fermeture

#### 1. shutdown() - Fermeture Partielle (Half-Close)

```c
// Fermer seulement le canal d'écriture
shutdown(fd, SHUT_WR);
// Résultat :
//   ✅ Canal lecture toujours ouvert (peut recevoir)
//   ❌ Canal écriture fermé (ne peut pas envoyer)

// Fermer seulement le canal de lecture
shutdown(fd, SHUT_RD);
// Résultat :
//   ❌ Canal lecture fermé (ne peut pas recevoir)
//   ✅ Canal écriture toujours ouvert (peut envoyer)

// Fermer lecture et écriture
shutdown(fd, SHUT_RDWR);
// Résultat :
//   ❌ Lecture et écriture fermées
//   ⚠️ Mais fd existe toujours, nécessite close()
```

#### 2. close() - Fermeture Complète

```c
close(fd);
// Résultat :
//   ❌ Canal lecture fermé
//   ❌ Canal écriture fermé
//   ❌ fd libéré
```

---

## 🎬 Votre Question : Que se Passe-t-il lors d'un EOF

### Séquence Complète de Fermeture

```
Étape    Client (nc)                      Serveur
──────────────────────────────────────────────────────
1       Utilisateur appuie sur Ctrl+D ligne vide
        ↓
        shutdown(SHUT_WR)                  
        Ferme seulement canal écriture ───────→
        
        État :
        ✅ Peut recevoir (canal lecture ouvert)
        ❌ Ne peut pas envoyer (canal écriture fermé)
                                           recv() retourne 0
                                           Détecte EOF
                                           ↓
2                                          closeClient(idx)
                                           ↓
                                           close(fd)
                                           Ferme lecture/écriture ←───────
        
        Reçoit FIN ←────────────────────────
        
        État :
        ❌ Ne peut pas recevoir (pair fermé)
        ❌ Ne peut pas envoyer (déjà fermé)
        
3       Connection closed
        Programme nc se termine
```

### Pourquoi Utilisons-nous close() au lieu de shutdown() ?

Regardons notre code :

```cpp
// Client::closeNow()
void Client::closeNow() {
    if (_fd >= 0) {
        ::close(_fd);  // ← Utilise close(), fermeture complète
        _fd = -1;
    }
}
```

**Nous utilisons `close(fd)`, qui ferme simultanément lecture et écriture !**

---

## 🤔 Pourquoi Ne Pas Seulement Fermer le Côté Lecture ?

### Théoriquement Possible d'Implémenter Half-Close

```cpp
// Implémentation théorique half-close
if (recv() == 0) {  // Client a fermé côté écriture
    // Fermer seulement côté lecture du serveur
    shutdown(fd, SHUT_RD);
    
    // Garder côté écriture ouvert
    // Continuer à envoyer données en attente
    
    // Attendre que toutes données soient envoyées puis close(fd)
}
```

### Mais Nous Choisissons Fermeture Complète, Raisons :

#### 1. Caractéristiques du Protocole IRC

```
IRC est un protocole interactif bidirectionnel :

Client envoie commande → Serveur traite → Serveur répond
       ↓                                      ↓
    Besoin canal écriture              Besoin canal lecture

Si client ne peut pas envoyer commandes (côté écriture fermé)
→ Garder connexion n'a pas de sens pratique
```

#### 2. Observation du Comportement Réel

**Ce que vous avez observé :** "A après EOF peut encore recevoir messages"

Ce n'est pas parce que nous avons implémenté half-close, mais à cause de la **fermeture différée** :

```cpp
// Ordre de traitement dans Server::run()
for (;;) {
    poll(...);
    
    // Traiter tous événements
    for (chaque fd) {
        if (a événement) {
            processPollEvent(i, toAddFds, toCloseIdx);
            // ← A reçoit EOF, seulement marqué dans toCloseIdx
            // ← B envoie message à A, A->sendLine(...)
            // ← POLLOUT de A peut être déclenché, send() à A
        }
    }
    
    // Après traitement du tour actuel, fermeture réelle
    for (chaque idx à fermer) {
        closeClient(idx);  // ← C'est là que close(fd)
    }
}
```

**Fenêtre temporelle :**
```
T0 : Événement POLLIN de A → recv EOF → Marquer fermeture
T1 : Événement POLLIN de B → Envoyer à A → sendLine(A)
T2 : Événement POLLOUT de A → send() à A ✅ (A peut encore recevoir !)
T3 : Fin boucle → closeClient(A) → close(fd) ❌ (Fermeture réelle maintenant)
```

**Donc A peut brièvement recevoir messages, parce que :**
- Fermeture différée (fermer après traitement du tour)
- Pas parce que nous avons implémenté half-close
- `close(fd)` est toujours fermeture complète lecture/écriture

---

## 📊 Comparaison close() vs shutdown()

### Notre Implémentation Actuelle

```cpp
void Client::closeNow() {
    ::close(_fd);  // Fermeture complète
}
```

**Effet :**
```
Côté serveur :
  ❌ Canal lecture fermé (ne peut plus recv)
  ❌ Canal écriture fermé (ne peut plus send)
  ❌ fd libéré

Client reçoit FIN :
  ❌ Connexion complètement terminée
```

### Si Implémentation Half-Close (Théorique)

```cpp
void Client::closeNow() {
    // Option 1 : Fermer seulement côté lecture
    ::shutdown(_fd, SHUT_RD);  // Half-close
    
    // Option 2 : Fermer seulement côté écriture
    ::shutdown(_fd, SHUT_WR);  // Half-close
}
```

**Mais problèmes :**
- Besoin suivi d'état supplémentaire (quels fd sont half-closed)
- Besoin appeler `close()` au bon moment
- Augmente complexité
- Pas de valeur pratique pour IRC

---

## 🎯 Compréhension Correcte

### Lors d'un Ctrl+D sur Ligne Vide

```
Instant 0 : Client appuie sur Ctrl+D
───────────────────────────────────────────────
État client :
  nc exécute shutdown(SHUT_WR)
  ✅ Client peut recevoir (canal lecture ouvert)
  ❌ Client ne peut pas envoyer (canal écriture fermé)

État serveur : (pas encore traité)
  ✅ Peut recevoir (canal lecture ouvert)
  ✅ Peut envoyer (canal écriture ouvert)


Instant 1 : Serveur recv() retourne 0
───────────────────────────────────────────────
Serveur détecte EOF
Marque comme à fermer


Instant 2 : Traitement poll en cours (possible)
───────────────────────────────────────────────
Autres clients envoient messages à ce client
Serveur send() au client ✅
Client reçoit ✅ (car canal lecture du client toujours ouvert)


Instant 3 : Exécution de closeClient()
───────────────────────────────────────────────
Serveur :
  close(fd)  ← Fermeture complète lecture/écriture
  
Client :
  Reçoit FIN
  Canal lecture aussi fermé
  Connection closed
```

---

## 🔑 Résumé

| Question | Réponse |
|----------|---------|
| **Utilisons-nous close() ou shutdown() ?** | `close(fd)` - Fermeture complète |
| **close() ferme combien de côtés ?** | Les deux (lecture + écriture) |
| **Pourquoi A peut encore recevoir messages ?** | Fermeture différée (close après traitement tour) |
| **Est-ce une implémentation half-close ?** | ❌ Non, c'est fermeture complète |
| **Délai ?** | Un cycle poll (généralement < 1ms) |

---

**Point Clé : Nous utilisons `close()`, fermant effectivement lecture et écriture !** 🎯

