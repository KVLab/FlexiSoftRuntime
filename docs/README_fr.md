# FlexiSoft Runtime

FlexiSoft Runtime est une application d’assistance service pour SICK Flexi Soft. Elle a été créée afin de réduire les interventions inutiles et répétées de maintenance sur une machine CRIPPA de cintrage de tubes, où la maintenance est souvent appelée uniquement pour réinitialiser un état dû à un capot / une enceinte mal fermé(e) ou mécaniquement usé(e).

Le but n’est pas de remplacer la maintenance lorsqu’il y a une vraie panne. Le but est de filtrer une erreur opérateur courante : l’opérateur ne ferme pas correctement le capot, Flexi Soft conserve l’état de défaut, et la maintenance doit venir uniquement pour effectuer un reset. Le runtime indique clairement à l’opérateur ce qu’il doit vérifier et lui permet de déclencher une impulsion de reset préparée à l’avance.

**FlexiSoft Runtime ne fait pas partie de la fonction de sécurité de la machine.** C’est uniquement un logiciel d’aide à l’exploitation. L’évaluation sûre, l’état sûr de la machine et l’autorisation / le reset de la logique de sécurité doivent toujours être assurés par le projet Flexi Soft et par le câblage de la machine.

---

## À quoi il sert

Cas d’utilisation typique :

1. L’opérateur ferme le capot / l’enceinte de la machine.
2. À cause de l’usure, d’une fermeture incorrecte ou d’une commutation non simultanée des capteurs, un défaut reste actif dans Flexi Soft.
3. Le runtime affiche un message avec une consigne claire pour l’opérateur.
4. L’opérateur contrôle le capot et confirme l’impulsion de reset.
5. Le runtime écrit le bit de commande RS-232 configuré via RK512.
6. Le projet Flexi Soft crée une courte impulsion de reset à partir de ce bit.
7. Si le même défaut revient plusieurs fois, le runtime affiche un message répété et l’opérateur doit appeler la maintenance ou effectuer un contrôle physique.

Le runtime traite donc principalement des situations répétitives, simples mais gênantes en production, où la machine n’est pas correctement fermée mécaniquement, sans qu’une intervention de maintenance soit nécessaire uniquement pour effectuer le reset.

---

## Ce que fait le programme

- lit des états sélectionnés dans Flexi Soft via RK512,
- affiche des messages de défaut à l’opérateur,
- permet de confirmer une impulsion de reset,
- surveille le retour répété du même défaut,
- affiche un message répété plus explicite lorsque le même défaut revient plusieurs fois,
- permet une commande manuelle depuis le menu de la zone de notification,
- prend en charge une interface multilingue et une documentation localisée,
- écrit son propre journal de service de l’application.

Le runtime **ne journalise pas les événements machine comme un enregistreur de production ou de sécurité**. Il journalise ses propres événements : démarrage, chargement de la configuration, état de communication, messages affichés, commandes, reconnexion, arrêt et informations de service similaires.

---

## Communication avec Flexi Soft

Le runtime communique avec Flexi Soft via RK512.

Types de connexion pris en charge :

| Type de connexion | Utilisation |
| --- | --- |
| COM direct / RS-232 | Le runtime fonctionne sur un PC qui possède un port série physique ou virtuel connecté à Flexi Soft. |
| TCP via convertisseur Ethernet/RS-232 | Le runtime fonctionne sur le PC de la machine et se connecte au CPU Flexi Soft via l’infrastructure réseau existante de la machine. |

Pour une intégration dans une machine existante, l’utilisation d’un convertisseur Ethernet/RS-232 est généralement pratique. Le runtime peut alors fonctionner directement sur l’OP/PC de la machine, souvent même sur un ancien système Windows XP. La communication avec le CPU Flexi Soft peut passer par le réseau machine existant au lieu d’un long câble série séparé.

La modification de la machine reste ainsi minimale :

- Le runtime fonctionne sur le PC/OP de la machine.
- Flexi Soft reste l’unité principale de sécurité.
- Le convertisseur ne fait que transporter la communication RK512 entre le runtime et l’interface RS-232 de Flexi Soft.
- L’infrastructure réseau existante de la machine peut servir à la liaison runtime ↔ convertisseur.

---

## Fichiers importants pour l’intégration et le service

L’opérateur standard ne travaille pas avec ces fichiers. Pour l’intégration et le service, les fichiers les plus importants sont :

