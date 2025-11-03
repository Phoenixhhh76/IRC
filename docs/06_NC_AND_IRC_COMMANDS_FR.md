# Paramètres nc et Commandes IRC Détaillés

## 🔧 Explication des Paramètres netcat (nc)

### Paramètre `-C` : Fin de Ligne CRLF

**Fonction :** Convertir les fins de ligne en CRLF (`\r\n`)

```bash
# Sans -C
nc localhost 6667
NICK alice[Enter]
# Envoie : "NICK alice\n" (seulement LF)

# Avec -C
nc -C localhost 6667
NICK alice[Enter]
# Envoie : "NICK alice\r\n" (CRLF)
```

#### Pourquoi Important ?

**Standard du Protocole IRC :**
- RFC 1459 spécifie : Les commandes IRC doivent se terminer par `\r\n` (CRLF)
- `\r` = Carriage Return (retour chariot, ASCII 13)
- `\n` = Line Feed (saut de ligne, ASCII 10)

**Différences Système :**
```
Unix/Linux/macOS:  Fin de ligne \n (LF)
Windows:           Fin de ligne \r\n (CRLF)
Protocole IRC:     Exige \r\n (CRLF)
```

**Traitement de Notre Serveur :**
```cpp
// Client::popLine() - traitement tolérant
std::string::size_type pos = _inbuf.find("\r\n");  // Cherche d'abord CRLF
if (pos != std::string::npos) {
    // CRLF trouvé, IRC standard
}
// Tolérant : accepte aussi seulement '\n'
std::string::size_type lf = _inbuf.find('\n');
if (lf != std::string::npos) {
    // Fonctionne aussi, pour compatibilité avec outils de test
}
```

#### Quand Utiliser -C

| Scénario | Utiliser -C | Raison |
|----------|------------|--------|
| **Tester son serveur** | Recommandé | Conforme au standard IRC |
| **Connexion serveur IRC réel** | **Obligatoire** | Sinon commandes non reconnues |
| **Scripts automatisés** | Recommandé | Assurer compatibilité |
| **Test rapide** | Optionnel | Notre serveur tolère |

---

### Paramètre `-q` : Délai après EOF

**Fonction :** Contrôler le temps d'attente après stdin EOF

```bash
# nc -q 0
nc -q 0 localhost 6667
# Après EOF, attend 0 seconde, ferme immédiatement

# nc -q 5
nc -q 5 localhost 6667
# Après EOF, attend 5 secondes avant fermeture

# nc (sans -q)
nc localhost 6667
# Comportement par défaut (varie selon version)
```

#### Comportement Détaillé

```
Scénario : Entrée par pipe
─────────────────────────────────────────────────────

Sans -q (ou valeur par défaut) :
echo "NICK alice" | nc localhost 6667
     ↓
nc envoie "NICK alice\n"
     ↓
stdin EOF (pipe se termine)
     ↓
[Attente...] (par défaut peut attendre 10s ou indéfiniment)
     ↓
(Peut ne pas recevoir réponse serveur avant fermeture)

Avec -q 0 :
echo "NICK alice" | nc -q 0 localhost 6667
     ↓
nc envoie "NICK alice\n"
     ↓
stdin EOF
     ↓
Attend 0 seconde ← Fermeture immédiate
     ↓
Connexion fermée

Avec -q 3 :
echo "NICK alice" | nc -q 3 localhost 6667
     ↓
nc envoie "NICK alice\n"
     ↓
stdin EOF
     ↓
Attend 3 secondes ← Temps pour recevoir réponse serveur
     ↓
Reçoit : ":server 001 alice :Welcome..."
     ↓
Ferme après 3 secondes
```

#### Quand Utiliser -q

| Scénario | Paramètre Suggéré | Raison |
|----------|------------------|--------|
| **Test interactif** | Pas besoin de -q | Entrée manuelle, pas EOF immédiat |
| **Entrée par pipe** | `-q 2` ou `-q 3` | Donner temps de recevoir réponse |
| **Test EOF** | `-q 0` | Déclencher immédiatement comportement EOF |
| **Scripts automatisés** | `-q 1` | Assurer réception réponse |

---

### Combinaison de `-C` et `-q`

```bash
# Meilleure pratique : test interactif
nc -C localhost 6667
# -C : Utiliser CRLF (conforme standard IRC)
# Pas -q : opération manuelle pas besoin

# Scripts automatisés
echo -e "PASS password123\nNICK alice\nUSER alice 0 * :Alice" | nc -C -q 2 localhost 6667
# -C : Fin de ligne CRLF
# -q 2 : Après EOF attend 2s pour recevoir réponse
```

---

## 📝 Explication Détaillée de la Commande IRC USER

### Format de Commande

```
USER <username> <mode> <unused> :<realname>
```

### Explication des Paramètres

#### Exemple Complet
```bash
USER alice 0 * :Alice Smith
     │     │ │  │
     │     │ │  └─ realname (nom réel)
     │     │ └──── unused (non utilisé, généralement *)
     │     └────── mode (mode utilisateur, généralement 0)
     └──────────── username (nom d'utilisateur)
```

### Détail de Chaque Paramètre

#### 1. `<username>` - Nom d'Utilisateur

```
Fonction : Identifiant du compte utilisateur
Format : Lettres, chiffres, certains symboles
Exemples : alice, bob123, test_user

Note :
- Différent de NICK (pseudonyme)
- username affiché généralement dans préfixe complet
```

