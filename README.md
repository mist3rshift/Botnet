# Conception et Développement d'un Botnet avec C&C


## Objectif du projet

Ce projet doit vous permettre de mettre en œuvre des compétences en développement C, en implémentation de structures de données, en programmation système et réseaux, et en gestion de projet.

L'objectif de ce projet est de concevoir et de développer un botnet avec une architecture composée :
- d'un serveur de commande et de contrôle (C&C) : ce serveur central est responsable de la coordination des bots et de l'envoi des instructions ;
- d'une multitude de bots : des petits clients qui s'enregistrent auprès du serveur C&C, exécutent des commandes reçues de ce serveur et lui renvoient un rapport sur ces exécutions.


![BotNet](./figs/botnet.png)


Les bots seront capables d'effectuer diverses tâches malveillantes comme la collecte d’informations sur la machine hôte, le lancement d'une attaque groupée DDoS, le déploiement de binaires ou l'exécution de commandes système.

Sont exclus du périmètre de ce projet : 
- les phases d'identification et d'infection de machines hôtes pour les bots ;
- la phase d'auto-protection durant laquelle généralement un bot tente de se dissimuler sur la machine hôte pour continuer son action ;
- la phase de propagation durant laquelle le botnet tente généralement de s'étendre.

Le périmètre du projet se limite donc aux phases suivantes :
- la phase d'activation durant laquelle un bot se déclare au centre de commande ;
- la phase opérationnelle durant laquelle les bots vont exécuter les ordres qui leur sont donnés par le centre de commande et rendre compte à celui-ci.

La phase de mise à jour qui permet à un botnet de se mettre à jour est une fonctionnalité optionnelle.


## Objectifs Pédagogiques

