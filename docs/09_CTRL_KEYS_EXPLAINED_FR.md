# Touches de Contrôle Expliquées : Ctrl+D vs Ctrl+C vs Ctrl+Z

## 🎯 Comparaison Rapide

| Touche | Essence | Action | Impact sur nc | Impact sur Serveur |
|--------|---------|--------|--------------|-------------------|
| **Ctrl+D** | Caractère EOF | Terminer saisie | Ferme côté écriture (envoie FIN) | recv() retourne 0 |
| **Ctrl+C** | Signal SIGINT | Interrompre programme | **Tue nc immédiatement** | recv() peut retourner 0 ou erreur |
| **Ctrl+Z** | Signal SIGTSTP | Suspendre programme | **Suspend nc** | Envoi peut retourner EAGAIN |

---

## 🔍 Ctrl+D - EOF (End of File)

### Essence
- **End of File** caractère
- Valeur ASCII : 4 (caractère de contrôle)
- **N'est pas** un signal, mais un caractère d'entrée

### Comportement

#### Lorsqu'il y a du Contenu sur la Ligne
```bash
bonjour[Ctrl+D]
```

**Ce qui se passe :**
```
Terminal : Envoie "bonjour" à nc
nc : Envoie "bonjour" au socket
Serveur : recv() reçoit "bonjour"
nc : Continue de fonctionner ✅
```

#### Lorsque la Ligne est Vide
```bash
[Ctrl+D ligne vide]
```

**Ce qui se passe :**
```
Terminal : Envoie EOF à nc
nc : "stdin terminé"
nc : shutdown(socket, SHUT_WR)
nc : Envoie TCP FIN au serveur
Serveur : recv() retourne 0 (EOF)
nc : macOS quitte / Ubuntu peut ne pas quitter
```

### Caractéristiques
- ✅ **Fermeture gracieuse** : Donne temps de traiter aux deux parties
- ✅ **Possibilité de half-close** : Ferme seulement côté écriture, lecture reste ouverte
- ✅ **Temps de nettoyage** : Serveur peut envoyer derniers messages
- ⚠️ **Peut ne pas quitter immédiatement** (dépend version nc)

---

## ⚡ Ctrl+C - SIGINT (Signal d'Interruption)

### Essence
- Signal **Signal Interrupt**
- Numéro de signal : 2
- Envoyé au groupe de processus en avant-plan

### Comportement

```bash
nc localhost 6667
PASS password123
NICK alice
[Ctrl+C]  ← Appuyer n'importe quand
```

**Ce qui se passe :**
```
1. Système d'exploitation envoie SIGINT à nc
   ↓
2. nc reçoit signal
   ↓
3. Traitement par défaut de nc : Terminer immédiatement programme
   ↓
4. Programme nc tué (similaire à kill)
   ↓
5. Système d'exploitation ferme tous les fd de nc
   ↓
6. Socket fermé (fermeture brutale)
   ↓
7. Côté serveur :
   - Peut recv() retourner 0 (pair fermé)
   - Peut recv() retourner erreur (connexion réinitialisée)
   - Peut voir événement POLLHUP
```

### Caractéristiques
- ⚡ **Terminaison immédiate** : Peu importe ce qui est en cours
- ❌ **Pas gracieuse** : Pas de temps de nettoyage
- ❌ **Fermeture forcée** : Peut perdre données non envoyées
- ✅ **Toujours efficace** : Peut quitter dans n'importe quel état

---

## ⏸️ Ctrl+Z - SIGTSTP (Signal de Suspension de Terminal)

### Essence
- Signal **Signal Terminal Stop**
- Numéro de signal : 18 (Unix) / 20 (Linux)
- Suspend le processus, ne le termine pas

### Comportement

```bash
nc localhost 6667
PASS password123
NICK alice
[Ctrl+Z]
```

**Ce qui se passe :**
```
1. Système d'exploitation envoie SIGTSTP à nc
   ↓
2. Processus nc suspendu (état : Stopped)
   ↓
3. nc n'exécute plus aucun code
   ↓
4. Socket reste ouvert ✅
   ↓
5. Côté serveur :
   - send() peut retourner EAGAIN (tampon plein)
   - Connexion existe toujours
   ↓
6. Utilisateur exécute fg (foreground)
   ↓
7. nc reprend fonctionnement
   ↓
8. Lit données accumulées depuis socket
```

### Caractéristiques
- ⏸️ **Récupérable** : fg pour reprendre, bg pour arrière-plan
- ✅ **Connexion maintenue** : socket ne se ferme pas
- ✅ **Pas de perte de données** : Accumulation dans tampon
- 🧪 **Pour test** : Tester traitement non-bloquant du serveur

---

## 📊 Tableau de Comparaison Détaillé

### Impact sur Saisie Terminal

