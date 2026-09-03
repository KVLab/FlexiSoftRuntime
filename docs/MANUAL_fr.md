# FlexiSoft Runtime — manuel d’intégration et de service

Ce manuel complète le README et décrit la configuration pratique de FlexiSoft Runtime. Il est destiné principalement à l’intégration dans une machine existante, à la modification de la configuration, à la préparation des textes de messages et au diagnostic de base.

FlexiSoft Runtime est une application d’aide à l’exploitation et au service. Elle ne fait pas partie de la fonction de sécurité de la machine. L’évaluation sûre et le reset de la logique de sécurité doivent toujours être assurés par le projet Flexi Soft et par le câblage de la machine.

---

## 1. Principe

Le runtime lit périodiquement des bits d’état dans Flexi Soft via RK512. À partir de ces bits, il évalue les canaux surveillés et affiche des messages à l’opérateur selon leur état.

Si l’opérateur confirme un reset après le contrôle, le runtime écrit le bit de commande RS-232 configuré. Le projet Flexi Soft crée une courte impulsion de reset à partir de ce bit.

Flux de base :

```text
FlexiSoftRuntime.exe
    -> RK512
    -> RS-232 ou convertisseur TCP/RS-232
    -> CPU Flexi Soft
    -> logique du projet
    -> relais de reset / sortie
```

---

## 2. Fichiers importants

| Fichier / dossier | Signification |
| --- | --- |
| `conf/config.json` | Configuration principale du runtime : communication, canaux, sorties, textes de messages, journalisation. |
| `conf/languages.json` | Traductions de l’interface, noms des langues et polices optionnelles. |
| `conf/runtime_state.json` | Derniers choix runtime de l’utilisateur, par exemple la dernière langue sélectionnée. |
| `docs/` | Documentation localisée. |
| `docs/assets/` | Images utilisées dans la documentation. |
| `fonts/` | Polices optionnelles. |
| `flexi_runtime.log` | Journal de service de l’application elle-même. |

Les métadonnées de la fenêtre About ne sont pas stockées dans un fichier runtime séparé. Le nom du produit, la version, les descriptions localisées (EN/CZ/UK/FR/DE), la date de build, le type de build, le toolset et le système cible sont compilés dans `FlexiSoftRuntime.exe` à partir de `src/about.h`. Leur modification nécessite donc un nouveau build ; le rechargement de la configuration ne modifie pas les données About.

Un déploiement manuel pratique ne nécessite donc pas de fichier séparé de métadonnées de version. La structure de base est :

```text
FlexiSoftRuntime.exe
FlexiSoftMdReader.exe          # lecteur de documentation recommandé
conf/
  config.json
  languages.json
  runtime_state.json          # optionnel ; créé/mis à jour par le runtime si nécessaire
docs/
fonts/                        # optionnel ; nécessaire uniquement avec une police fichier configurée
```

L’opérateur standard ne modifie pas ces fichiers.

---

## 3. Configuration `conf/config.json`

`conf/config.json` est la configuration machine principale. Le format recommandé est UTF-8.

Les chaînes JSON peuvent être écrites directement en UTF-8 ou avec des escapes `\uXXXX`. Les paires de surrogate UTF-16 valides sont prises en charge. L’UTF-8 direct reste le format recommandé pour les fichiers de configuration et de langue.

Structure de base :

```json
{
  "language": "",
  "transport": "",
  "serial": {},
  "network": {},
  "rk512": {},
  "poll": {},
  "inputs": [],
  "ui": {},
  "logging": {},
  "debug": {}
}
```

### 3.1 `language`

```json
"language": "cz"
```

Langue par défaut de l’application. Elle est utilisée au premier démarrage ou lorsqu’aucune langue runtime n’est enregistrée.

Recommandations :

- le fallback de base est `en`,
- le tchèque utilise `cz`,
- l’ukrainien utilise `uk`,
- d’autres langues peuvent être ajoutées dans `conf/languages.json`.

Un changement de langue pendant l’exécution n’écrase pas `conf/config.json`.

