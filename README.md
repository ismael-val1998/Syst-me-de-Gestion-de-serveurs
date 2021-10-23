# Projet-Syst-me-d-exploitation-Gestion-de-serveurs

Plusieurs tâches sont à effectuer sur le nouveau serveur :

- un module “test server” permettant de tester quel serveur est disponible (Serveur de Production ou Serveur de Backup);
- un module “synchro list” maintenant une liste synchronisée des noms de fichiers avec leur date entre les deux serveurs afin de minimiser les accès disque;
- un module “copy list” permettant de rapatrier les nouvelles données. Pour éviter de gros téléchargement, seules les nouvelles données devront être transférées 
- ce module utilisera donc le module “synchro list”;
- un module “stat” de statistiques : chaque module pourra venir régulièrement faire évoluer des données de
statistiques le concernant (exemple : nombre de fichiers reçus, nombre d’erreurs, etc.). Chaque module a ses
propres informations à venir renseigner ;

- un module “log” d'enregistrement des logs. Chaque module peut venir à tout moment inscrire un log (message
sous forme de texte de 128 octets maximum) dans ce module.