- Élaborer un cahier des charges.
- Réaliser un état de l'art.
- Implémentation de structures de données et d'algorithmes avancés en langage C.
- Développement et application de tests unitaires pour assurer la qualité et la fiabilité du code.
- Expérience concrète de la gestion de projet en équipe (planification, suivi d'avancement, compte rendu de réunion, etc.).
- Mise en pratique de la gestion de versions (utilisation de git/GitLab).


## Cahier des charges

#### Fonctionnalités minimales requises

##### Côté Serveur (C&C) :

- Capacité à gérer les connexions de plusieurs bots.
- Envoi de commandes aux bots (via sockets).
- Journalisation des bots connectés et des commandes envoyées.

##### Côté Bot :

- Enregistrement et communication avec le serveur.
- Exécution des commandes reçues :
   - récupération d’informations système ;
   - simulation de ping massif (DDoS) vers une cible définie ;
   - simulation d'une attaque de type TCP/SYN Flooding vers une cible définie.

##### Interface utilisateur (sur le serveur) :

- Liste des bots actifs.
- Interface pour choisir une commande et la transmettre à des bots spécifiques.


#### Technologies

- **C** : Utilisation exclusive du langage C pour des bots et du centre de contrôle.
- **Bibliothèques réseau** : BSD Sockets (probablement).


## Étapes du projet


#### Étude préalable et conception

- Réalisez une étude sur le fonctionnement et l'architecture réseau et l'architecture logicielle d'un botnet.
- Définissez les messages d’échange entre le serveur et les bots.
- Définissez l'ensemble des structures de données nécessaires (listes des bots, données des commandes à exécuter, etc.) 

#### Implémentation du serveur de commande et de contrôle (C&C)

- Création d’un serveur multiclient en C utilisant les sockets TCP.
- Gestion de multiples connexions avec `select()`, `poll()` ou des threads.
- Fonctionnalités de base : ajout et suppression de bots, envoi de commandes.


#### Implémentation des bots

- Développement de bots capables de :
   - S'enregistrer au serveur.
   - Récupérer et interpréter les commandes.
   - Exécuter des tâches comme la collecte d’informations système (exemple : IP, nom d’hôte) ou comme un "ping flood" sur une adresse spécifiée.


#### Implémentation des tâches spéciales et fonctionnalités optionnelles

- Exemple de tâches :
   - Exécution de commandes système ;
   - Simulation d’une attaque DDoS (localisée pour éviter tout impact réel) ;
   - Collecte et envoi de fichiers de données au serveur.
   - Téléchargement de binaires (à exécuter) aux bots.
- Chiffrement simple des communications entre le C&C et les bots (exemple : AES ou XOR).
- Fonctionnalités de mise à jour des bots par le C&C.

#### Documentation et tests
   - Documentation complète du code et des choix d'implémentation (structures de données, algorithmes mis en œuvre, etc.).
   - Tests unitaires et fonctionnels pour s'assurer que du fonctionnement et de la résilience du botnet.


##  Exigences du projet et points évalués

Pour l'évaluation, les points suivants seront pris en considération :

- Qualité de la modélisation et des **structures de données** utilisées pour représenter les éléments du botnet.
- Implémentation correcte des **connexions réseaux** et de la **communication** entre le C&C et les bots.
- Fonctionnalités disponibles sur le C&C et les bots.
- Respect des **bonnes pratiques de programmation** (structure du code, compilation séparée, commentaires, organisation, Makefile).
- Tests et gestion des erreurs (robustesse de l’application).


## Rendu Final

Le rendu final sera constitué a minima des éléments suivants :

- **Code source** : Livraison du code source complet et des instructions pour exécuter le centre de commande et les bots. 
- **État de l'art** : Rapport de l’état de l’art sur le fonctionnement d'un botnet. 
- **Documentation** : Comprend une description des fonctionnalités de votre botnet, des détails techniques et tous les éléments de gestion de projet que vous aurez produits (fiche de projet, comptes-rendus de réunion, planification et répartition des tâches, analyse post-mortem des efforts individuels et de l'atteinte des objectifs, etc.).
- **Tests unitaires** : Un ensemble de tests unitaires accompagnant le code source. 

Tous ces éléments seront fournis sur le dépôt GitLab du projet.


## Date de rendu et Soutenance

Le projet est à rendre pour le **26 mai 2025** à 22 heures au plus tard.

Des soutenances de groupes de projet seront organisées dans la foulée (début juin).

Votre projet fera l'objet d'une démonstration devant un jury composé d'au moins deux membres de l’équipe pédagogique. Durant cette soutenance, vous serez jugés sur votre démonstration de l'application et votre capacité à expliquer votre projet et son fonctionnement. Chaque membre du groupe devra être présent lors de soutenance et **participer activement**.

*Toute personne ne se présentant pas à la soutenance sera considérée comme démissionnaire de l'UE et en conséquence, ne pourra pas la valider pour l’année universitaire 2024-2025.*

Il est attendu que chaque membre du groupe ait contribué à plusieurs parties fonctionnelles du code (il ne s'agit pas d'avoir uniquement corrigé quelques lignes par ci et par là).


## Développement incrémental

Il est vivement recommandé à ce que le groupe adopte une stratégie de développement incrémentale.

L'idée est donc de planifier et de définir des "incréments" ou de petites unités fonctionnelles du botnet (ou de ces composants). Cela permet de se concentrer sur une petite section de code à la fois et d'être toujours capable d'avoir une version fonctionnelle de l'application complète. Cela permet également d'éviter l'effet tunnel : de commencer le développement de beaucoup de fonctionnalités et de n'avoir finalement rien ou pas grand-chose de fonctionnel à montrer à la fin du projet.


## Fraude, tricherie et plagiat

Ne trichez pas ! Ne copiez pas ! Ne plagiez pas ! Si vous le faites, vous serez lourdement sanctionnés. Nous ne ferons pas de distinction entre copieur et copié. Vous n'avez pas de (bonnes) raisons de copier. De même, vous ne devez pas utiliser de solution clé en main trouvée sur internet.

Par tricher, nous entendons notamment :
- Rendre le travail d’un collègue en y apposant votre nom ;
- Obtenir un code, une solution par un moteur de recherche (ou une IA) et la rendre sous votre nom ;
- Récupérer du code et ne changer que les noms de variables et fonctions ou leur ordre avant de les présenter sous votre nom 
- Autoriser consciemment ou inconsciemment un collègue à s'approprier votre travail personnel. Assurez-vous particulièrement que votre projet et ses différentes copies locales ne soient lisibles que par vous et les membres de votre groupe.

Nous encourageons les séances de *brainstorming* et de discussion entre les élèves sur le projet. C’est une démarche naturelle et saine comme vous la rencontrerez dans votre vie professionnelle. Si les réflexions communes sont fortement recommandées, vous ne pouvez rendre que du code et des documents écrits par vous-même. Vous indiquerez notamment dans votre rapport toutes vos sources (comme les sites internet que vous auriez consultés), en indiquant brièvement ce que vous en avez retenu.
Il est quasi certain que nous détections les tricheries. En effet, les rapports et les codes sont systématiquement soumis à des outils de détection de plagiat et de copie. Il existe spécifiquement des outils de détection de manipulation de code extraordinaire mis à disposition par l’Université de Stanford, tels que `MOSS` (https://theory.stanford.edu/~aiken/moss/) ou `compare50` (https://cs50.readthedocs.io/projects/compare50/). De plus, chacun a son propre style de programmation et personne ne développe la même chose de la même manière.

Puisqu'il s'agit d'un projet réalisé dans le cadre de cours avancés de programmation, nous nous attendons à ce que vous soyez capable d'apprendre à déboguer des programmes par vous-même. Par exemple, demander à un autre élève de regarder directement votre code et de donner des suggestions d'amélioration commence à devenir un peu délicat au niveau éthique.

Si vous rencontrez des difficultés pour terminer une tâche, veuillez contacter l'un de vos enseignants afin que nous puissions vous aider. Nous préférons de loin passer du temps à vous aider plutôt que de traiter des cas de fraudes.