---

### 3.2 `transport`

```json
"transport": "network"
```

Définit le mode de communication.

| Valeur | Signification |
| --- | --- |
| `serial` | Port COM/RS-232 direct. |
| `network` | Transport réseau via TCP ou UDP. |

Pour une intégration dans un OP/PC de machine, le mode `network` avec un convertisseur Ethernet/RS-232 est généralement le plus adapté.

---

## 4. Communication série `serial`

La section `serial` est utilisée avec :

```json
"transport": "serial"
```

Exemple :

```json
"serial": {
  "port": "COM5",
  "baud": 115200,
  "data_bits": 8,
  "parity": "N",
  "stop_bits": 1,
  "timeout_ms": 2500
}
```

| Paramètre | Signification |
| --- | --- |
| `port` | Nom du port COM, par exemple `COM4` ou `COM5`. |
| `baud` | Vitesse de la liaison série. |
| `data_bits` | Nombre de bits de données. Généralement `8`. |
| `parity` | Parité. Généralement `N`. |
| `stop_bits` | Nombre de bits d’arrêt. Généralement `1`. |
| `timeout_ms` | Timeout pour lecture et écriture sur la liaison série. |

Pour Flexi Soft dans ce projet, le réglage courant est :

```text
115200 8N1
```

---

## 5. Communication réseau `network`

La section `network` est utilisée avec :

```json
"transport": "network"
```

Exemple :

```json
"network": {
  "mode": "tcp_client",
  "host": "192.168.0.7",
  "port": 4001,
  "bind_host": "0.0.0.0",
  "bind_port": 0,
  "connect_timeout_ms": 3000,
  "timeout_ms": 1500
}
```

| Paramètre | Signification |
| --- | --- |
| `mode` | Mode de transport réseau. |
| `host` | Adresse IP ou nom d’hôte du convertisseur. |
| `port` | Port TCP/UDP du convertisseur. |
| `bind_host` | Adresse locale pour le bind UDP. En mode client TCP, il n’est généralement pas nécessaire de la modifier. |
| `bind_port` | Port local pour le bind UDP. `0` signifie automatique. |
| `connect_timeout_ms` | Timeout pour établir une connexion TCP. |
| `timeout_ms` | Timeout de lecture d’une réponse. |

Valeurs prises en charge pour `mode` :

| Valeur | Signification |
| --- | --- |
| `tcp_client` | Le runtime se connecte comme client TCP à un convertisseur Ethernet/RS-232. Mode recommandé. |
| `udp` | Mode UDP. À utiliser seulement s’il correspond au convertisseur concret et a été testé. |

Déploiement pratique sur machine :

```text
OP/PC machine avec runtime
    -> Ethernet machine existant
    -> convertisseur Ethernet/RS-232
    -> RS-232 CPU Flexi Soft
```

Cela évite de tirer une longue liaison RS-232 séparée entre l’OP machine et l’armoire électrique.

---

## 6. RK512 `rk512`

Exemple :

```json
"rk512": {
  "device_local": "0x4F",
  "device_reply": "0x4D",
  "token_hex": "0F 0F 46 4C 58 54 30 31"
}
```

| Paramètre | Signification |
| --- | --- |
| `device_local` | Adresse RK512 locale de l’application runtime. |
| `device_reply` | Adresse de réponse RK512 attendue depuis Flexi Soft. |
| `token_hex` | Token utilisé avant l’écriture d’une commande. |

Utiliser `token_hex` dans la configuration.

Les valeurs doivent correspondre à la communication RK512 validée pour le projet concret.

---

## 7. Polling `poll`

Exemple :

```json
"poll": {
  "period_ms": 1000,
  "read_block": "0x76",
  "read_size": 54
}
```

| Paramètre | Signification |
| --- | --- |
| `period_ms` | Période de lecture du bloc d’état. |
| `read_block` | Bloc RK512 depuis lequel les états sont lus. |
| `read_size` | Nombre d’octets lus. |

À partir du bloc lu, le runtime évalue les canaux selon `status_byte`, `on_bit` et `ok_bit`.