| Fichier / dossier | Signification |
| --- | --- |
| `conf/config.json` | Communication, canaux surveillés, textes des messages, bits de commande de sortie et règles de message répété. |
| `conf/languages.json` | Traductions de l’interface et polices optionnelles. |
| `conf/runtime_state.json` | Derniers choix runtime de l’utilisateur, actuellement surtout la langue sélectionnée. |
| `FlexiSoftMdReader.exe` | Lecteur de documentation externe utilisé par le Runtime. |
| `fonts/` | Polices optionnelles pour les langues qui peuvent ne pas disposer d’une police système adaptée. |
| `docs/` | Documentation localisée. |
| `flexi_runtime.log` | Journal de service de l’application. |

Les métadonnées About sont intégrées directement dans `FlexiSoftRuntime.exe` au moment du build à partir de `src/about.h` ; elles ne sont pas stockées dans un fichier de configuration runtime séparé. Les données About intégrées contiennent les informations produit/version/build et les descriptions localisées EN/CZ/UK/FR/DE.

---

## Interface multilingue

La langue par défaut est l’anglais. D’autres langues sont ajoutées dans `conf/languages.json`.

Le runtime n’est pas limité à une liste fixe de langues. Si les traductions et, éventuellement, la documentation sont ajoutées, une autre langue peut être ajoutée.

La documentation s’ouvre selon la langue active si le fichier correspondant existe :

```text
docs/README_<langue>.md
docs/MANUAL_<langue>.md
```

Si la documentation localisée est absente, la documentation de base sans suffixe de langue est utilisée.

---

## Réglage dans Flexi Soft Designer

### Activer le routage RS-232 pour le CPU

Dans le projet Flexi Soft, l’option suivante doit être activée :

```text
Activer le routage RS-232 pour le CPU
```

![Réglage du routage RS-232 pour le CPU](assets/flexisoft_designer_rs232_routing.png)

Sans ce réglage, les signaux RS-232 peuvent ne pas être disponibles pour la logique CPU.

### Impulsion de reset à partir d’un bit RS-232

Le runtime écrit uniquement le bit de commande RS-232. L’impulsion de reset réelle doit être créée dans la logique du CPU Flexi Soft.

Principe recommandé :

```text
bit RS-232 -> détection de front -> temporisation / impulsion courte -> relais de reset ou sortie de reset
```

Exemple :

```text
RS232 0.0 -> détection de front -> temporisation au relâchement -> XTIO[1].Q1
RS232 0.1 -> détection de front -> temporisation au relâchement -> XTIO[1].Q2
```

![Exemple de relais de reset à partir de bits RS-232](assets/flexisoft_designer_rs232_reset_relays.png)

Cette logique est recommandée pour obtenir un comportement sûr en cas de défaut de communication. Si la communication tombe pendant une commande, ou si le runtime cesse de fonctionner, les entrées surveillées ne doivent pas rester déconnectées de façon statique et le signal de reset ne doit pas rester actif en permanence. L’impulsion courte doit donc être assurée par le projet Flexi Soft, et non uniquement par le runtime.

La configuration dans `conf/config.json` doit correspondre à l’utilisation des bits de commande RS-232 dans le projet Flexi Soft.

---

## Messages

Le runtime affiche les messages selon les canaux configurés.

Un message normal indique à l’opérateur ce qu’il doit vérifier, par exemple :

```text
Vérifier que le capot est complètement fermé.
Ouvrir puis refermer correctement le capot.
Après le contrôle, confirmer le reset.
```

L’opérateur dispose généralement de deux choix :

| Choix | Signification |
| --- | --- |
| Oui / YES | Déclenche l’impulsion de reset après le contrôle. |
| Non / NO | Ferme le message sans impulsion de reset. |

Les textes des messages sont définis dans la configuration.

---

## Message répété

Un message répété s’affiche si le même défaut revient plusieurs fois après l’impulsion de reset.

C’est important surtout pour un capot / une enceinte mécaniquement usé(e) :

- un défaut ponctuel peut être seulement une fermeture incorrecte,
- un défaut répété signifie que le capot, le capteur ou le câblage doit être contrôlé,
- l’opérateur ne doit pas confirmer le reset indéfiniment sans vérifier la cause.

Exemple de texte de message répété :

```text
Le défaut revient de manière répétée.
Vérifier la fermeture correcte du capot et l’état du capteur.
Si ce message se répète, appeler la maintenance.
```

---

## Icône de zone de notification et état des canaux

Le runtime fonctionne dans la zone de notification Windows. La couleur de l’icône donne une information rapide sur l’état global de l’application et de la communication.

