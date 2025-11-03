# Système de Commandes Détaillé

## 🎯 Pattern de Conception : Command Pattern

Chaque commande IRC est une classe indépendante, interface unifiée, enregistrement centralisé.

## 📋 Architecture du Système

```
IrcMessage (résultat de l'analyse)
    ↓
Server::handleIrcMessage() (vérifier état d'enregistrement)
    ↓
CommandRegistry::dispatch() (router vers la bonne commande)
    ↓
execute() de la classe Command spécifique (exécuter la logique)
    ↓
Client::sendLine() (répondre au client)
```

## 🔧 Composants Principaux

### 1. Classe de Base Command (Command.hpp)

```cpp
class Command {
public:
    virtual ~Command() {}
    virtual void execute(Server& srv, Client& cl, const IrcMessage& m) = 0;
};
```

Toutes les commandes héritent de cette interface.

### 2. CommandRegistry (Registre des Commandes)

```cpp
class CommandRegistry {
private:
    map<string, Command*> _cmds;  // nom de commande -> objet commande

public:
    void registerCmd(const string& name, Command* cmd) {
        _cmds[name] = cmd;
    }
    
    bool dispatch(const string& cmd, Server& srv, Client& cl, const IrcMessage& m) {
        map<string, Command*>::iterator it = _cmds.find(cmd);
        if (it == _cmds.end()) return false;  // Commande inconnue
        
        it->second->execute(srv, cl, m);  // ← Appeler la commande spécifique
        return true;
    }
};
```

### 3. CommandInit (Enregistrement des Commandes)

```cpp
void registerAllCommands(CommandRegistry& registry, Server& srv) {
    registry.registerCmd("PASS", new CmdPass());
    registry.registerCmd("NICK", new CmdNick());
    registry.registerCmd("USER", new CmdUser());
    registry.registerCmd("JOIN", new CmdJoin());
    registry.registerCmd("PRIVMSG", new CmdPrivmsg());
    registry.registerCmd("QUIT", new CmdQuit());
    // ... autres commandes
}
```

## 📝 Exemples d'Implémentation de Commandes

### Commande NICK

```cpp
class CmdNick : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m) {
        // 1. Vérification des paramètres
        if (m.params.empty()) {
            cl.sendLine(ERR_NONICKNAMEGIVEN(srv.serverName()));
            srv.enableWriteForFd(cl.fd());
            return;
        }
        
        const string& nick = m.params[0];
        
        // 2. Vérification de validité
        if (!isValidNick(nick)) {
            cl.sendLine(ERR_ERRONEUSNICKNAME(srv.serverName(), nick));
            srv.enableWriteForFd(cl.fd());
            return;
        }
        
        // 3. Vérification de doublon
        if (srv.isNickInUse(nick)) {
            cl.sendLine(ERR_NICKNAMEINUSE(srv.serverName(), nick));
            srv.enableWriteForFd(cl.fd());
            return;
        }
        
        // 4. Mettre à jour le pseudonyme
        srv.takeNick(cl, nick);
        
        // 5. Diffuser à tous les canaux concernés
        const set<string>& channels = cl.channels();
        for (set<string>::const_iterator it = channels.begin(); it != channels.end(); ++it) {
            srv.broadcastToChannel(*it, oldPrefix + " NICK :" + nick, -1);
        }
        
        // 6. Si c'est une partie du processus d'enregistrement, envoyer message de bienvenue
        if (cl.tryFinishRegister()) {
            srv.sendWelcome(cl);
        }
    }
};
```

### Commande PRIVMSG

```cpp
class CmdPrivmsg : public Command {
public:
    void execute(Server& srv, Client& cl, const IrcMessage& m) {
        // 1. Vérification des paramètres
        if (m.params.size() < 2) {
            cl.sendLine(ERR_NEEDMOREPARAMS(srv.serverName(), "PRIVMSG"));
            srv.enableWriteForFd(cl.fd());
            return;
        }
        
        const string& target = m.params[0];
        const string& text = m.params[1];
        string line = cl.getFullPrefix() + " PRIVMSG " + target + " :" + text;
        
        // 2. Déterminer le type de cible
        if (target[0] == '#') {
            // Envoyer au canal
            if (!srv.isChannelMember(target, cl.fd())) {
                cl.sendLine(ERR_CANNOTSENDTOCHAN(srv.serverName(), target));
                srv.enableWriteForFd(cl.fd());
                return;
            }
            srv.broadcastToChannel(target, line, cl.fd());
        } else {
            // Envoyer à l'utilisateur
            srv.sendToNick(target, line);
        }
    }
};
```