---

## 8. Canaux `inputs`

Le runtime prend en charge jusqu’à quatre canaux. Chaque canal est un élément du tableau `inputs`.

Exemple :

```json
{
  "enabled": true,
  "name": "Côté gauche du capot",
  "status_byte": 0,
  "on_bit": 0,
  "ok_bit": 1,
  "alert_text_fr": "Vérifier que le côté gauche du capot est complètement fermé.",
  "output": {
    "block": "0x42",
    "byte": 0,
    "bit": 0,
    "pulse_ms": 1000
  },
  "repeat_fault": {
    "count": 3,
    "window_ms": 30000,
    "ignore_after_command_ms": 1500,
    "text_fr": "Le défaut revient de manière répétée. Vérifier le capot gauche et le capteur."
  }
}
```

### 8.1 Paramètres de base du canal

| Paramètre | Signification |
| --- | --- |
| `enabled` | Active/désactive le canal. Un canal désactivé n’est pas surveillé et n’affiche pas de messages. |
| `name` | Nom du canal affiché dans le menu et les messages. |
| `status_byte` | Index d’octet dans le bloc d’état lu. |
| `on_bit` | Bit informatif ON/OFF du canal. |
| `ok_bit` | Bit utilisé par le runtime pour décider si le canal est correct. |
| `alert_text_<langue>` | Texte du message normal pour la langue donnée. |

L’ancienne clé `alert_text` peut servir de fallback, mais les clés par langue sont recommandées :

```text
alert_text_en
alert_text_cz
alert_text_uk
alert_text_fr
alert_text_de
```

Si la langue active n’a pas de texte, le runtime utilise l’anglais ou un texte fallback.

### 8.2 Commande de sortie `output`

```json
"output": {
  "block": "0x42",
  "byte": 0,
  "bit": 0,
  "pulse_ms": 1000
}
```

| Paramètre | Signification |
| --- | --- |
| `block` | Bloc RK512 pour l’écriture de la commande. |
| `byte` | Octet dans les données écrites. |
| `bit` | Bit dans cet octet. |
| `pulse_ms` | Durée pendant laquelle le runtime maintient le bit de commande. |

Lors d’une commande, le runtime met le bit à 1, attend `pulse_ms`, puis remet le bit à 0.

Important : même si le runtime maintient le bit pendant une durée limitée, une courte impulsion de reset doit aussi être créée dans la logique Flexi Soft. Cela évite un effet permanent en cas de perte de communication ou d’arrêt de l’application.

---

## 9. Message répété `repeat_fault`

Repeat fault gère la situation où le même défaut revient plusieurs fois après une impulsion de reset.

Exemple :

```json
"repeat_fault": {
  "count": 3,
  "window_ms": 30000,
  "ignore_after_command_ms": 1500,
  "text_fr": "Vérifier que le capot est complètement fermé."
}
```

| Paramètre | Signification |
| --- | --- |
| `count` | Nombre de retours du défaut autorisés dans la fenêtre active avant création du message répété. Minimum `1`. |
| `window_ms` | Fenêtre temporelle pour compter les retours répétés du défaut. Minimum `1000 ms`. |
| `ignore_after_command_ms` | Temps de protection après une commande de reset. Pendant ce temps, le runtime ignore l’état transitoire des entrées causé par le reset matériel via relais. Les valeurs négatives sont traitées comme `0`. |
| `text_<langue>` | Texte du message répété pour la langue donnée. |

Clés de langue recommandées :

```text
text_en
text_cz
text_uk
text_fr
text_de
```

### 9.1 Comment le comptage des répétitions fonctionne

Après confirmation du reset, le runtime envoie la commande de reset et démarre le temps de protection `ignore_after_command_ms`.

Ce temps n’est pas principalement destiné à « masquer le retour du même défaut ». Son véritable but est de filtrer l’état transitoire des entrées pendant le reset matériel.