| Touche | Traitement Saisie | Peut Continuer Saisie |
|--------|------------------|---------------------|
| **Ctrl+D** | Envoie tampon actuel | ✅ Oui (si contenu en ligne)<br>❌ Non (si ligne vide) |
| **Ctrl+C** | Rejette ligne actuelle | ❌ Non (programme terminé) |
| **Ctrl+Z** | Conserve ligne actuelle | ✅ Oui (après fg reprise) |

### Impact sur Programme

| Touche | État Programme | État socket | Récupérabilité |
|--------|---------------|-------------|---------------|
| **Ctrl+D** | Continue | Côté écriture fermé | ❌ Côté écriture irr

écupérable |
| **Ctrl+C** | **Terminé** | Fermé | ❌ Programme terminé |
| **Ctrl+Z** | **Suspendu** | Reste ouvert | ✅ fg/bg récupération |

### Impact sur Serveur

| Touche | Serveur recv() | Traitement Serveur | Nettoyage Connexion |
|--------|---------------|-------------------|-------------------|
| **Ctrl+D** | Retourne 0 (EOF) | Marquer fermeture, traitement différé | Nettoyage gracieux |
| **Ctrl+C** | Retourne 0 ou erreur | Nettoyage immédiat | Peut être brutal |
| **Ctrl+Z** | EAGAIN (temporairement pas données) | Continue fonctionnement, pas nettoyage | Pas de nettoyage |

---

## 🎬 Test de Comparaison Pratique

### Environnement de Test

```bash
# Terminal 1 : Serveur
./ircserv 6667 password123

# Terminal 2 : Client A
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
JOIN #test

# Terminal 3 : Client B
nc -C localhost 6667
PASS password123
NICK bob
USER bob 0 * :Bob
JOIN #test
```

### Test 1 : Ctrl+D (EOF)

```bash
# Terminal A
[Ctrl+D ligne vide]
```

**Sortie serveur :**
```
[DEBUG] Client fd=4 (alice) recv EOF, closing
[DEBUG] Marking client idx=1 fd=4 for close
[DEBUG] closeClient called for idx=1
[DEBUG] Closing client fd=4
```

**Sortie Terminal A :**
```
Connection closed by foreign host.  (macOS)
ou
(Toujours actif, attend)  (Ubuntu)
```

**Test Terminal B :**
```bash
PRIVMSG alice :test
# → :ft_irc 401 alice :No such nick  ✅ alice n'existe plus
```

### Test 2 : Ctrl+C (Interruption)

```bash
# Terminal A (nouvelle connexion)
nc -C localhost 6667
PASS password123
NICK charlie
USER charlie 0 * :Charlie
JOIN #test
[Ctrl+C]  ← Appuyer immédiatement
```

**Sortie serveur :**
```
[DEBUG] Client fd=5 (charlie) recv EOF, closing
ou
[DEBUG] Client fd=5 recv error: Connection reset by peer
[DEBUG] closeClient called for idx=2
```

**Sortie Terminal A :**
```
^C  ← Affiche Ctrl+C
(Programme nc terminé immédiatement) ✅ Identique toutes plateformes
```

**Différence :**
- ⚡ **Quitte immédiatement** (contrairement Ctrl+D sur Ubuntu peut ne pas quitter)
- ❌ **Peut être plus brutal** (connexion peut afficher reset plutôt que fermeture normale)

### Test 3 : Ctrl+Z (Suspension)

```bash
# Terminal A (nouvelle connexion)
nc -C localhost 6667
PASS password123
NICK david
USER david 0 * :David
JOIN #test
[Ctrl+Z]
```

**Sortie Terminal A :**
```
[1]+  Stopped                 nc -C localhost 6667
```

**Sortie serveur :**
```
(Aucun message erreur ou EOF)
(david toujours en ligne)
```

**Test Terminal B :**
```bash
PRIVMSG david :Es-tu là ?
# Serveur essaie d'envoyer à david
# send() peut retourner EAGAIN (tampon david plein)
# Message conservé dans _outbuf
```

**Reprise Terminal A :**
```bash
fg
# nc de david reprend
# Reçoit tous messages accumulés ! ✅
```

---

## 📚 Scénarios d'Utilisation

### Ctrl+D - Test Sortie Normale

```bash
# Usage :
- Tester traitement EOF
- Tester commandes partielles (Ctrl+D en ligne)
- Terminer normalement saisie

# Adapté pour :
- Tests interactifs
- Vérifier traitement EOF serveur
- Fin d'entrée par pipe
```

### Ctrl+C - Terminaison Forcée

```bash
# Usage :
- Quitter immédiatement nc
- Tester déconnexion inattendue client
- Terminer programme bloqué

# Adapté pour :
- Forcer fermeture quand nc ne quitte pas
- Tester traitement anormal serveur
- Arrêt d'urgence
```

### Ctrl+Z - Test Suspension