**Exemple :**
```
NICK : alice
USER : alice_user

Préfixe complet : alice!alice_user@hostname
                  ↑     ↑
                  pseudo nom d'utilisateur
```

#### 2. `<mode>` - Mode Utilisateur (généralement 0)

```
Fonction : Définir drapeaux de mode initial de l'utilisateur
Format : Nombre (masque de bits)

Valeurs courantes :
- 0  : Aucun mode spécial (le plus courant)
- 8  : invisible (mode invisible)
- 4  : wallops
```

**Utilisation Réelle :**
```bash
USER alice 0 * :Alice    # ← Le plus courant, aucun mode spécial
USER bob 8 * :Bob        # ← Définir mode invisible
```

**Notre Implémentation :**
```cpp
// cmd_user.cpp
const std::string username = m.params[0];
const std::string realname = m.params[3];
// Paramètre mode (m.params[1]) généralement ignoré
// Car les clients IRC modernes ne l'utilisent pas beaucoup
```

#### 3. `<unused>` - Paramètre Non Utilisé (généralement `*`)

```
Fonction : Paramètre historique legacy, non utilisé maintenant
Format : Généralement remplir * ou nom d'hôte

Historique :
- IRC ancien utilisait pour transmettre nom d'hôte
- Serveurs IRC modernes détectent automatiquement nom d'hôte
- Donc ce paramètre devient "non utilisé"
```

**Exemples :**
```bash
USER alice 0 * :Alice        # ← Usage standard
USER bob 0 localhost :Bob    # ← Aussi possible, mais sera ignoré
USER charlie 0 unused :C     # ← N'importe quelle valeur
```

**Notre Traitement :**
```cpp
// cmd_user.cpp
const std::string hostname = m.params[2];  
if (hostname != "*")  // Utilise seulement si pas *
    cl.setHost(hostname);
// Sinon utilise IP détectée automatiquement par serveur
```

#### 4. `:<realname>` - Nom Réel

```
Fonction : Nom complet ou description de l'utilisateur
Format : Commence par deux-points, peut contenir espaces
Exemples : :Alice Smith, :Bob from Paris, :Utilisateur Test

Note :
- Doit commencer par : (indique dernier paramètre, peut avoir espaces)
- Peut être n'importe quel texte
- Affiché dans réponses aux commandes WHOIS etc
```

**Exemples :**
```bash
USER alice 0 * :Alice Smith          # Nom complet
USER bob 0 * :IRC Test User          # Description
USER charlie 0 * :Charlie from NYC   # Avec lieu
USER dave 0 * :测试用户              # Chinois aussi possible
```

### Exemples Complets de Commande USER

#### Usage Le Plus Courant

```bash
nc -C localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice Smith
```

**Analyse :**
```
USER alice 0 * :Alice Smith
     ↑     ↑ ↑  ↑
     │     │ │  └─ realname = "Alice Smith"
     │     │ └──── unused = "*" (pratique standard)
     │     └────── mode = 0 (aucun mode spécial)
     └──────────── username = "alice"

Résultat :
nick     = alice
username = alice
realname = Alice Smith
host     = 127.0.0.1 (détection auto serveur)

Préfixe complet : alice!alice@127.0.0.1
```

---

## 🎯 Suggestions de Combinaisons de Paramètres

### Meilleure Pratique

#### Test Interactif (Recommandé)
```bash
nc -C localhost 6667
# Seulement -C suffit
# Entrée manuelle pas besoin de -q
```

#### Scripts Automatisés
```bash
(
  echo "PASS password123"
  echo "NICK alice"
  echo "USER alice 0 * :Alice"
  sleep 2
  echo "JOIN #test"
  echo "PRIVMSG #test :Bonjour"
  sleep 1
  echo "QUIT :Au revoir"
) | nc -C -q 3 localhost 6667
# -C : Fin de ligne CRLF
# -q 3 : Après EOF attend 3s pour recevoir réponse
```

#### Test Comportement EOF
```bash
nc -C -q 0 localhost 6667
PASS password123
NICK alice
USER alice 0 * :Alice
[Ctrl+D]
# -q 0 : Fermeture immédiate, pour tester traitement EOF serveur
```

---

## 📊 Processus Complet d'Enregistrement

### Enregistrement Standard

```bash
nc -C localhost 6667

# Étape 1 : Vérification mot de passe
PASS password123
# Serveur interne : cl.setPassOk()

# Étape 2 : Définir pseudonyme
NICK alice
# Serveur interne : cl.setNick("alice")

# Étape 3 : Définir infos utilisateur
USER alice 0 * :Alice Smith
# Serveur interne :
# cl.setUser("alice", "Alice Smith")
# cl.tryFinishRegister() → Vérifier hasPass && hasNick && hasUser
# Envoyer message de bienvenue !

# Reçoit réponse
:ft_irc 001 alice :Welcome to the ft_irc network, alice
```

---

## ✅ Liste de Vérification Rapide

**À utiliser pendant les tests :**
```
□ nc -C localhost 6667           (test interactif)
□ nc -C -q 2 ... (test entrée pipe)
□ nc -C -q 0 ... (test EOF)
```

**Format commande USER :**
```
□ USER <username> 0 * :<realname>
□ Deux-points devant realname
□ 4 paramètres requis
□ Après PASS et NICK
```

---

**Maintenant vous comprenez complètement les paramètres nc et la commande USER !** 🎉