Pendant le reset, les canaux d’entrée surveillés sont brièvement ouverts par des relais. Dans cet état, les deux canaux d’entrée peuvent être OFF, et Flexi Soft recommence à mesurer son temps de simultanéité des canaux. Du point de vue du runtime, l’entrée peut donc sembler brièvement OK, alors qu’il ne s’agit que d’un état transitoire causé par le reset.

Si le runtime considérait cet état court comme un vrai OK, il réinitialiserait à tort la logique du message répété. C’est pourquoi l’état transitoire ne doit pas influencer la logique repeat pendant `ignore_after_command_ms`.

Pendant `ignore_after_command_ms`, le runtime :

- ne considère pas un court état OK comme une vraie réparation de l’entrée,
- ne réinitialise pas le compteur de répétition,
- ne compte pas l’état transitoire comme un nouveau retour de défaut,
- n’ouvre pas de nouveau message sur la base de l’état transitoire.

Après expiration de `ignore_after_command_ms`, le runtime évalue de nouveau l’état réel du canal :

- si le canal est toujours en défaut, le compteur de répétition augmente,
- si le canal est réellement OK après le temps de protection, le compteur de répétition est remis à zéro,
- si le compteur atteint `count`, le message répété est affiché.

Le temps d’attente de l’opérateur avant confirmation est compté dans `window_ms`. La durée de la commande, la durée de l’impulsion et le temps de protection `ignore_after_command_ms` ne sont pas comptés dans la fenêtre active.

### 9.2 Défauts actifs et limite repeat

Les compteurs de répétition sont maintenus séparément pour chaque canal surveillé. En interne, l’ensemble des défauts actuellement actifs (`errorMask`) et l’ensemble des canaux ayant atteint la limite repeat (`repeatMask`) sont distincts. Un repeat alert peut donc lister tous les défauts actuellement actifs, tandis que le texte correctif repeat n’est affiché que pour les canaux présents dans `repeatMask`.

---

## 10. UI `ui`

Exemple :

```json
"ui": {
  "tray_tooltip": "FlexiSoft RS232 Runtime",
  "tray_tooltip_en": "FlexiSoft RS232 Runtime",
  "tray_tooltip_cz": "FlexiSoft RS232 Runtime",
  "tray_tooltip_uk": "FlexiSoft RS232 Runtime",
  "tray_tooltip_fr": "FlexiSoft RS232 Runtime",
  "tray_tooltip_de": "FlexiSoft RS232 Runtime"
}
```

| Paramètre | Signification |
| --- | --- |
| `tray_tooltip` | Texte fallback de l’infobulle de l’icône. |
| `tray_tooltip_<langue>` | Infobulle localisée pour une langue donnée. |

`tray_tooltip` est conservé comme fallback legacy. Pour les configurations actuelles, il est préférable d’utiliser les clés localisées `tray_tooltip_<langue>`.

Il est recommandé d’utiliser les clés par langue :

```text
tray_tooltip_en
tray_tooltip_cz
tray_tooltip_uk
tray_tooltip_fr
tray_tooltip_de
```

---

## 11. Journalisation `logging`

Exemple :

```json
"logging": {
  "enabled": true,
  "file": "flexi_runtime.log",
  "newest_first": true,
  "max_bytes": 65536
}
```

| Paramètre | Signification |
| --- | --- |
| `enabled` | Active/désactive la journalisation. |
| `file` | Chemin du fichier journal. |
| `newest_first` | Si `true`, les lignes les plus récentes sont en haut. |
| `max_bytes` | Taille maximale approximative du journal. Minimum `4096`. |

Mode de production recommandé :

```json
"newest_first": true
```

`newest_first=false` reste disponible comme mode legacy, mais il n’est pas recommandé pour un déploiement en production.

Le journal est toujours en anglais. La langue de l’interface n’a aucun effet dessus.

---

## 12. Debug `debug`

Exemple :

```json
"debug": {
  "force_command_fail": false
}
```

| Paramètre | Signification |
| --- | --- |
| `force_command_fail` | Option de test pour forcer l’échec d’une commande. En fonctionnement normal, elle doit être `false`. |