```bash
# Usage :
- Tester I/O non-bloquant
- Tester gestion tampons
- Tester que serveur ne se bloque pas

# Adapté pour :
- Test Ctrl+Z requis pour évaluation
- Vérifier accumulation messages
- Vérifier absence fuite mémoire
```

---

## 🎯 Scénarios de Test d'Évaluation

### Scénario 1 : Commande Partielle (Ctrl+D)

```bash
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
PRI[Ctrl+D]VMG[Ctrl+D] #test :msg[Enter]  ← Ctrl+D en ligne
# ✅ Teste traitement commande partielle
```

### Scénario 2 : Déconnexion EOF (Ctrl+D)

```bash
nc -C localhost 6667
PASS password123
NICK bob
USER bob 0 * :Bob
[Ctrl+D ligne vide]  ← Ctrl+D ligne vide
# ✅ Teste fermeture gracieuse
```

### Scénario 3 : Déconnexion Inattendue (Ctrl+C)

```bash
nc -C localhost 6667
PASS password123
NICK charlie
USER charlie 0 * :Charlie
[Ctrl+C]  ← Terminaison forcée
# ✅ Teste traitement déconnexion anormale
```

### Scénario 4 : Suspension Reprise (Ctrl+Z)

```bash
# Terminal A
nc -C localhost 6667
PASS password123
NICK david
USER david 0 * :David
JOIN #test
[Ctrl+Z]  ← Suspension

# Terminal B : Envoyer beaucoup messages
PRIVMSG david :msg1
PRIVMSG david :msg2
...

# Terminal A : Reprendre
fg
# Reçoit tous messages ✅
# ✅ Teste I/O non-bloquant et tampons
```

---

## 💡 Technique de Mémorisation

### Mémorisation Simple

```
Ctrl+D = Door (Porte)
  → Sortir gracieusement par la porte
  → Peut encore dire "au revoir"

Ctrl+C = Cancel (Annuler)
  → Annuler directement, partir
  → Ne même pas fermer la porte

Ctrl+Z = Zzz (Dormir)
  → S'endormir, mais encore dans la pièce
  → Peut être réveillé
```

### Mémorisation par Effet des Touches

```
D = Done (Terminé) → Saisie terminée
C = Cancel (Annuler) → Annuler programme
Z = Zzz (Sommeiller) → Suspension
```

---

## ⚠️ Malentendus Courants

### Malentendu 1 : "Ctrl+D tue le programme"

**Faux !**
- Ctrl+D est seulement un caractère EOF
- Programme peut continuer (surtout Ctrl+D en ligne)
- Ctrl+**C** tue le programme

### Malentendu 2 : "Ctrl+Z ferme la connexion"

**Faux !**
- Ctrl+Z suspend seulement le programme
- Connexion existe toujours
- fg peut reprendre

### Malentendu 3 : "Ctrl+C et Ctrl+D ont même effet"

**Complètement Différent !**
- Ctrl+D = Fin de fichier (fermeture possiblement gracieuse)
- Ctrl+C = Tuer programme (terminaison forcée)

---

## 📖 Carte de Référence Rapide

### Table de Référence Rapide des Touches de Contrôle

```
┌────────────────────────────────────────────────┐
│ Ctrl+D - EOF Fin de Fichier                    │
├────────────────────────────────────────────────┤
│ Action : Terminer saisie                       │
│ En ligne : Envoie contenu, ne ferme pas        │
│ Ligne vide : Envoie EOF, fermeture gracieuse   │
│ Usage : Tester commande partielle, traitement  │
│         EOF                                    │
└────────────────────────────────────────────────┘

┌────────────────────────────────────────────────┐
│ Ctrl+C - Signal SIGINT Interruption            │
├────────────────────────────────────────────────┤
│ Action : Terminer immédiatement programme      │
│ Effet : Fermeture forcée, quitte immédiatement │
│ Usage : Quitter de force, tester déconnexion   │
│         anormale                               │
└────────────────────────────────────────────────┘

┌────────────────────────────────────────────────┐
│ Ctrl+Z - Signal SIGTSTP Suspension             │
├────────────────────────────────────────────────┤
│ Action : Suspendre programme                   │
│ Effet : Connexion maintenue, reprise avec fg   │
│ Usage : Tester I/O non-bloquant, gestion       │
│         tampons                                │
└────────────────────────────────────────────────┘
```

---

## ✅ Résumé

| | Ctrl+D | Ctrl+C | Ctrl+Z |
|---|--------|--------|--------|
| **Qu'est-ce** | Caractère EOF | Signal interruption | Signal suspension |
| **Programme** | Peut continuer | Terminé | Suspendu |
| **Connexion** | Ferme côté écriture | Fermeture forcée | Maintenue |
| **Récupérable** | ❌ Côté écriture non | ❌ | ✅ fg |
| **Usage Évaluation** | Commande partielle + EOF | Déconnexion anormale | Test non-bloquant |

**Point Clé : Trois choses complètement différentes, testent différentes caractéristiques du serveur !** 🎯

