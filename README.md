# dmg-torctl

*Contrôleur xPL de sorties TOR à partir de téléinformations ERDF*

---
Copyright 2016-2017 (c), epsilonRT

<a href="http://www.cecill.info/licences/Licence_CeCILL_V2.1-en.html">
  <img src="https://raw.githubusercontent.com/epsilonrt/gxPL/master/doc/images/osi.png" alt="osi.png" align="right" valign="top">
</a>

dmg-torctl est contrôleur qui permet la commande de sorties binaires (TOR) 
en manuelle, ou automatique à partir d'informations fournies par le moniteur
xPL de téléinformation ERDF xpl-tinfomon.

## Installation

### Pré-requis

Il faut installer au prélable [SysIo](https://github.com/epsilonrt/sysio) et 
[gxPL](https://github.com/epsilonrt/gxPL).

### Compilation et Installation
 
Il suffit alors d'exécuter les commandes suivantes :

        git clone https://github.com/epsilonrt/domogik-torctl.git
        cd domogik-torctl
        make
        sudo make install

### Installation script de démarrage

Pour automatiser le démarrage et l'arrêt, il est possible d'installer le script de démarrage:

        cd init
        sudo make install

## Commande

    dmg-torctl - xPL Teleinformation controller
    Copyright (c) 2016 epsilonRT

    Usage: dmg-torctl [-i interface] [-n iolayer] [-W timeout] [-D] [-d] [-h]
      -i interface - use interface named interface (i.e. eth0) as network interface
      -n iolayer   - use hardware abstraction layer to access the network
                     (i.e. udp, xbeezb... default: udp)
      -W timeout   - set the timeout at the opening of the io layer
      -D           - do not daemonize -- run from the console
      -d           - enable debugging, it can be doubled or tripled to
                     increase the level of debug.
      -h           - print this message

Pour lancer le daemon en mode débugage:

    dmg-torctl -D -ddd

Un script de lancement dmg-torctl est installé dans /etc/init.d et peut être
lancé à l'aide de la commande:

    sudo /etc/init.d/dmg-torctl start

Il peut être lancé automatiquement au démarrage du système:

    sudo insserv dmg-torctl


## Paramètres configurables

Les paramètres ci-dessous sont configurables par l'intermédiaire du protocole
de configuration prévu par xPL :

* interval interval en minutes entre 2 battements de coeur
* newconf nom de la configuration
* group groupes dont dmg-torctl fait partie, voir 
  [Devices Groups](http://xplproject.org.uk/wiki/XPL_Specification_Document.html#Device_Groups)
* filter filtre de messages utilisés, voir 
  [Installer and User Defined Filters](http://xplproject.org.uk/wiki/XPL_Specification_Document.html#Installer_and_User_Defined_Filters)
* **output** configuration des sorties TOR. Il y a autant de paramètres 
  **output** que de sorties. Les paramètres sont rangés dans l'ordre croissant
  des numéros de sortie. Un paramètre **output** a la forme:
  
        name[:type:on_value:off_value][:adco]
  
    - **name** : nom donnée à la sortie, ce nom sera utilisé dans les messages
      xPL pour identifier la sortie (device). Ce nom est obligatoire. Si il est
      seul, la sortie est en mode manuel et ne réagira qu'aux messages de 
      commande xPL.
    - **:type:on_value:off_value**: ce bloc est optionnel et indique que la
      sortie est en mode automatique. Elle réagira aux messages de commande xPL
      et aux messages XPL-TRIG du schéma teleinfo.basic:
        + **:type** type de message XPL-TRIG du schéma teleinfo.basic modifiant 
          l'état de la sortie. type fait partie de la liste { "ptec", "adps", 
          "demain", "motdetat" }.
        + **:on_value** valeur **current** qui déclenche l'activation de la 
          sortie.
        + **:off_value** valeur **current** qui déclenche l'inactivation de la 
          sortie.
    - **adco** ce bloc est optionnel et permet de préciser l'identifiant du
      compteur électrique qui commandera la sortie. Si non précisé, la sortie
      réagit à n'importe quel compteur.
  
Par exemple la configuration **ecs:ptec:hc:hp:** indique que:

* la sortie **ecs** est configurée en automatique,
* pour réagir au messages XPL-TRIG du schéma teleinfo.basic de **type=ptec** 
  (changement de période tarifaire en cours)
* que la sortie sera activé en heures creuses et 
* que la sortie sera inactivée en heures pleines et
* qu'elle réagit à n'importe quel compteur.
    
## Messages xPL

### XPL-STAT Structure

    sensor.basic
    {
    device=<output_name/output_name-auto>
    type=<output>
    current=<0/1>
    }

Ce message est émis en réponse à un message **XPL-CMND** du schéma 
**sensor.request**. Le champ **device** correspond à l'identifiant de la 
sortie concernée, il correspond au paramètre configurable **name** décrit dans 
le paragraphe des *Paramètres configurables*. Il peut être suffixé par **-auto** 
et dans ce cas, c'est le mode de fonctionnement de la sortie conerné qui est 
transmis: 0 pour manuel, 1 pour automatique.

### XPL-TRIG Structure

    sensor.basic
    {
    device=<output_name/output_name-auto>
    type=<output>
    current=<0/1>
    }

Ce message est émis si un changement d'état ou de mode de la sortie se produit.

Le champ **device** correspond à l'identifiant de la 
sortie concernée, il correspond au paramètre configurable **name** décrit dans 
le paragraphe des *Paramètres configurables*. Il peut être suffixé par **-auto** 
et dans ce cas, c'est le mode de fonctionnement de la sortie conerné qui est 
transmis: 0 pour manuel, 1 pour automatique.

### XPL-CMND Structure

    sensor.request
    {
    request=current
    [device=output_name/output_name-auto>]
    [type=<output>]
    }

Ce message permet de demander l'état ou le mode d'une plusieurs sorties. 

Si **request=current** est le seul champ du message, la réponse sera constituée de 
plusieurs messages **XPL-STAT** correspond à l'état et au mode de fonctionnement
de chaque toutes les sorties. 

Si le champ **device** est fourni, il permet de préciser la sortie demandée. Si 
le champ **device** est de la forme **output_name-auto** (nom de sortie avec le 
suffix **-auto**), c'est le mode de fonctionnement qui sera transmis en réponse.


    control.basic
    {
    device=<output name>
    type=<output/mode>
    current=<value>
    }

Ce message permet de modifier l'état ou le mode d'une sortie correspondant au 
champ **device**.Si le champ **device** est de la forme **output_name-auto** 
(nom de sortie avec le suffix **-auto**), c'est le mode de fonctionnement qui 
sera modifié (0 pour manuel, 1 pour auto).

**current** fait partie de la liste { "high", "on",
"true", "enable", "1", "low", "off", "false", "disable", "0" } en majuscules ou 
minuscules. 

L'état de la sortie peut être modifié que la sortie soit en mode manuel ou 
automatique.

Un message **XPL-TRIG** du schéma **sensor.basic** sera émis en réponse à cette 
commande **si** l'état ou le mode de la sortie change d'état.