Cette section sert uniquement aux tests et au diagnostic.

---

## 13. État runtime `conf/runtime_state.json`

`conf/runtime_state.json` enregistre le dernier choix runtime de l’utilisateur, actuellement surtout la langue.

Exemple :

```json
{
  "language": "cz"
}
```

Si le fichier est absent, la langue de `conf/config.json` est utilisée. Si le fichier est endommagé, le runtime écrit un avertissement dans le journal et utilise la langue de `conf/config.json`.

Un changement de langue pendant l’exécution n’écrase pas `conf/config.json`.

Si l’enregistrement de `runtime_state.json` échoue, le runtime écrit uniquement un warning dans le journal. La langue UI déjà sélectionnée reste active et n’est pas rétablie à la valeur précédente.

---

## 14. Traductions `conf/languages.json`

`conf/languages.json` contient les traductions de l’interface et les réglages optionnels de police.

Structure de base :

```json
{
  "languages": {
    "en": {
      "name": "",
      "font_face": "",
      "font_file": "",
      "strings": {}
    }
  }
}
```

### 14.1 `languages`

L’objet `languages` contient les langues individuelles. La clé de langue est aussi son code, par exemple :

```text
en
cz
uk
fr
de
```

### 14.2 Paramètres de langue

| Paramètre | Signification |
| --- | --- |
| `name` | Nom de la langue affiché dans le menu. |
| `font_face` | Nom d’une police système ou font face. |
| `font_file` | Fichier de police optionnel, relatif au dossier de l’application. |
| `strings` | Clés de traduction de l’interface. |

Si `font_file` existe et peut être chargé, le runtime utilise cette police. Sinon, il utilise une police fallback. C’est important surtout pour les langues qui peuvent ne pas avoir de police système adaptée sur les anciens Windows.

### 14.3 Règles de fallback des traductions

- L’anglais (`en`) sert de fallback de base.
- Si la langue active ne contient pas un texte, le texte anglais est utilisé.
- Si le texte manque aussi en anglais, le nom de la clé est affiché.
- Un `conf/languages.json` absent ou vide peut être remplacé par le runtime par un modèle anglais de base.

### 14.4 Clés de traduction importantes

#### Menu de zone de notification

| Clé | Signification |
| --- | --- |
| `tray.title` | Titre du menu. |
| `tray.reload_config` | Élément pour recharger la configuration. |
| `tray.reconnect` | Élément pour reconnecter la communication. |
| `tray.about` | Élément About. |
| `tray.exit` | Quitter le programme. |
| `tray.language` | Sous-menu des langues. |
| `tray.send_command` | Texte de la commande manuelle ; prend en charge le token `{channel}`. |

#### Textes d’état

| Clé | Signification |
| --- | --- |
| `status.ok` | État OK. |
| `status.error` | État erreur/défaut. |
| `status.on` | État ON. |
| `status.off` | État OFF. |
| `status.disabled` | Canal désactivé. |

#### Messages

| Clé | Signification |
| --- | --- |
| `alert.input.window_title` | Titre de fenêtre du message normal. |
| `alert.repeat.window_title` | Titre de fenêtre du message répété. |
| `alert.command_failed.window_title` | Titre de fenêtre du message d’échec de commande. |
| `alert.input.title` | Titre du message normal. |
| `alert.repeat.title` | Titre du message répété. |
| `alert.command_failed.title` | Titre du message d’échec de commande. |
| `alert.repeat.prefix` | Texte d’introduction du message répété. |
| `alert.active_errors` | Titre de la liste des défauts actifs. |
| `alert.command_failed.text` | Texte d’échec de commande. |
| `alert.affected_channels` | Titre de la liste des canaux concernés. |
| `alert.input.footer_yes` | Explication du bouton YES/OUI dans un message normal. |
| `alert.input.footer_no` | Explication du bouton NO/NON dans un message normal. |
| `alert.command_failed.footer_yes` | Explication du bouton YES/OUI dans un message d’échec de commande. |
| `alert.command_failed.footer_no` | Explication du bouton NO/NON dans un message d’échec de commande. |