## 🔐 Vérification des Permissions (Server::handleIrcMessage)

```cpp
void Server::handleIrcMessage(Client& cl, const IrcMessage& m) {
    const string& cmd = m.command;
    
    // ========== Restrictions pour clients non enregistrés ==========
    if (!cl.registered()) {
        // Doit d'abord PASS
        if (!cl.hasPass() && (cmd == "NICK" || cmd == "USER")) {
            cl.sendLine(ERR_PASSWDMISMATCH(_servername));
            enableWriteForFd(cl.fd());
            return;
        }
        
        // Autoriser seulement certaines commandes
        if (cmd != "PASS" && cmd != "NICK" && cmd != "USER" && 
            cmd != "PING" && cmd != "CAP" && cmd != "QUIT") {
            cl.sendLine(ERR_NOTREGISTERED(_servername));
            enableWriteForFd(cl.fd());
            return;
        }
    }
    
    // ========== Dispatcher les commandes ==========
    if (!_cmds.dispatch(cmd, *this, cl, m)) {
        // Commande inconnue (optionnel d'envoyer une erreur)
        cl.sendLine(ERR_UNKNOWNCOMMAND(_servername, cmd));
        enableWriteForFd(cl.fd());
    }
}
```

## 📊 Diagramme de Flux des Commandes

```
Client envoie : "PRIVMSG #test :Bonjour"
    ↓
Parser::parseLine()
    ↓
IrcMessage {
    command: "PRIVMSG"
    params: ["#test", "Bonjour"]
}
    ↓
Server::handleIrcMessage()
    ├─ Vérifier enregistrement ? ✓
    └─ _cmds.dispatch("PRIVMSG", ...)
           ↓
       CmdPrivmsg::execute()
           ├─ Vérifier paramètres ✓
           ├─ Déterminer cible est canal ✓
           ├─ Vérifier présence dans canal ✓
           └─ broadcastToChannel("#test", ...)
                  ↓
              Parcourir membres du canal
                  ├─ member1->sendLine(...)
                  ├─ member2->sendLine(...)
                  └─ member3->sendLine(...)
                         ↓
                  enableWriteForFd() pour chacun
                         ↓
                  Prochain poll() déclenche POLLOUT
                         ↓
                  send() envoie au client
```

## 🎯 Liste des Commandes Principales

| Commande | Fonction | Enregistrement Requis |
|----------|----------|---------------------|
| **PASS** | Vérification mot de passe | ❌ |
| **NICK** | Définir pseudonyme | ❌ |
| **USER** | Définir infos utilisateur | ❌ |
| **JOIN** | Rejoindre canal | ✅ |
| **PART** | Quitter canal | ✅ |
| **PRIVMSG** | Envoyer message | ✅ |
| **QUIT** | Déconnexion | ❌ |
| **PING** | Heartbeat | ❌ |
| **KICK** | Expulser utilisateur | ✅ |
| **MODE** | Définir modes | ✅ |
| **TOPIC** | Définir sujet | ✅ |

## 🔑 Avantages

1. **Extensibilité** : Ajouter une commande nécessite seulement créer nouvelle classe et l'enregistrer
2. **Interface unifiée** : Toutes les commandes utilisent la même interface execute()
3. **Gestion centralisée** : CommandRegistry route uniformément
4. **Séparation des responsabilités** : Chaque commande est responsable uniquement de sa propre logique
5. **Facilité de test** : Peut tester chaque classe de commande séparément

## 💡 Étapes pour Ajouter une Nouvelle Commande

1. Créer `cmd_xxx.hpp` et `cmd_xxx.cpp`
2. Hériter de la classe de base `Command`
3. Implémenter la méthode `execute()`
4. Enregistrer dans `CommandInit.cpp`
5. Ajouter le fichier source dans le `Makefile`