| État de l’icône | Signification |
| --- | --- |
| Vert | La communication avec le CPU Flexi Soft est correcte et aucun canal surveillé n’est en défaut. |
| Rouge | La communication est correcte, mais au moins un canal surveillé signale un défaut. |
| Jaune / neutre | L’application fonctionne, mais la communication ou l’état du CPU n’est pas encore complètement confirmé. Typiquement au démarrage, pendant une reconnexion ou en attente d’une réponse. |
| Gris / défaut de communication | Le runtime n’a pas de connexion valide avec le CPU Flexi Soft ou la communication est en défaut. |

Le menu de la zone de notification affiche aussi l’état des canaux surveillés CH1 à CH4. Pour chaque canal activé, son nom et son état sont visibles, par exemple si le canal est actif/inactif et s’il est correct ou en défaut.

Ainsi, l’opérateur n’a pas besoin d’ouvrir immédiatement le diagnostic Flexi Soft. Le menu montre rapidement quelle entrée surveillée pose problème.

---

## Commande manuelle depuis le menu de la zone de notification

Le menu peut contenir une commande manuelle pour les différents canaux.

La commande manuelle n’est disponible que lorsque la communication avec le CPU Flexi Soft est valide. Si la communication n’est pas correcte, les commandes sont grisées.

---

## Recharger la configuration, reconnecter et quitter

Le menu permet de recharger la configuration. Cela recharge non seulement `conf/config.json`, mais aussi les fichiers de langue et le réglage de langue runtime. C’est utile lors de la modification des textes de messages, des traductions ou des paramètres de communication sans redémarrer manuellement le programme.

Si un message est ouvert pendant le rechargement de la configuration, le runtime le ferme. Si le défaut persiste, un nouveau message actuel s’affiche après le rechargement, avec la nouvelle configuration et les nouveaux textes.

Les métadonnées About font partie de l’EXE et ne sont pas modifiées par le rechargement de la configuration.

Le menu permet aussi de lancer manuellement une reconnexion de la communication. Cela s’utilise en cas de problème avec le convertisseur, la connexion réseau ou le CPU.

L’application se ferme depuis le menu. Lors de l’arrêt, le runtime écrit son propre enregistrement shutdown dans le journal.

---

## Journal

Journal par défaut :

```text
flexi_runtime.log
```

Le journal est un enregistrement de service de l’application elle-même. Il est toujours en anglais, quelle que soit la langue de l’interface.

Le journal contient par exemple :

- démarrage de l’application,
- chargement de la configuration,
- état de la communication,
- perte et rétablissement de la communication,
- commande ON/OFF,
- message répété,
- reconnexion manuelle,
- shutdown.

Le journal ne remplace pas le diagnostic de sécurité Flexi Soft ni un journal de production machine.

---

## Documentation

Le programme peut ouvrir une documentation localisée par langue.

Noms de base :

```text
docs/README.md
docs/MANUAL.md
```

Noms localisés :

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

Le suffixe utilisé doit correspondre au code de langue dans `languages.json`.

La documentation est normalement ouverte avec `FlexiSoftMdReader.exe` (v1.0.1 dans le package Runtime actuel). Le runtime transmet au Reader le document et la police UI effective. Reader v1.0.1 contient une gestion de compatibilité pour l’ancien moteur IE sous Windows XP. Si le Reader ne peut pas être utilisé, le runtime passe à Notepad puis à ShellExecute.

---

## Installation

Cette section sera complétée après la finalisation du package d’installation.

Sujets prévus :

- installation du programme,
- emplacement de la configuration,
- mise à jour sans écraser la configuration machine,
- démarrage automatique,
- mise à niveau de service,
- sauvegarde de la configuration.

---

## Contrôle de base après déploiement

Après le déploiement, vérifier :

- Le routage RS-232 pour le CPU est activé dans Flexi Soft Designer.
- Les bits de commande RS-232 sont raccordés dans la logique Flexi Soft.
- L’impulsion de reset est créée dans la logique Flexi Soft comme impulsion courte.
- Le runtime établit la communication.
- La couleur de l’icône correspond à l’état réel de la communication et des canaux.
- L’état des canaux surveillés dans le menu correspond à l’état réel de Flexi Soft.
- Les messages correspondent aux états réels de la machine.
- La commande de reset écrit le bon bit.
- Le rechargement de la configuration recharge aussi les traductions modifiées et les fichiers de langue.
- Le message répété s’affiche lorsque le même défaut revient plusieurs fois.

---

## Auteur

```text
KVLab - Vladimír Kopal
vladakopal@gmail.com
```