#### Boutons

| Clé | Signification |
| --- | --- |
| `button.yes` | Texte du bouton Oui / YES. |
| `button.no` | Texte du bouton Non / NO. |
| `button.ok` | Texte du bouton OK. |

#### About et documentation

| Clé | Signification |
| --- | --- |
| `about.title` | Titre de la boîte About. |
| `about.version` | Libellé de version. |
| `about.build_date` | Libellé de date de build. |
| `about.open_readme` | Bouton pour ouvrir README. |
| `about.open_manual` | Bouton pour ouvrir MANUAL. |
| `about.open_log` | Bouton pour ouvrir le journal. |
| `about.project_page` | Bouton de page projet. |
| `error.missing_readme` | Erreur si README est absent. |
| `error.missing_manual` | Erreur si MANUAL est absent. |
| `error.missing_log` | Erreur si le journal est absent. |

---

## 15. Ajouter une autre langue

Procédure :

1. Ajouter un nouvel objet de langue dans `languages.json`, par exemple `pl`.
2. Renseigner `name`, `font_face` et éventuellement `font_file`.
3. Ajouter les textes dans `strings`.
4. Dans `config.json`, il est possible de définir `"language": "pl"`, ou de sélectionner la langue pendant l’exécution dans le menu.
5. Pour la documentation, ajouter les fichiers correspondants, par exemple :

```text
docs/README_pl.md
docs/MANUAL_pl.md
```

---

## 16. Documentation

Le runtime ouvre la documentation selon la langue active.

Documentation de base :

```text
docs/README.md
docs/MANUAL.md
```

Documentation localisée :

```text
docs/README_cz.md
docs/MANUAL_cz.md
docs/README_uk.md
docs/MANUAL_uk.md
docs/README_fr.md
docs/MANUAL_fr.md
docs/README_de.md
docs/MANUAL_de.md
```

Si le fichier localisé est absent, le fichier de base sans suffixe de langue est utilisé.

La documentation est normalement ouverte avec l’exécutable externe `FlexiSoftMdReader.exe` (v1.0.1 dans le package Runtime actuel). Le runtime transmet au Reader le document sélectionné et la police UI effective. Reader v1.0.1 contient une gestion de compatibilité pour l’ancien moteur IE utilisé sous Windows XP. Si le Reader ne peut pas être utilisé, le runtime passe à Notepad puis à ShellExecute. Le Reader prend en charge le sous-ensemble Markdown utilisé par la documentation fournie, y compris les images de `docs/assets/`.

---

## 17. Réglage dans Flexi Soft Designer

### 17.1 Routage RS-232

Dans le projet, l’option suivante doit être activée :

```text
Activer le routage RS-232 pour le CPU
```

![Réglage du routage RS-232 pour le CPU](assets/flexisoft_designer_rs232_routing.png)

### 17.2 Impulsion courte dans la logique CPU

Le runtime écrit le bit de commande, mais l’impulsion courte réelle doit aussi être créée dans le projet Flexi Soft.

Principe recommandé :

```text
bit RS-232 -> détection de front -> temporisation / impulsion courte -> relais de reset ou sortie de reset
```

![Exemple de relais de reset à partir de bits RS-232](assets/flexisoft_designer_rs232_reset_relays.png)

Raison : en cas de perte de communication ou d’arrêt du runtime, les entrées surveillées ne doivent pas rester déconnectées de façon permanente et le signal de reset ne doit pas rester actif en permanence.

---

## 18. Rechargement de la configuration

La configuration peut être rechargée depuis le menu.

Les fichiers suivants sont rechargés :

- `conf/config.json`,
- `conf/languages.json`,
- `conf/runtime_state.json`.

Les métadonnées About sont intégrées dans l’EXE et ne sont pas modifiées par le rechargement de la configuration.

Si un message est ouvert, le runtime le ferme. Si le défaut persiste, il affiche un nouveau message selon la configuration actuelle et les textes actuels.

Le reconnect utilise le même flux de message modeless : l’ancien message est fermé avant le reconnect et, si le défaut est toujours présent après le retour de la communication, un nouveau message actuel est affiché.

---

## 19. Icône et menu de zone de notification

L’icône indique l’état global de la communication et des canaux :

| État de l’icône | Signification |
| --- | --- |
| Vert | La communication est correcte et aucun canal surveillé n’est en défaut. |
| Rouge | La communication est correcte, mais au moins un canal surveillé est en défaut. |
| Jaune / neutre | Démarrage, reconnexion ou attente de confirmation de l’état. |
| Gris / défaut de communication | Le runtime n’a pas de connexion valide avec le CPU Flexi Soft. |

Le menu affiche l’état des canaux activés CH1 à CH4. Pour chaque canal, son nom ainsi que les états ON/OFF et OK/DÉFAUT sont visibles.

Les commandes manuelles ne sont disponibles que lorsque la communication avec le CPU est valide.

---

## 20. Journalisation et diagnostic

Le journal est un journal de service de l’application, pas un journal de production ni de sécurité de la machine.

Il journalise par exemple :

- démarrage de l’application,
- chargement de la configuration,
- ouverture du transport,
- erreur de communication,
- rétablissement de la communication,
- affichage d’un message,
- commande ON/OFF,
- échec de commande,
- message répété,
- reconnexion,
- shutdown.

En cas d’erreur de communication identique persistante, le runtime limite le remplissage du journal. La même erreur n’est pas journalisée à chaque cycle si l’état ou le texte d’erreur ne change pas.

---

## 21. Dépannage

### Le runtime ne se connecte pas

Vérifier :

- le type de transport,
- l’adresse IP et le port du convertisseur,
- le port COM,
- les paramètres série,
- le câblage,
- l’alimentation du convertisseur,
- le routage RS-232 dans le projet Flexi Soft,
- les adresses RK512.

### Le convertisseur est accessible, mais Flexi Soft ne répond pas

Vérifier :

- si le CPU Flexi Soft est en RUN,
- si l’interface RS-232 est correctement raccordée,
- si les réglages RK512 correspondent,
- si le bloc lu et la taille correspondent,
- si le projet contient les signaux nécessaires.

### La commande manuelle est grisée

Le runtime n’a pas de communication valide avec le CPU Flexi Soft. Rétablir d’abord la communication ou contrôler le convertisseur.

### Le message revient après reset

Après expiration du temps de protection, l’entrée surveillée est toujours en défaut. Vérifier le capot / l’enceinte, les capteurs et le câblage.

### Message répété

Le défaut revient plusieurs fois après reset. L’opérateur ne doit pas continuer à confirmer sans contrôle physique. Le capot, les capteurs, le câblage ou la logique de sécurité doivent être contrôlés.

### Commande échouée

Le bit de commande n’a pas pu être écrit ou remis à zéro. Vérifier la communication, le bloc d’écriture RK512, le token et le routage RS-232.

---

## 22. Liste de contrôle de mise en service

Avant la remise, vérifier :

- Le routage RS-232 pour le CPU est activé.
- Les bits de commande RS-232 sont raccordés dans la logique Flexi Soft.
- L’impulsion de reset est créée dans Flexi Soft comme impulsion courte.
- La communication correspond aux réglages dans `config.json`.
- Le bloc lu et la taille correspondent au projet.
- Les octets et bits d’état correspondent au projet.
- Les bits de commande de sortie correspondent au projet.
- Les textes de messages sont compréhensibles pour l’opérateur.
- Le message répété indique clairement quand appeler la maintenance.
- `ignore_after_command_ms` couvre uniquement le reset matériel transitoire des entrées et ne remet pas à zéro la logique repeat sur la base d’un faux état OK.
- L’icône et le menu correspondent à l’état réel.
- Le rechargement de la configuration recharge aussi les langues modifiées.
- Le journal est lisible.

---

## Auteur

```text
KVLab - Vladimír Kopal
vladakopal@gmail.com
```
